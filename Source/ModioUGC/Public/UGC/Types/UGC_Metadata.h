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
#include "Engine/DataAsset.h"
#include "ModioUGC.h"

#include "UGC_Metadata.generated.h"

struct FPrimaryAssetTypeInfo;
class FEngineVersion;

/**
 * Metadata asset to be included in every UGC plugin
 */

UCLASS()
class MODIOUGC_API UUGC_Metadata : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/*
	 * List of asset types to scan at startup
	 */
	UPROPERTY(EditAnywhere, Category = "mod.io|UGCMetadata", meta = (TitleProperty = "PrimaryAssetType"))
	TArray<FPrimaryAssetTypeInfo> PrimaryAssetTypesToScan;

	/*
	 * The version of Unreal Engine this UGC was used to cooked with
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCMetadata", meta = (TitleProperty = "UnrealVersion"))
	FString UnrealVersion;

	/*
	 * Whether IoStore was enabled during the cooking process
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCMetadata", meta = (TitleProperty = "IoStore"))
	bool bIoStoreEnabled;

	static FString GetDefaultAssetName();

	void DebugLogValues() const;
};