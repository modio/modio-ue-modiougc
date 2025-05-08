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

#include "ModioUGC.h"

class FPakFileContentsIterator final : public IPlatformFile::FDirectoryVisitor
{
	FString PakName;

public:
	explicit FPakFileContentsIterator(const FString& InPakName) : PakName(InPakName)
	{
		UE_LOG(LogModioUGC, VeryVerbose, TEXT("Pakfile %s mounted"), *PakName);
	}

	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		UE_LOG(LogModioUGC, VeryVerbose, TEXT("Pakfile %s : %s"), *PakName, FilenameOrDirectory);
		return true;
	}
};

class FPakFileSearchVisitor final : public IPlatformFile::FDirectoryVisitor
{
	TArray<FString>& FoundFiles;

public:
	explicit FPakFileSearchVisitor(TArray<FString>& InFoundFiles) : FoundFiles(InFoundFiles) {}
	virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
	{
		if (bIsDirectory == false)
		{
			const FString Filename(FilenameOrDirectory);
			if (Filename.MatchesWildcard(TEXT("*.pak")) && !FoundFiles.Contains(Filename))
			{
				FoundFiles.Add(Filename);
			}
		}
		return true;
	}
};