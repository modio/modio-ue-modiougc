/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/Types/GenericModID.h"

#include "Misc/Crc.h"

FGenericModID::FGenericModID() : ModID(INDEX_NONE) {}

uint32 GetTypeHash(const FGenericModID& ModId)
{
	return FCrc::MemCrc32(&ModId.ModID, sizeof(int64));
}