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

#include "Commandlets/Commandlet.h"
#include "CoreMinimal.h"
#include "ModioUGCBaseCommandlet.generated.h"

UCLASS()
class MODIOUGCCOMMANDLET_API UModioUGCBaseCommandlet : public UCommandlet
{
	GENERATED_BODY()

protected:
	/**
	 * @brief Read and store any parameters passed to the commandlet, for instance from a build graph script to perform
	 * cloud cooking.
	 */
	void ParseParameters(const FString& Params);

	/**
	 * @brief The expected output directory where the UGC was cooked to, e.g.:
	 * stagingDirectory/<Platform>/<ProjectName>/Plugins/<UGCName>"
	 */
	FString OutputDir;

	/**
	 * @brief The name of the plugin asset, e.g.: "MyModPlugin.uplugin"
	 */
	FString PluginName;

	/**
	 * @brief Any additional arguments passed to the commandlet.
	 */
	FString AdditionalArgs;
};