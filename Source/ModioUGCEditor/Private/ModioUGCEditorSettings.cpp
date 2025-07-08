/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCEditorSettings.h"
#include "Objects/ModioOpenWeblinkAction.h"

#define LOCTEXT_NAMESPACE "FModioUGCEditorSettings"

UModioUGCEditorSettings::UModioUGCEditorSettings(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
	//Maybe move this to some other class for neatness, in case we need to add more later.
	GettingStartedEntries.Add(FModioGettingStartedEntry(
		true, LOCTEXT("ModioUGCEntryName", "Modio UGC"),
		LOCTEXT("ModioUGCEntryDescription", "Framework for adding dynamic user generated content management in the form of PAK files."),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath("/Modio/GettingStarted/T_modbrowser_D.T_modbrowser_D")),
		UModioOpenWeblinkAction::StaticClass(), "https://github.com/modio/modio-ue-modiougc"));
}

#undef LOCTEXT_NAMESPACE