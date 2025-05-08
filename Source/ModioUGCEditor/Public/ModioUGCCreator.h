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

#include "Templates/SharedPointer.h"

class FModioGameFeaturesPluginWizardDefinition;
class SDockTab;

class FModioUGCCreator : public TSharedFromThis<FModioUGCCreator>
{
public:
	FModioUGCCreator();
	~FModioUGCCreator();

	/**
	 * Opens the mod creator wizard.
	 * @param	bSuppressErrors		If false, a dialog will be shown if the wizard cannot be opened
	 */
	void OpenNewPluginWizard(bool bSuppressErrors = false) const;

	/** The name to use when creating the tab for the tab spawner */
	static const FName ModioUGCEditorPluginCreatorName;

private:
	/** Registers a nomad tab spawner that will create the mod wizard */
	void RegisterTabSpawner();

	/** Unregisters the nomad tab spawner */
	void UnregisterTabSpawner();

	/** Spawns the tab that hosts the mod creator wizard widget */
	TSharedRef<SDockTab> HandleSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
};
