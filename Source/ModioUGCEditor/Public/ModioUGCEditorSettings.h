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

#include "Types/ModioGettingStartedEntry.h"

#include "ModioUGCEditorSettings.generated.h"

UCLASS(Config = Editor, defaultconfig)
class MODIOUGCEDITOR_API UModioUGCEditorSettings : public UObject
{
	GENERATED_BODY()

public:

	UModioUGCEditorSettings(const FObjectInitializer& Initializer);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Config, Category = "modio UGC Editor|Getting Started ")
	TSet<FModioGettingStartedEntry> GettingStartedEntries;
};
