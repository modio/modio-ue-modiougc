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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "UGC/Types/GenericModID.h"

#include "UGCLibrary.generated.h"

struct FUGCPackage;

/**
 * Function library for UGC-related functions
 */
UCLASS()
class MODIOUGC_API UUGCLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetModID (FUGCPackage)", CompactNodeTitle = "GetModID"),
			  Category = "mod.io|UGC")
	static bool GetModID(const FUGCPackage& UGCPackage, FGenericModID& ModID);

	/** Returns true if GenericModID A is equal to GenericModID B (A == B) */
	UFUNCTION(BlueprintPure,
			  meta = (DisplayName = "Equal (GenericModID)", CompactNodeTitle = "==", ScriptMethod = "Equals",
					  ScriptOperator = "==", Keywords = "== equal"),
			  Category = "mod.io|UGC")
	static bool EqualEqual_GenericModID(const FGenericModID& A, const FGenericModID& B);

	/** Returns true if GenericModID A is not equal to GenericModID B (A != B) */
	UFUNCTION(BlueprintPure,
			  meta = (DisplayName = "Not Equal (GenericModID)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual",
					  ScriptOperator = "!=", Keywords = "!= not equal"),
			  Category = "mod.io|UGC")
	static bool NotEqual_GenericModID(const FGenericModID& A, const FGenericModID& B);
};
