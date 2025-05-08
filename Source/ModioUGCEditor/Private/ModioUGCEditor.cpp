/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCEditor.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPlatformFilePak.h"
#include "LevelEditor.h"
#include "Misc/MessageDialog.h"
#include "ModioUGCCreator.h"
#include "ModioUGCEditorCommands.h"
#include "ModioUGCEditorStyle.h"
#include "ModioUGCSettings.h"

#include "UGC/Types/UGCPackage.h"
#include "UGC/UGCSubsystem.h"

DEFINE_LOG_CATEGORY(ModioUGCEditor);

#define LOCTEXT_NAMESPACE "FModioUGCEditorModule"

void FModioUGCEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin
	// file per-module

	UGCCreator = MakeShared<FModioUGCCreator>();
	UGCPackager = MakeShared<FModioUGCPackager>();

	FModioUGCEditorStyle::Initialize();
	FModioUGCEditorStyle::ReloadTextures();

	FModioUGCEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(FModioUGCEditorCommands::Get().CreateUGCAction,
							  FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::CreateUGCButtonClicked),
							  FCanExecuteAction());

	PluginCommands->MapAction(FModioUGCEditorCommands::Get().PackageUGCAction,
							  FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::PackageUGCButtonClicked),
							  FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	// Add commands
	{
		FName MenuSection = "FileProject";
		FName ToolbarSection = "Content";

		// Add creator button to the menu
		{
			TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddMenuExtension(
				MenuSection, EExtensionHook::After, PluginCommands,
				FMenuExtensionDelegate::CreateRaw(this, &FModioUGCEditorModule::AddUGCCreatorMenuExtension));

			LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
		}

		// Add creator button to the toolbar
		{
			TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
			ToolbarExtender->AddToolBarExtension(
				ToolbarSection, EExtensionHook::After, PluginCommands,
				FToolBarExtensionDelegate::CreateRaw(this, &FModioUGCEditorModule::AddUGCCreatorToolbarExtension));

			LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		}

		// Add packager button to the menu
		{
			TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
			MenuExtender->AddMenuExtension(
				MenuSection, EExtensionHook::After, PluginCommands,
				FMenuExtensionDelegate::CreateRaw(this, &FModioUGCEditorModule::AddUGCPackagerMenuExtension));

			LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
		}

		// Add packager button to the toolbar
		{
			TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
			ToolbarExtender->AddToolBarExtension(
				ToolbarSection, EExtensionHook::After, PluginCommands,
				FToolBarExtensionDelegate::CreateRaw(this, &FModioUGCEditorModule::AddUGCPackagerToolbarExtension));

			LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		}
	}

	RegisterPakFileOverride();

	const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
	if (UGCSettings && UGCSettings->bEnableUGCProviderInEditor)
	{
		// Enable unversioned content loading in editor to support loading assets from pak files containing unversioned
		// assets Setting GAllowUnversionedContentInEditor to 0 occurs by default when bEnableUGCProviderInEditor is
		// disabled and editor restarted
		GAllowUnversionedContentInEditor = 1;

		// Enable cooked data loading in editor to support loading assets from pak files containing cooked assets
		// Setting GAllowCookedDataInEditorBuilds reverts to bAllowCookedDataInEditorBuilds config value when
		// bEnableUGCProviderInEditor is disabled and editor restarted
		GAllowCookedDataInEditorBuilds = 1;
	}

	UE_LOG(ModioUGCEditor, Display, TEXT("mod.io UGC Editor module loaded."));
}

void FModioUGCEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FModioUGCEditorStyle::Shutdown();

	FModioUGCEditorCommands::Unregister();

	UnregisterPakFileOverride();

	UE_LOG(ModioUGCEditor, Display, TEXT("mod.io UGC Editor module unloaded."));
}

void FModioUGCEditorModule::CreateUGCButtonClicked()
{
	if (UGCCreator.IsValid())
	{
		UGCCreator->OpenNewPluginWizard();
	}
}

void FModioUGCEditorModule::PackageUGCButtonClicked()
{
	if (UGCPackager.IsValid())
	{
		UGCPackager->ShowPackagerWindow();
	}
}

void FModioUGCEditorModule::AddUGCCreatorToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FModioUGCEditorCommands::Get().CreateUGCAction);
}

void FModioUGCEditorModule::AddUGCCreatorMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FModioUGCEditorCommands::Get().CreateUGCAction);
}

void FModioUGCEditorModule::AddUGCPackagerToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FModioUGCEditorCommands::Get().PackageUGCAction);
}

void FModioUGCEditorModule::AddUGCPackagerMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FModioUGCEditorCommands::Get().PackageUGCAction);
}

void FModioUGCEditorModule::RegisterPakFileOverride()
{
	UnregisterPakFileOverride();

	FCoreDelegates::OnEnginePreExit.AddLambda([this]() { bEngineExitRequested = true; });

	// Ensure the pak platform file is persistent during the module's lifespan in the editor, and only within PIE/SIE
	// sessions. This is required as the pak platform file isn't the default in the editor unless "UsePaks" argument is
	// passed. See FPakPlatformFile::ShouldBeUsed.
	FEditorDelegates::PreBeginPIE.AddLambda([this](bool bIsSimulating) { TogglePakFileOverride(true); });

	// Unmount all UGC packages on exiting PIE to prevent issues due to the pak platform file being the current platform
	// file. Issues include: 1) Inability to load assets of UGC created in previous session upon editor reopening 2)
	// Failure of the cooker to find assets when packaging UGC, resulting in an empty pak file 3) Random crashes when
	// working with mods in the editor
	FEditorDelegates::EndPIE.AddLambda([this](bool bIsSimulating) { TogglePakFileOverride(false); });
	RefreshPakFileOverride();
}

void FModioUGCEditorModule::UnregisterPakFileOverride()
{
	TogglePakFileOverride(false);
	FEditorDelegates::OnSwitchBeginPIEAndSIE.Remove(OnSwitchBeginPIEAndSIE_Handle);
	OnSwitchBeginPIEAndSIE_Handle.Reset();
}

void FModioUGCEditorModule::RefreshPakFileOverride()
{
	const bool bIsPlaying = (GWorld && !GWorld->IsEditorWorld()) ||
							(GEditor && GEditor->PlayWorld && !GEditor->IsSimulateInEditorInProgress());

	TogglePakFileOverride(bIsPlaying);
}

void FModioUGCEditorModule::TogglePakFileOverride(bool bEnable)
{
	if (bEnable)
	{
		FPakPlatformFile* PakPlatformFile = static_cast<FPakPlatformFile*>(
			FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName()));
		if (!PakPlatformFile ||
			!PakPlatformFile->ShouldBeUsed(&FPlatformFileManager::Get().GetPlatformFile(), FCommandLine::Get()))
		{
			PlatformPakFileOverride = MakeShared<FScopedPlatformPakFileOverride>();
			UE_LOG(ModioUGCEditor, Warning,
				   TEXT("PakPlatformFile is not being used. Overriding to use PakPlatformFile to correctly load UGC "
						"assets from paks in the editor"));
		}
		else
		{
			UE_LOG(ModioUGCEditor, Display, TEXT("PakPlatformFile is being used. No need to override"));
		}
	}
	else
	{
		if (!bEngineExitRequested)
		{
			if (UUGCSubsystem* UGCSubsystem = GEngine->GetEngineSubsystem<UUGCSubsystem>())
			{
				TArray<FUGCPackage> Packages;
				UGCSubsystem->EnumerateAllUGCPackages([&Packages](const FUGCPackage& Package) {
					Packages.Add(Package);
					return true;
				});

				for (FUGCPackage& Package : Packages)
				{
					UGCSubsystem->UnmountUGCPackage(Package, true);
				}
			}
		}
		PlatformPakFileOverride.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModioUGCEditorModule, ModioUGCEditor)
