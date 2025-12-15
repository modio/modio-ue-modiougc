/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "Commandlets/ModioUGCPreCookCommandlet.h"

#include "HAL/PlatformFilemanager.h"

#include "Async/Async.h"
#include "Modules/ModuleManager.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ModioUGCCommandlet.h"

#include "ModioUGCEditor/public/ModioUGCPackager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Containers/Ticker.h"
#include "Containers/UnrealString.h"

UModioUGCPreCookCommandlet::UModioUGCPreCookCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UModioUGCPreCookCommandlet::Main(const FString& Params)
{
	UE_LOG(ModioUGCCommandlet, Display, TEXT("Running ModioUGCPreCookCommandlet!"));
	ParseParameters(Params);

	if (PluginName.IsEmpty())
	{
		UE_LOG(ModioUGCCommandlet, Error,
			   TEXT("Usage: -PluginName=<Name> "
					"[-Args=<Extra>]"));
		return 1;
	}

	UGCPackager = MakeShared<FModioUGCPackager>();

	bool bCanPackage = false;
	TSharedPtr<IPlugin> FoundPlugin;
	TArray<TSharedRef<IPlugin>> AvailablePlugins;
	UGCPackager->FindAvailablePlugins(AvailablePlugins);
	for (TSharedRef<IPlugin> Plugin : AvailablePlugins)
	{
		if (Plugin->GetName() == PluginName)
		{
			FoundPlugin = Plugin;
			bCanPackage = true;
			break;
		}
	}

	bool bResult = false;
	if (bCanPackage)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		FString PluginBaseDir = FoundPlugin->GetBaseDir();
		UE_LOG(ModioUGCCommandlet, Display, TEXT("Looking for uplugin in: %s"), *PluginBaseDir);
		const FString UPluginPath = [this, &PlatformFile](const FString& StagingDirectory) {
			TArray<FString> FoundFiles;
			PlatformFile.FindFilesRecursively(FoundFiles, *StagingDirectory, TEXT(".uplugin"));
			if (FoundFiles.Num() == 1)
			{
				return FoundFiles[0];
			}
			else
			{
				UE_LOG(ModioUGCCommandlet, Error,
					   TEXT("Could not find .uplugin file."));
			}
			return FString();
		}(PluginBaseDir);

		if (!UPluginPath.IsEmpty())
		{
			UGCPackager->StoreUGCMetadata(FoundPlugin);
			UGCPackager->StoreShaderCodeSettings();
			bResult = true;
		}
	}
	else
	{
		UE_LOG(ModioUGCCommandlet, Error, TEXT("Could not find plugin named '%s'"), *PluginName);
	}

	if (bResult)
	{
		UE_LOG(ModioUGCCommandlet, Display, TEXT("[SUCCESS] ModioUGCPreCookCommandlet process finished"));
	}
	else
	{
		UE_LOG(ModioUGCCommandlet, Error, TEXT("[FAILURE] ModioUGCPreCookCommandlet process finished"));
	}
	return bResult ? 0 : 1;
}