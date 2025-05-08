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
#include "Templates/UnrealTemplate.h"
#include "Types/GenericModID.h"
#include "UObject/Interface.h"

#include "UGCProvider.generated.h"

/**
 * Struct to represent a map of UGC paths to their associated mod IDs
 */
USTRUCT(BlueprintType, Category = "mod.io|UGC")
struct MODIOUGC_API FModUGCPathMap
{
	GENERATED_BODY()

	virtual ~FModUGCPathMap() = default;
	FModUGCPathMap() = default;

	FModUGCPathMap(TMap<FString, FGenericModID>&& InUGCPaths) : PathToModIDMap(MoveTemp(InUGCPaths)) {}

	FModUGCPathMap(const TMap<FString, FGenericModID>& InUGCPaths) : PathToModIDMap(InUGCPaths) {}

	bool operator==(const FModUGCPathMap& Other) const
	{
		return PathToModIDMap.OrderIndependentCompareEqual(Other.PathToModIDMap);
	}

	bool operator!=(const FModUGCPathMap& Other) const
	{
		return !(*this == Other);
	}

	FModUGCPathMap operator+(const FModUGCPathMap& Other) const
	{
		FModUGCPathMap OutMap;
		OutMap.PathToModIDMap = PathToModIDMap;
		OutMap.PathToModIDMap.Append(Other.PathToModIDMap);

		return OutMap;
	}

	FModUGCPathMap operator+=(const FModUGCPathMap& Other)
	{
		PathToModIDMap.Append(Other.PathToModIDMap);

		return *this;
	}

	friend uint32 GetTypeHash(const FModUGCPathMap& Other)
	{
		uint32 Hash = 0;
		for (const auto& Pair : Other.PathToModIDMap)
		{
			Hash = HashCombine(Hash, HashCombine(GetTypeHash(Pair.Key), GetTypeHash(Pair.Value)));
		}
		return Hash;
	}

	/**
	 * Map type for UGC paths and their associated mod IDs
	 * Mod IDs are optional and may be empty
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "mod.io|UGC")
	TMap<FString, FGenericModID> PathToModIDMap;
};

// Delegate for UGC provider initialization result
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnUGCProviderInitializedDelegate, bool, bSuccess);

// Delegate for UGC provider deinitialization result
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnUGCProviderDeinitializedDelegate, bool, bSuccess);

UINTERFACE(Blueprintable)
class MODIOUGC_API UUGCProvider : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for interacting with UGC providers.
 */
class MODIOUGC_API IUGCProvider
{
	GENERATED_BODY()

public:
	/**
	 * Initializes the UGC provider and scans for available UGC.
	 * @param Handler Callback for initialization success.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "mod.io|UGC|Provider")
	void InitializeProvider(const FOnUGCProviderInitializedDelegate& Handler);

	/**
	 * Deinitializes the UGC provider.
	 * @param Handler Callback for deinitialization success.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "mod.io|UGC|Provider")
	void DeinitializeProvider(const FOnUGCProviderDeinitializedDelegate& Handler);

	/**
	 * Indicates whether the UGC provider is enabled
	 * @return bool indicating whether the UGC provider is enabled
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta = (DisplayName = "Is Provider Enabled"),
			  Category = "mod.io|UGC|Provider")
	bool IsProviderEnabled();

	/**
	 * Gets a list of installed UGC paths.
	 * @return A map of installed UGC paths to their IDs.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, meta = (DisplayName = "Get Installed UGC Paths"),
			  Category = "mod.io|UGC|Provider")
	FModUGCPathMap GetInstalledUGCPaths();
};