/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/UGCLibrary.h"

#include "UGC/UGCSubsystem.h"

bool UUGCLibrary::GetModID(const FUGCPackage& UGCPackage, FGenericModID& ModID)
{
	if (UGCPackage.ModID.IsSet())
	{
		ModID = UGCPackage.ModID.GetValue();
		return true;
	}

	return false;
}

bool UUGCLibrary::EqualEqual_GenericModID(const FGenericModID& A, const FGenericModID& B)
{
	return A == B;
}

bool UUGCLibrary::NotEqual_GenericModID(const FGenericModID& A, const FGenericModID& B)
{
	return A != B;
}