/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "Commandlets/ModioUGCPostCookCommandlet.h"

#include "HAL/PlatformFilemanager.h"

#include "Async/Async.h"
#include "Modules/ModuleManager.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ModioUGCCommandlet.h"

#include "ModioUGCEditor/public/ModioUGCPackager.h"

#include "Async/TaskGraphInterfaces.h"
#include "Containers/Ticker.h"
#include "Containers/UnrealString.h"

UModioUGCPostCookCommandlet::UModioUGCPostCookCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UModioUGCPostCookCommandlet::Main(const FString& Params)
{
	UE_LOG(ModioUGCCommandlet, Display, TEXT("Running ModioUGCPostCookCommandlet!"));
	ParseParameters(Params);

	if (OutputDir.IsEmpty())
	{
		UE_LOG(ModioUGCCommandlet, Error,
			   TEXT("Usage: -OutputDir=<Path>"
					"[-Args=<Extra>]"));
		return 1;
	}

	
	// The .uplugin file is expected to be within the output directory
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	UE_LOG(ModioUGCCommandlet, Display, TEXT("Looking for uplugin in: %s"), *OutputDir);
	const FString UPluginPath = [this, &PlatformFile](const FString& StagingDirectory) {
		TArray<FString> FoundFiles;
		PlatformFile.FindFilesRecursively(FoundFiles, *StagingDirectory, TEXT(".uplugin"));
		if (FoundFiles.Num() == 1)
		{
			return FoundFiles[0];
		}

		if (FoundFiles.Num() > 1)
		{
			UE_LOG(ModioUGCCommandlet, Error,
					TEXT("Multiple .uplugin files found in output directory '%s', expected only one, found:"),
					*StagingDirectory);
			for (const FString& FoundFile : FoundFiles)
			{
				UE_LOG(ModioUGCCommandlet, Error, TEXT("	- %s"), *FoundFile);
			}
		}
		return FString();
	}(OutputDir);

	bool bResult = false;
	if (!UPluginPath.IsEmpty())
	{
		bResult = FModioUGCPackager::CorrectUPluginFile(UPluginPath);
	}

	UGCPackager = MakeShared<FModioUGCPackager>();
	UGCPackager->LoadShaderCodeSettings();

	if (bResult)
	{
		UE_LOG(ModioUGCCommandlet, Display, TEXT("[SUCCESS] ModioUGCPostCCCommandlet process finished"));
	}
	else
	{
		UE_LOG(ModioUGCCommandlet, Error, TEXT("[FAILURE] ModioUGCPostCCCommandlet process finished"));
	}
	return bResult ? 0 : 1;
}