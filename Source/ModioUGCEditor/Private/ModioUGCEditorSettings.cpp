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

#include "ModioEditorSettings.h"
#include "Objects/ModioOpenWeblinkAction.h"
#include "Types/ModioToolWindowEntry.h"
#include "Widgets/SModioEditorCreateUGCWidget.h"
#include "Widgets/SModioEditorPackageUGCWidget.h"

#define LOCTEXT_NAMESPACE "FModioUGCEditorSettings"

UModioUGCEditorSettings::UModioUGCEditorSettings(const FObjectInitializer& Initializer) : Super(Initializer)
{
	// Maybe move this to some other class for neatness, in case we need to add more later.
	GettingStartedEntries.Add(FModioGettingStartedEntry(
		true, LOCTEXT("ModioUGCEntryName", "Modio UGC"),
		LOCTEXT("ModioUGCEntryDescription",
				"Framework for adding dynamic user generated content management in the form of PAK files."),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath("/Modio/GettingStarted/T_modbrowser_D.T_modbrowser_D")),
		UModioOpenWeblinkAction::StaticClass(), "https://github.com/modio/modio-ue-modiougc"));

	UModioEditorSettings* Settings = GetMutableDefault<UModioEditorSettings>();
	Settings->SubmoduleToolLandingEntries.Add(FModioToolWindowEntry(
		true, LOCTEXT("CreateUGCEntryName", "Create [UGC_NAME]"),
		LOCTEXT("DocumentationEntryDescription",
				"Create a regular blank plugin to be used for [UGC_NAME], that can only contain content."),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath("/ModioUGC/UI/Materials/MI_GC_Docs.MI_GC_Docs")),
		FModioToolWindowEntry::CreateSubwindow::CreateLambda([](SModioEditorWindowCompoundWidget* Widget) {
			return SNew(SModioEditorCreateUGCWidget).ParentWindow(Widget);
		})));

	Settings->SubmoduleToolLandingEntries.Add(FModioToolWindowEntry(
		true, LOCTEXT("PackageUGCEntryName", "Package [UGC_NAME]"),
		LOCTEXT("DocumentationEntryDescription",
				"Package your Plugin as [UGC_NAME], ready for uploading to the mod.io servers."),
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath("/Modio/GettingStarted/T_modbrowser_D.T_modbrowser_D")),
		FModioToolWindowEntry::CreateSubwindow::CreateLambda([](SModioEditorWindowCompoundWidget* Widget) {
			return SNew(SModioEditorPackageUGCWidget).ParentWindow(Widget);
		})));
}

#undef LOCTEXT_NAMESPACE