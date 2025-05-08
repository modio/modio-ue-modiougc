/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCPluginWizardDefinition.h"

#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "Features/IPluginsEditorFeature.h"
#include "FileHelpers.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "ModioUGCEditor.h"
#include "UGC/Types/UGC_Metadata.h"

#define LOCTEXT_NAMESPACE "ModioUGCPluginWizard"

FModioUGCPluginWizardDefinition::FModioUGCPluginWizardDefinition(bool bContentOnlyProject)
	: bIsContentOnlyProject(bContentOnlyProject)
{
	PluginBaseDir = IPluginManager::Get().FindPlugin(TEXT("ModioUGC"))->GetBaseDir();

	PopulateTemplatesSource();
}

bool FModioUGCPluginWizardDefinition::HasModules() const
{
	bool bHasModules = false;

	if (FPaths::DirectoryExists(PluginBaseDir / TEXT("Templates") / CurrentTemplateDefinition->OnDiskPath /
								TEXT("Source")))
	{
		bHasModules = true;
	}

	return bHasModules;
}

FText FModioUGCPluginWizardDefinition::GetInstructions() const
{
	return LOCTEXT("CreateNewUGCPanel",
				   "Give your new UGC package a name and click 'Create Mod' to make a new content only UGC package.");
}

bool FModioUGCPluginWizardDefinition::GetPluginIconPath(FString& OutIconPath) const
{
	// Replace this file with your own 128x128 image if desired.
	OutIconPath = PluginBaseDir / TEXT("Resources") / TEXT("ModioUGCEditor") / TEXT("Icon128.png");
	return false;
}

EHostType::Type FModioUGCPluginWizardDefinition::GetPluginModuleDescriptor() const
{
	EHostType::Type ModuleDescriptorType = EHostType::Runtime;

	if (CurrentTemplateDefinition.IsValid())
	{
		ModuleDescriptorType = CurrentTemplateDefinition->ModuleDescriptorType;
	}

	return ModuleDescriptorType;
}

ELoadingPhase::Type FModioUGCPluginWizardDefinition::GetPluginLoadingPhase() const
{
	ELoadingPhase::Type Phase = ELoadingPhase::Default;

	if (CurrentTemplateDefinition.IsValid())
	{
		Phase = CurrentTemplateDefinition->LoadingPhase;
	}

	return Phase;
}

bool FModioUGCPluginWizardDefinition::GetTemplateIconPath(TSharedRef<FPluginTemplateDescription> InTemplate,
														  FString& OutIconPath) const
{
	OutIconPath = PluginBaseDir / TEXT("Resources") / TEXT("ModioUGCEditor") / TEXT("Icon128.png");
	return false;
}

FString FModioUGCPluginWizardDefinition::GetPluginFolderPath() const
{
	return GetFolderForTemplate(CurrentTemplateDefinition.ToSharedRef());
}

TArray<FString> FModioUGCPluginWizardDefinition::GetFoldersForSelection() const
{
	TArray<FString> SelectedFolders;
	SelectedFolders.Add(BackingTemplatePath); // This will always be a part of the UGC plugin

	if (CurrentTemplateDefinition.IsValid())
	{
		SelectedFolders.AddUnique(PluginBaseDir / TEXT("Templates") / CurrentTemplateDefinition->OnDiskPath);
	}

	return SelectedFolders;
}

void FModioUGCPluginWizardDefinition::PluginCreated(const FString& PluginName, bool bWasSuccessful) const
{
	// Override Category to UGC
	if (bWasSuccessful)
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
		if (Plugin != nullptr)
		{
			FPluginDescriptor Desc = Plugin->GetDescriptor();
			Desc.Category = "UGC";
			FText UpdateFailureText;
			Plugin->UpdateDescriptor(Desc, UpdateFailureText);
		}
	}
}

void FModioUGCPluginWizardDefinition::PopulateTemplatesSource()
{
	const FText RegularContentOnlyTemplateName = LOCTEXT("RegularContentOnlyLabel", "Regular Content Only");
	const FText RegularContentOnlyDescription = LOCTEXT(
		"ContentOnlyTemplateDesc", "Create a regular blank plugin to be used for UGC that can only contain content.");

	// @TODO: Don't hardcode this struct class so that games can create their own and have it be used here
	BackingTemplate = MakeShareable(new FModioUGCPluginTemplateDescription(
		RegularContentOnlyTemplateName, RegularContentOnlyDescription, TEXT("BaseTemplate"), true, EHostType::Runtime));
	BackingTemplate->bCanBePlacedInEngine = false;

	TemplateDefinitions.Add(BackingTemplate.ToSharedRef());
	BackingTemplatePath = PluginBaseDir / TEXT("Templates") / BackingTemplate->OnDiskPath;
}

FString FModioUGCPluginWizardDefinition::GetFolderForTemplate(TSharedRef<FPluginTemplateDescription> InTemplate) const
{
	return InTemplate->OnDiskPath;
}

void FModioUGCPluginTemplateDescription::OnPluginCreated(TSharedPtr<IPlugin> NewPlugin)
{
	const FString DataAssetName = TEXT("UUGC_Metadata");
	UClass* MetadataClass = UUGC_Metadata::StaticClass();

	FString PluginRootPackagePath;
	FString PluginContentDir = NewPlugin->GetContentDir();

	if (!PluginContentDir.IsEmpty() &&
		FPackageName::TryConvertFilenameToLongPackageName(PluginContentDir, PluginRootPackagePath))
	{
		// Ensure path ends with a slash
		if (!PluginRootPackagePath.EndsWith(TEXT("/")))
		{
			PluginRootPackagePath += TEXT("/");
		}
	}
	else
	{
		// Fallback: Construct manually (less robust but often works)
		PluginRootPackagePath = FString::Printf(TEXT("/%s/"), *NewPlugin->GetName());
	}

	const FString FullPackagePath = PluginRootPackagePath + DataAssetName;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UDataAssetFactory* DataAssetFactory = NewObject<UDataAssetFactory>();
	if (DataAssetFactory == nullptr)
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("OnPluginCreated: Failed to create UDataAssetFactory for plugin %s."),
			   *NewPlugin->GetName());
		return;
	}

	if (UObject* CreatedObject =
			AssetTools.CreateAsset(DataAssetName, PluginRootPackagePath, MetadataClass, DataAssetFactory))
	{
		UE_LOG(LogTemp, Log, TEXT("OnPluginCreated: Successfully created Data Asset '%s' in plugin %s."),
			   *FullPackagePath, *NewPlugin->GetName());

		TArray<UPackage*> MetadataPackage {CreatedObject->GetPackage()};
		UEditorLoadingAndSavingUtils::SavePackages(MetadataPackage, false);
	}
	else
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("OnPluginCreated: Failed to create Data Asset '%s' in plugin %s."),
			   *FullPackagePath, *NewPlugin->GetName());
	}
}

const TArray<TSharedRef<FPluginTemplateDescription>>& FModioUGCPluginWizardDefinition::GetTemplatesSource() const
{
	return TemplateDefinitions;
}

void FModioUGCPluginWizardDefinition::OnTemplateSelectionChanged(TSharedPtr<FPluginTemplateDescription> InSelectedItem,
																 ESelectInfo::Type SelectInfo)
{
	CurrentTemplateDefinition = InSelectedItem;
}

TSharedPtr<FPluginTemplateDescription> FModioUGCPluginWizardDefinition::GetSelectedTemplate() const
{
	return CurrentTemplateDefinition;
}

void FModioUGCPluginWizardDefinition::ClearTemplateSelection()
{
	CurrentTemplateDefinition.Reset();
}

bool FModioUGCPluginWizardDefinition::HasValidTemplateSelection() const
{
	return CurrentTemplateDefinition.IsValid();
}

#undef LOCTEXT_NAMESPACE
