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

#include "UGCSubsystemFeature.generated.h"

// @TODO: check whether we still need this enum?

UENUM(BlueprintType)
enum class EUGCSubsystemFeature : uint8
{
	EUSF_ModEnableDisable,
	EUSF_Monetization,
	EUSF_ModDownvote,
};