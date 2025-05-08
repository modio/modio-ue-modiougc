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

#include "SideLoadUGCProvider.generated.h"

/**
 *
 */
UCLASS()
class MODIOUGC_API USideLoadUGCProvider : public UObject, public IUGCProvider
{
	GENERATED_BODY()

protected:
	//~ Begin IUGCProvider Interface
	virtual void InitializeProvider_Implementation(const FOnUGCProviderInitializedDelegate& Handler) override;
	virtual void DeinitializeProvider_Implementation(const FOnUGCProviderDeinitializedDelegate& Handler) override;
	virtual bool IsProviderEnabled_Implementation() override;
	virtual FModUGCPathMap GetInstalledUGCPaths_Implementation() override;
	//~ End IUGCProvider Interface
};
