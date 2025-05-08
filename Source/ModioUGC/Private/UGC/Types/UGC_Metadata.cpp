/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/Types/UGC_Metadata.h"

FString UUGC_Metadata::GetDefaultAssetName()
{
	// Adds the prefix to match the asset name
	FString ClassNameForAsset = StaticClass()->GetPrefixCPP() + StaticClass()->GetName();
	// Resulting in "UUGC_Metadata.UUGC_Metadata"
	FString AssetName = FString::Printf(TEXT("%s.%s"), *ClassNameForAsset, *ClassNameForAsset);
	return AssetName;
}

void UUGC_Metadata::DebugLogValues() const
{
	UE_LOG(LogModioUGC, Verbose, TEXT("Reading UGC Metadata:"));
	UE_LOG(LogModioUGC, Verbose, TEXT("		-> IoStore Enabled: %d"), bIoStoreEnabled ? 1 : 0);
	UE_LOG(LogModioUGC, Verbose, TEXT("		-> Unreal Version: %s"), *UnrealVersion);
}
