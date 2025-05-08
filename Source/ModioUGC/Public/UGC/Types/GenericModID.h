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

#include "GenericModID.generated.h"

/**
 * Strong type struct to wrap a ModID to uniquely identify user generated content
 **/
USTRUCT(BlueprintType)
struct MODIOUGC_API FGenericModID
{
	GENERATED_BODY()

	/**
	 * Default constructor without parameters
	 **/
	FGenericModID();

	/**
	 * Preferred constructor with ModID initialization parameter
	 * @param InModId Base ModID to create this strong type
	 **/
	constexpr explicit FGenericModID(int64 InModId) : ModID(InModId) {}

	/**
	 * Transform a ModID into its 32 bit integer representation
	 * @return unsigned 32 bit integer that matches the hash of the ModID provided
	 **/
	friend uint32 GetTypeHash(const FGenericModID& ModId);

	bool operator==(const FGenericModID& Other) const
	{
		return ModID == Other.ModID;
	}

	/**
	 * Comparison operator between ModID elements
	 **/
	bool operator!=(const FGenericModID& Other) const
	{
		return ModID != Other.ModID;
	}

	/**
	 * Less than operator between ModID elements
	 **/
	bool operator<(const FGenericModID& Other) const
	{
		return ModID < Other.ModID;
	}

	/**
	 * More than operator between ModID elements
	 **/
	bool operator>(const FGenericModID& Other) const
	{
		return ModID > Other.ModID;
	}

protected:
	/**
	 * UPROPERTY to expose the ModID to Blueprints, specifically for construction
	 */
	UPROPERTY(BlueprintReadWrite, Category = "mod.io|UGC")
	int64 ModID;
};

template<>
struct TStructOpsTypeTraits<FGenericModID> : TStructOpsTypeTraitsBase2<FGenericModID>
{
	enum
	{
		// This is needed to allow the use of the == operator in hashing functions (used in TMap), particularly for UScriptStruct::CompareScriptStruct
		WithIdenticalViaEquality = true,
	};
};