/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCCreator.h"

#include "IPluginBrowser.h"
#include "Misc/MessageDialog.h"
#include "ModioUGCPluginWizardDefinition.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "FModioUGCCreator"

const FName FModioUGCCreator::ModioUGCEditorPluginCreatorName("ModioUGCPluginCreator");

FModioUGCCreator::FModioUGCCreator()
{
	RegisterTabSpawner();
}

FModioUGCCreator::~FModioUGCCreator()
{
	UnregisterTabSpawner();
}

void FModioUGCCreator::OpenNewPluginWizard(bool bSuppressErrors) const
{
	if (IPluginBrowser::IsAvailable())
	{
		FGlobalTabmanager::Get()->TryInvokeTab(ModioUGCEditorPluginCreatorName);
	}
	else if (!bSuppressErrors)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			LOCTEXT("PluginBrowserDisabled",
					"Creating UGC requires the use of the Plugin Browser, but it is currently disabled."));
	}
}

void FModioUGCCreator::RegisterTabSpawner()
{
	FTabSpawnerEntry& Spawner = FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		ModioUGCEditorPluginCreatorName, FOnSpawnTab::CreateRaw(this, &FModioUGCCreator::HandleSpawnPluginTab));

	// Set a default size for this tab
	FVector2D DefaultSize(800.0f, 500.0f);
	FTabManager::RegisterDefaultTabWindowSize(ModioUGCEditorPluginCreatorName, DefaultSize);

	Spawner.SetDisplayName(LOCTEXT("NewUGCTabHeader", "Create New UGC Package"));
	Spawner.SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FModioUGCCreator::UnregisterTabSpawner()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ModioUGCEditorPluginCreatorName);
}

TSharedRef<SDockTab> FModioUGCCreator::HandleSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	check(IPluginBrowser::IsAvailable());
	return IPluginBrowser::Get().SpawnPluginCreatorTab(SpawnTabArgs, MakeShared<FModioUGCPluginWizardDefinition>());
}

#undef LOCTEXT_NAMESPACE
