/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCCommandlet.h"

DEFINE_LOG_CATEGORY(ModioUGCCommandlet);

#define LOCTEXT_NAMESPACE "FModioUGCCommandletModule"

void FModioUGCCommandletModule::StartupModule()
{
	UE_LOG(ModioUGCCommandlet, Display, TEXT("Modio UGC Commandlet module has been loaded in mode: %s"),
		   FCommandLine::Get());
}

void FModioUGCCommandletModule::ShutdownModule()
{
	UE_LOG(ModioUGCCommandlet, Display, TEXT("Modio UGC Commandlet module has been unloaded."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModioUGCCommandletModule, ModioUGCCommandlet)
