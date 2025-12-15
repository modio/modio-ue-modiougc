/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "Commandlets/ModioUGCBaseCommandlet.h"

void UModioUGCBaseCommandlet::ParseParameters(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> ParamMap;
	ParseCommandLine(*Params, Tokens, Switches, ParamMap);

	OutputDir = ParamMap.FindRef(TEXT("OutputDir"));
	PluginName = ParamMap.FindRef(TEXT("PluginName"));
	AdditionalArgs = ParamMap.FindRef(TEXT("Args"));
}