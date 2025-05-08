/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/SideLoadUGCProvider.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

void USideLoadUGCProvider::InitializeProvider_Implementation(const FOnUGCProviderInitializedDelegate& Handler)
{
	// Check that side-load paths are valid

	Handler.ExecuteIfBound(true);
}

void USideLoadUGCProvider::DeinitializeProvider_Implementation(const FOnUGCProviderDeinitializedDelegate& Handler)
{
	Handler.ExecuteIfBound(true);
}

bool USideLoadUGCProvider::IsProviderEnabled_Implementation()
{
	return true;
}

FModUGCPathMap USideLoadUGCProvider::GetInstalledUGCPaths_Implementation()
{
	TMap<FString, FGenericModID> UGCPathsToModIDs;

	// Scan folder for mods
	const FString BaseModsPath = FPaths::ProjectDir() / TEXT("Modio");
	const FString ModsPathSearch = BaseModsPath / TEXT("*");

	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> OutFolders;
	FileManager.FindFiles(OutFolders, *ModsPathSearch, false, true);

	for (const FString& Folder : OutFolders)
	{
		UGCPathsToModIDs.Add(BaseModsPath / Folder, FGenericModID());
	}

	return FModUGCPathMap(MoveTemp(UGCPathsToModIDs));
}
