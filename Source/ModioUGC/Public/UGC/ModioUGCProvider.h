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

#include "UGCProvider.h"
#include "UObject/Object.h"

#include "ModioUGCProvider.generated.h"

/**
 * Mod.io UGC provider implementation.
 */
UCLASS()
class MODIOUGC_API UModioUGCProvider : public UObject, public IUGCProvider
{
	GENERATED_BODY()

protected:
	//~ Begin IUGCProvider Interface
	virtual void InitializeProvider_Implementation(const FOnUGCProviderInitializedDelegate& Handler) override;
	virtual void DeinitializeProvider_Implementation(const FOnUGCProviderDeinitializedDelegate& Handler) override;
	virtual bool IsProviderEnabled_Implementation() override;
	virtual FModUGCPathMap GetInstalledUGCPaths_Implementation() override;
	//~ End IUGCProvider Interface

#if MODIO_UGC_SUPPORTED_PLATFORM
	/** Whether the UGC provider has been initialized. Only relevant when mod.io is used as the UGC provider */
	bool bSuccessfullyInitializedModio = false;
#endif
};