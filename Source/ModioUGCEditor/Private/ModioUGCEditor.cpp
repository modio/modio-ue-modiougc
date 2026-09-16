/*
 *  Copyright (C) 2025-2026 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCEditor.h"
#include "Runtime/Launch/Resources/Version.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "IPlatformFilePak.h"
#include "LevelEditor.h"
#include "Misc/AutomationTest.h"
#include "Misc/MessageDialog.h"
#include "ModioEditorUtilityFunctions.h"
#include "ModioUGCEditorSettings.h"
#include "ModioUGCEditorStyle.h"
#include "ModioUGCSettings.h"
#include "UGC/Types/UGCPackage.h"
#include "UGC/UGCSubsystem.h"
#include "UGCTemplates/Widgets/SModioEditorUGCTemplateWidget.h"
#include "Developer/Settings/Public/ISettingsModule.h"
#include "UGCTemplates/UGCTemplateSettings.h"
#include "UGCTemplates/Widgets/SModioExportUGCTemplateWidget.h"
#include "UGCTemplates/UGCTemplateExportCustomisation.h"
#include "Editor/ContentBrowser/Public/ContentBrowserMenuContexts.h"
#include "Editor/ContentBrowser/Private/SContentBrowser.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Interfaces/IPluginManager.h"
#include "PropertyEditorModule.h"

DEFINE_LOG_CATEGORY(ModioUGCEditor);

#define LOCTEXT_NAMESPACE "FModioUGCEditorModule"

const FText FModioUGCEditorModule::CreateModLabel			= LOCTEXT("UGCTemplateCreateModMenuLabel", "New Mod from Template");
const FText FModioUGCEditorModule::CreateModTooltip			= LOCTEXT("UGCTemplateCreteModMenuTooltip", "Create a new mod from a template");
const FText FModioUGCEditorModule::AddItemLabel				= LOCTEXT("UGCTemplateAddItemMenuLabel", "Add Item");
const FText FModioUGCEditorModule::AddItemTooltip			= LOCTEXT("UGCTemplateAddItemMenuTooltip", "Add a template item to UGC");
const FText FModioUGCEditorModule::ExportTemplateLabel		= LOCTEXT("UGCTemplateExportMenuLabel", "Export Template to Mod");
const FText FModioUGCEditorModule::ExportTemplateTooltip	= LOCTEXT("UGCTemplateExportMenuTooltip", "Export an existing plugin as a new UGC Template");
const FText FModioUGCEditorModule::ModdingSectionLabel		= LOCTEXT("UGCTemplateModdingSectionLabel", "Modding");

void FModioUGCEditorModule::StartupModule()
{
	UE_LOG(LogTemp, Display, TEXT("ModioUGCEditor module has been loaded in mode: %s"), FCommandLine::Get());

	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin
	// file per-module
	FModioUGCEditorStyle::Initialize();
	FModioUGCEditorStyle::ReloadTextures();

	RegisterPakFileOverride();

	RegisterSettings();

	const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
	if (UGCSettings && UGCSettings->bEnableUGCProviderInEditor)
	{
		// Enable unversioned content loading in editor to support loading assets from pak files containing unversioned
		// assets Setting GAllowUnversionedContentInEditor to 0 occurs by default when bEnableUGCProviderInEditor is
		// disabled and editor restarted
		// Equivalent to DefaultEngine.ini setting:
		// [/Script/UnrealEd.CookerSettings]
		// s.AllowUnversionedContentInEditor = True
		GAllowUnversionedContentInEditor = 1;

		// Enable cooked data loading in editor to support loading assets from pak files containing cooked assets
		// Setting GAllowCookedDataInEditorBuilds reverts to bAllowCookedDataInEditorBuilds config value when
		// bEnableUGCProviderInEditor is disabled and editor restarted
		// Equivalent to DefaultEngine.ini setting:
		// [/Script/UnrealEd.CookerSettings]
		// cook.AllowCookedDataInEditorBuilds = True
		GAllowCookedDataInEditorBuilds = 1;
	}

	UModioEditorUtilityFunctions::AddGettingStartedWidgetEntries(
		GetMutableDefault<UModioUGCEditorSettings>()->GettingStartedEntries);

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FModioUGCEditorModule::RegisterMenus));

	FModioEditorUGCTemplateWidgetCommands::Register();
}

void FModioUGCEditorModule::ShutdownModule()
{
	UE_LOG(ModioUGCEditor, Display, TEXT("mod.io UGC Editor module unloaded."));

	//Unregister any details customisations
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

		// Unregister all classes customized by name
		for (auto It = RegisteredClassNames.CreateConstIterator(); It; ++It)
		{
			if (It->IsValid())
			{
				PropertyModule.UnregisterCustomClassLayout(*It);
			}
		}
	}

	UnregisterSettings();

	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FModioUGCEditorStyle::Shutdown();

	UnregisterPakFileOverride();
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
			// Skip unmounting during automation tests — the test manages its own UGC lifecycle
			// and unmounting here while PIE objects are still alive causes assertion failures.
			const bool bIsAutomationRunning = FAutomationTestFramework::GetInstance().GetCurrentTest() != nullptr;
			if (!bIsAutomationRunning)
			{
				if (UUGCSubsystem* UGCSubsystem = GEngine->GetEngineSubsystem<UUGCSubsystem>())
				{
					UGCSubsystem->UnloadAllUGCPackages();
				}
			}
		}
		PlatformPakFileOverride.Reset();
	}
}

void FModioUGCEditorModule::RegisterSettings()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "mod.io UGC Templates", LOCTEXT("UGCTemplateSettingsName", "mod.io UGC Templates"),
										 LOCTEXT("UGCTemplateSettingsDescription", "Configure the mod.io UGC Template settings"),
										 GetMutableDefault<UUGCTemplateSettings>());
	}
#endif
}

void FModioUGCEditorModule::UnregisterSettings()
{
#if WITH_EDITOR
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "mod.io UGC Templates");
	}
#endif // WITH_EDITOR
}

void FModioUGCEditorModule::RegisterClassLayoutCustomisation(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate)
{
	check(ClassName != NAME_None);

	RegisteredClassNames.Add(ClassName);

	static FName PropertyEditor("PropertyEditor");
	FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(PropertyEditor);
	PropertyModule.RegisterCustomClassLayout(ClassName, DetailLayoutDelegate);
}

void FModioUGCEditorModule::RegisterMenus()
{
	UUGCTemplateSettings* TemplateSettings = GetMutableDefault<UUGCTemplateSettings>();
	if (TemplateSettings != nullptr && TemplateSettings->bDisplayUGCTemplateMenus)
	{
		RegisterUGCTemplateMenus();
	}
}

void FModioUGCEditorModule::RegisterUGCTemplateMenus()
{
	RegisterClassLayoutCustomisation("UGCTemplateExportDetails", FOnGetDetailCustomizationInstance::CreateStatic(&FUGCTemplateExportCustomisation::MakeInstance));

	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			//Section.AddMenuEntryWithCommandList(FModioEditorWindowCommands::Get().OpenPluginWindow, PluginCommands);
		}
	}

	//Toolbar extension
	{
#if ENGINE_MAJOR_VERSION >= 5
		FName ToolBarName = "LevelEditor.LevelEditorToolBar.PlayToolBar";
		FName ExtensionPoint = "Play";
#else
		FName ToolBarName = "LevelEditor.LevelEditorToolBar";
		FName ExtensionPoint = "Settings";
#endif
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(ToolBarName);
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection(ExtensionPoint);
			{
				FToolMenuEntry& CreateButtonEntry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						"UGCTemplatesCreateModMenuButton",
						FUIAction(FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnCreateUGCTemplateModMenuButtonClicked)),
						CreateModLabel, 
						CreateModTooltip,
						FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.CreateUGCToolbarIcon")
						)
					);

				FToolMenuEntry& AddButtonEntry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						"UGCTemplatesAddItemMenuButton",
						FUIAction(FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnAddUGCTemplateItemMenuButtonClicked)),
						AddItemLabel,
						AddItemTooltip,
						FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.AddItemToolbarIcon")
					)
				);

				FToolMenuEntry& ExportButtonEntry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton(
						"UGCTemplatesExportTemplateMenuButton",
						FUIAction(FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnExportUGCTemplateMenuButtonClicked)),
						ExportTemplateLabel, 
						ExportTemplateTooltip,
						FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.ExportTemplateToolbarIcon")
					)
				);
			}
		}
	}

	//File menu extension
	{
		FToolMenuInsert InsertPos("Exit", EToolMenuInsertType::Before);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.File");
		FToolMenuSection& ModdingSection = Menu->FindOrAddSection("Modding", ModdingSectionLabel);
		ModdingSection.InsertPosition = InsertPos;
		{
			FToolMenuEntry& CreateFileMenuEntry = ModdingSection.AddEntry(
				FToolMenuEntry::InitMenuEntry(
					"UGCTemplatesCreateModFileMenuButton",
					CreateModLabel,
					CreateModTooltip,
					FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.CreateUGCMenuIcon"),
					FUIAction(
						FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnCreateUGCTemplateModMenuButtonClicked))
				)
			);

			FToolMenuEntry& ExportFileMenuEntry = ModdingSection.AddEntry(
				FToolMenuEntry::InitMenuEntry(
					"UGCTemplatesExportTemplateFileMenuButton",
					ExportTemplateLabel,
					ExportTemplateTooltip,
					FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.ExportTemplateMenuIcon"),
					FUIAction(
						FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnExportUGCTemplateMenuButtonClicked))
				)
			);
		}
	}

	//Content browser extensions
	{
		FToolMenuInsert InsertPos("ContentBrowserNewAsset", EToolMenuInsertType::Before);
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AddNewContextMenu");

		FToolMenuSection& ModdingSection = Menu->FindOrAddSection("Modding", ModdingSectionLabel);
		ModdingSection.InsertPosition = InsertPos;

		FToolMenuEntry& AddItemMenuEntry = ModdingSection.AddEntry(
			FToolMenuEntry::InitMenuEntry(
				"UGCTemplatesAddItemContextMenuButton",
				AddItemLabel,
				AddItemTooltip,
				FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.AddItemMenuIcon"),
				FUIAction(
					FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnAddUGCTemplateItemFromContentBrower))
			)
		);

		FToolMenuEntry& ExportModMenuEntry = ModdingSection.AddEntry(
			FToolMenuEntry::InitMenuEntry(
				"UGCTemplatesExportTemplateContextMenuButton",
				ExportTemplateLabel,
				ExportTemplateTooltip,
				FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.ExportTemplateMenuIcon"),
				FUIAction(
					FExecuteAction::CreateRaw(this, &FModioUGCEditorModule::OnExportUGCTemplateFromContentBrower))
			)
		);
	}
}

void FModioUGCEditorModule::OnCreateUGCTemplateModMenuButtonClicked() 
{
	CreateTemplateWindow(EUGCTemplateType::TT_Mod);
}

void FModioUGCEditorModule::OnAddUGCTemplateItemMenuButtonClicked()
{
	CreateTemplateWindow(EUGCTemplateType::TT_Item);
}

void FModioUGCEditorModule::OnExportUGCTemplateMenuButtonClicked()
{
	CreateExportWindow();
}

void FModioUGCEditorModule::OnAddUGCTemplateItemFromContentBrower() 
{
	CreateTemplateWindow(EUGCTemplateType::TT_Item, GetSelectedPlugin());
}

void FModioUGCEditorModule::OnExportUGCTemplateFromContentBrower()
{
	CreateExportWindow(GetSelectedPlugin());
}

FDelayedAutoRegisterHelper FModioUGCEditorModule::ModdingContextMenuRegister(EDelayedRegisterRunPhase::EndOfEngineInit, 
	[]
	{
		FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);

		static const FName FolderContextMenuName("ContentBrowser.FolderContextMenu");
		UToolMenu* Menu = UToolMenus::Get()->FindMenu(FolderContextMenuName);

		Menu->bCloseSelfOnly = true;

		Menu->AddDynamicSection("Modding", 
			FNewToolMenuDelegate::CreateLambda([](UToolMenu* InMenu)
			{
				UContentBrowserFolderContext* Context = InMenu->FindContext<UContentBrowserFolderContext>();
				if (!Context || !Context->ContentBrowser.IsValid())
				{
					return;
				}

				//Check if we have one plugin selected and that it is a UGC plugin
				TSharedPtr<IPlugin> Plugin = GetSelectedPlugin();
				if(Plugin == nullptr || !Plugin->GetDescriptor().Category.Contains("UGC")) //TODO: take into account other possible values
				{
					return;
				}

				FToolMenuInsert InsertPos("PathViewFolderOptions", EToolMenuInsertType::Before);
				FToolMenuSection& ModdingSection = InMenu->AddSection(NAME_Default, ModdingSectionLabel);
				ModdingSection.InsertPosition = InsertPos;

				ModdingSection.AddMenuEntry(
					"UGCTemplatesAddItemFileMenuButton", 
					AddItemLabel,
					AddItemTooltip,
					FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.AddItemMenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([Plugin]() 
						{
							FModioUGCEditorModule& ModioUGCEditorModule = FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
							ModioUGCEditorModule.CreateTemplateWindow(EUGCTemplateType::TT_Item, Plugin);
					})));

				ModdingSection.AddMenuEntry(
					"UGCTemplatesExportTemplateFileMenuButton",
					ExportTemplateLabel,
					ExportTemplateTooltip,
					FSlateIcon(FModioUGCEditorStyle::GetStyleSetName(), "ModioUGCEditor.ExportTemplateMenuIcon"),
					FUIAction(FExecuteAction::CreateLambda([Plugin]() {
						FModioUGCEditorModule& ModioUGCEditorModule =
							FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
						ModioUGCEditorModule.CreateExportWindow(Plugin);
					})));
				
			}));
	}
);

TSharedPtr<IPlugin> FModioUGCEditorModule::GetSelectedPlugin()
{
	IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
	TArray<FString> SelectedFolders;
	ContentBrowser.GetSelectedPathViewFolders(SelectedFolders);

	if (SelectedFolders.Num() != 1)
	{
		return nullptr;
	}

	FString Path = SelectedFolders[0];
	return GetPluginFromPath(Path);
}

TSharedPtr<IPlugin> FModioUGCEditorModule::GetPluginFromPath(const FString& PluginPath)
{
	TArray<FString> PathPieces;
	PluginPath.ParseIntoArray(PathPieces, TEXT("/"));
	int32 PluginIndex = PathPieces.Find("Plugins");
	if (PluginIndex == INDEX_NONE)
	{
		return nullptr;
	}

	return IPluginManager::Get().FindPlugin(PathPieces[PluginIndex + 1]);
}

bool FModioUGCEditorModule::CreateTemplateWindow(EUGCTemplateType Mode, TSharedPtr<IPlugin> Context) 
{
	bool bCreatedWindow = false;
	if (!UGCTemplateWindow.IsValid())
	{
		FText WindowTitle = (Mode == EUGCTemplateType::TT_Mod ? LOCTEXT("UGCTemplateWindowTitleMod",  "Add Item")
															  : LOCTEXT("UGCTemplateWindowTitleItem", "Add Item"));
		UGCTemplateWindow =
			SNew(SWindow)
				.Title(WindowTitle)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.HasCloseButton(true)
				.ClientSize(FVector2D(1000.f, 720.f))
				.SizingRule(ESizingRule::FixedSize)
				.AutoCenter(EAutoCenter::PreferredWorkArea)
				.ScreenPosition(FVector2D(0, 0))
				.LayoutBorder(FMargin(3.f))
				[
					SAssignNew(TemplateWidget, SModioEditorUGCTemplateWidget)
					.Mode(Mode)
					.Context(Context)
				];

		UGCTemplateWindow->SetOnWindowClosed(
			FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>& WindowRef) {
				TemplateWidget->TearDown();
				TemplateWidget = nullptr;
				UGCTemplateWindow = nullptr;
			}));
		FSlateApplication::Get().AddWindow(UGCTemplateWindow.ToSharedRef(), false);

		bCreatedWindow = true;
	}
	UGCTemplateWindow->BringToFront(true);
	UGCTemplateWindow->ShowWindow();

	return bCreatedWindow;
}

bool FModioUGCEditorModule::CreateExportWindow(TSharedPtr<IPlugin> Context)
{
	bool bCreatedWindow = false;
	if (!UGCTemplateWindow.IsValid())
	{
		UGCTemplateWindow = 
			SNew(SWindow)
			.Title(LOCTEXT("UGCTemplateExportWindowTitle", "Create Template From Mod"))
			.SupportsMaximize(false)
			.SupportsMinimize(false)
			.HasCloseButton(true)
			.ClientSize(FVector2D(800.f, 720.f))
			.SizingRule(ESizingRule::FixedSize)
			.AutoCenter(EAutoCenter::PreferredWorkArea)
			.ScreenPosition(FVector2D(0, 0))
			.LayoutBorder(FMargin(3.f))
			[
				SAssignNew(ExportWidget, SModioExportUGCTemplateWidget)
				.Context(Context)
			];

		UGCTemplateWindow->SetOnWindowClosed(
			FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>& WindowRef) {
				ExportWidget = nullptr;
				UGCTemplateWindow = nullptr;
			}));
		FSlateApplication::Get().AddWindow(UGCTemplateWindow.ToSharedRef(), false);

		bCreatedWindow = true;
	}
	UGCTemplateWindow->BringToFront(true);
	UGCTemplateWindow->ShowWindow();

	return bCreatedWindow;
}

void FModioUGCEditorModule::DismissUGCTemplateWindow() 
{
	if (UGCTemplateWindow == nullptr)
	{
		return;
	}

	UGCTemplateWindow->RequestDestroyWindow();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModioUGCEditorModule, ModioUGCEditor)
