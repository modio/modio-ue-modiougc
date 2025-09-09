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
#include "ModioUGCPackager.h"
#include "Modules/ModuleManager.h"
#include "Slate.h"

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
};
