/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */


#include "Mutators/Mutator.h"
#include "Engine/World.h"

void UMutator::Initialise() 
{
	for (auto it = AssetOverrides.CreateIterator(); it; ++it)
	{
		it.Value().LoadSynchronous();
	}

	Initialised();
}

class UWorld* UMutator::GetWorld() const
{
	//TODO: this is not a good implementation; can cause conflicts with EditorWorld when in PIE
	return GWorld;
}
