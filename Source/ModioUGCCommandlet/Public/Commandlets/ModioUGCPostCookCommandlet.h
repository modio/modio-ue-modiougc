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
#include "ModioUGCBaseCommandlet.h"

#include "ModioUGCPostCookCommandlet.generated.h"

/**
 * @docpublic
 * @brief This commandlet is intended to be run after cooking UGC content, to perform any additional work to package it
 * into a ModioUGC compatible format
 */
UCLASS()
class MODIOUGCCOMMANDLET_API UModioUGCPostCookCommandlet : public UModioUGCBaseCommandlet
{
	GENERATED_BODY()

public:
	UModioUGCPostCookCommandlet();

	virtual int32 Main(const FString& Params) override;

private:
	TSharedPtr<class FModioUGCPackager> UGCPackager;
};