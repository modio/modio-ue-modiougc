/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGC.h"
#include "CoreMinimal.h"

#define LOCTEXT_NAMESPACE "FModioUGCModule"

DEFINE_LOG_CATEGORY(LogModioUGC)

void FModioUGCModule::StartupModule()
{
	UE_LOG(LogModioUGC, Display, TEXT("ModioUGC module has been loaded"));
}

void FModioUGCModule::ShutdownModule()
{
	UE_LOG(LogModioUGC, Display, TEXT("ModioUGC module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModioUGCModule, ModioUGC)