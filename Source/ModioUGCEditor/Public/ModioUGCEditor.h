/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IPluginManager.h"
#include "ModioUGCPackager.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Slate.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"

class FToolBarBuilder;
class FMenuBuilder;

DECLARE_LOG_CATEGORY_EXTERN(ModioUGCEditor, All, All);

class FModioUGCEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	//~ Begin UGC Packager and Creator UI

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	//~ End UGC Packager and Creator UI

public:
	//~ Begin Pak File Override

	/** Registers the pak file override. Listens to PIE/SIE events to refresh the override */
	void RegisterPakFileOverride();

	/** Unregisters the pak file override. Unbinds PIE/SIE events and resets the override */
	void UnregisterPakFileOverride();

	/** Refreshes the pak file override */
	void RefreshPakFileOverride();

	void TogglePakFileOverride(bool bEnable);

	TSharedPtr<class FScopedPlatformPakFileOverride> PlatformPakFileOverride;

	FDelegateHandle OnSwitchBeginPIEAndSIE_Handle;

	bool bEngineExitRequested = false;

	//~ End Pak File Override

	void RegisterSettings();
	void UnregisterSettings();

	void RegisterClassLayoutCustomisation(FName ClassName, FOnGetDetailCustomizationInstance DetailLayoutDelegate);

	void RegisterMenus();
	void RegisterUGCTemplateMenus();

	void OnCreateUGCTemplateModMenuButtonClicked();
	void OnAddUGCTemplateItemMenuButtonClicked();
	void OnExportUGCTemplateMenuButtonClicked();

	void OnAddUGCTemplateItemFromContentBrower();
	void OnExportUGCTemplateFromContentBrower();

	bool CreateTemplateWindow(EUGCTemplateType Mode, TSharedPtr<IPlugin> Context = nullptr);
	bool CreateExportWindow(TSharedPtr<IPlugin> Context = nullptr);

	void DismissUGCTemplateWindow();

private:

	TSharedPtr<class SWindow> UGCTemplateWindow = nullptr;
	TSharedPtr<class SModioEditorUGCTemplateWidget> TemplateWidget = nullptr;
	TSharedPtr<class SModioExportUGCTemplateWidget> ExportWidget = nullptr;

	TSet<FName> RegisteredClassNames;

	static FDelayedAutoRegisterHelper ModdingContextMenuRegister;

	static TSharedPtr<IPlugin> GetSelectedPlugin();
	static TSharedPtr<IPlugin> GetPluginFromPath(const FString& PluginPath);

	static const FText CreateModLabel;
	static const FText CreateModTooltip;
	static const FText AddItemLabel;
	static const FText AddItemTooltip;
	static const FText ExportTemplateLabel;
	static const FText ExportTemplateTooltip;
	static const FText ModdingSectionLabel;
};
