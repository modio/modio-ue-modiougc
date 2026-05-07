// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/TransactionStep.h"
#include "FileHelpers.h"
#include "Interfaces/IPluginManager.h"
#include "../Plugins/Developer/PluginUtils/Source/PluginUtils/Public/PluginUtils.h"
#include "Subsystems/EditorAssetSubsystem.h"

void UTransactionStepCreateFile::Revert()
{
	// if file is uasset/umap
	//// unload from registry
	auto Extension = FPaths::GetExtension(Filename, true);
	if (Extension.Equals(".uasset", ESearchCase::IgnoreCase) || Extension.Equals(".umap", ESearchCase::IgnoreCase))
	{
		FString PackageName;
		FPackageName::TryConvertFilenameToLongPackageName(Filename, PackageName);
		UEditorAssetSubsystem* EditorAssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
		if(EditorAssetSubsystem->DeleteAsset(PackageName))
		{
			return;
		}
	}

	// delete
	bool bWasDeleted = IFileManager::Get().Delete(*Filename);
	if (!bWasDeleted) 
	{
		//todo: log error
	}
}

UTransactionStepCreateFile* UTransactionStepCreateFile::Make(FString InFilename)
{
	auto NewCreateFile = NewObject<UTransactionStepCreateFile>();
	if (NewCreateFile == nullptr)
	{
		return nullptr;
	}

	NewCreateFile->Filename = InFilename;
	return NewCreateFile;
}

void UTransactionStepCreatePlugin::Revert() 
{
	//Unload plugin
	FText UpdateDescriptorFailReason;
	auto Descriptor = Plugin->GetDescriptor();
	Descriptor.bExplicitlyLoaded = true;
	bool bSuccessfulUpdate = Plugin->UpdateDescriptor(Descriptor, UpdateDescriptorFailReason);
	if (!bSuccessfulUpdate)
	{
		//todo: log error
		return;
	}

	bool bSuccessfulUnload = FPluginUtils::UnloadPlugin(Plugin.ToSharedRef());
	if (!bSuccessfulUnload)
	{
		//todo: log error
		return;
	}

	//delete directory
	FString BaseDir = Plugin->GetBaseDir();
	bool bWasDeleted = IFileManager::Get().DeleteDirectory(*(Plugin->GetBaseDir()), false, true);
	if (!bWasDeleted)
	{
		// todo: log error
		return;
	}

	//todo: log success
}

UTransactionStepCreatePlugin* UTransactionStepCreatePlugin::Make(TSharedPtr<class IPlugin> InPlugin)
{
	auto NewCreatePlugin = NewObject<UTransactionStepCreatePlugin>();
	if (NewCreatePlugin == nullptr)
	{
		return nullptr;
	}

	NewCreatePlugin->Plugin = InPlugin;
	return NewCreatePlugin;
}

void UTransactionStepRenameFile::Revert()
{
	auto& FileManager = IFileManager::Get();
	bool bSuccess = FileManager.Move(*OldName, *NewName);
	if (!bSuccess)
	{
		//todo: log error
	}
}

UTransactionStepRenameFile* UTransactionStepRenameFile::Make(FString InOldName, FString InNewName)
{
	auto NewRenameFile = NewObject<UTransactionStepRenameFile>();
	if (NewRenameFile == nullptr)
	{
		return nullptr;
	}

	NewRenameFile->OldName = InOldName;
	NewRenameFile->NewName = InNewName;
	return NewRenameFile;
}
