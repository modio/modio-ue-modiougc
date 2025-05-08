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

#include "Delegates/Delegate.h"
#include "UGC/Types/GenericModID.h"
#include "UObject/Interface.h"

#include "IModEnabledStateProvider.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(FModEnabledStateChangeHandler, FGenericModID, RawID, bool, bNewEnabledState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnModEnabledStateChangeMulticastDelegate, FGenericModID, RawID, bool,
											 bNewEnabledState);

/**
 * Interface for querying or setting the enabled or disabled state for a mod
 * Extend this interface if you wish to provide mod priority functionality
 */
UINTERFACE(BlueprintType)
class MODIOUGC_API UModEnabledStateProvider : public UInterface
{
	GENERATED_BODY()
};

class MODIOUGC_API IModEnabledStateProvider : public IInterface
{
	GENERATED_BODY()

protected:
	virtual bool NativeQueryIsModEnabled(FGenericModID ModID)
	{
		return true;
	}

	virtual bool NativeRequestModEnabledStateChange(FGenericModID ID, bool bNewEnabledState)
	{
		return true;
	}

	bool QueryIsModEnabled_Implementation(FGenericModID ModID)
	{
		return NativeQueryIsModEnabled(ModID);
	}
	bool RequestModEnabledStateChange_Implementation(FGenericModID ID, bool bNewEnabledState)
	{
		return NativeRequestModEnabledStateChange(ID, bNewEnabledState);
	}

public:
	/**
	 * Queries if a mod is currently enabled
	 * @param ModID the raw ID for the mod to query
	 * @return true if the mod is enabled, else false
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UGC|Mod Enabled State Provider")
	bool QueryIsModEnabled(FGenericModID ModID);

	/**
	 * Requests that a mod's enabled state be changed
	 * @param ID The raw ID for the mod to enable or disable
	 * @param bNewEnabledState the new state for the mod
	 * @return true if the mod's state was successfully changed, else false
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UGC|Mod Enabled State Provider")
	bool RequestModEnabledStateChange(FGenericModID ID, bool bNewEnabledState);
};
