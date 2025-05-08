/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/ModioUGCProvider.h"

#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "Libraries/ModioCommonTypesLibrary.h"
#include "Libraries/ModioErrorConditionLibrary.h"
#include "Libraries/ModioSDKLibrary.h"
#include "ModioSubsystem.h"
#include "ModioUGC.h"
#include "ModioUGCSettings.h"
#include "Types/ModioInitializeOptions.h"
#include "UGC/Types/GenericModID.h"

void UModioUGCProvider::InitializeProvider_Implementation(const FOnUGCProviderInitializedDelegate& Delegate)
{
#if MODIO_UGC_SUPPORTED_PLATFORM
	const FModioInitializeOptions ModioInitializeOptions =
		UModioSDKLibrary::GetProjectInitializeOptionsForSessionId(FPlatformProcess::UserName());
	if (UModioSubsystem* ModioSubsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
	{
		ModioSubsystem->InitializeAsync(
			ModioInitializeOptions,
			FOnErrorOnlyDelegateFast::CreateWeakLambda(this, [this, Delegate](FModioErrorCode ErrorCode) {
				bSuccessfullyInitializedModio =
					!ErrorCode || UModioErrorConditionLibrary::ErrorCodeMatches(
									  ErrorCode, EModioErrorCondition::SDKAlreadyInitialized);
				if (bSuccessfullyInitializedModio)
				{
					UE_LOG(LogModioUGC, Log, TEXT("Mod.io as UGC provider initialized"));
				}
				else
				{
					UE_LOG(LogModioUGC, Error, TEXT("Failed to initialize mod.io as UGC provider: %s"),
						   *ErrorCode.GetErrorMessage());
				}

				Delegate.ExecuteIfBound(bSuccessfullyInitializedModio);
			}));
		return;
	}
	UE_LOG(LogModioUGC, Warning, TEXT("Mod.io subsystem not found, cannot initialize UGC provider"));
	Delegate.ExecuteIfBound(false);
#endif
}

void UModioUGCProvider::DeinitializeProvider_Implementation(const FOnUGCProviderDeinitializedDelegate& Handler)
{
#if MODIO_UGC_SUPPORTED_PLATFORM
	if (UModioSubsystem* ModioSubsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
	{
		ModioSubsystem->ShutdownAsync(
			FOnErrorOnlyDelegateFast::CreateWeakLambda(this, [Handler](FModioErrorCode ErrorCode) {
				if (ErrorCode)
				{
					UE_LOG(LogModioUGC, Error, TEXT("Failed to deinitialize mod.io as UGC provider: %s"),
						   *ErrorCode.GetErrorMessage());
					Handler.ExecuteIfBound(false);
				}
				else
				{
					UE_LOG(LogModioUGC, Log, TEXT("Mod.io as UGC provider deinitialized"));
					Handler.ExecuteIfBound(true);
				}
			}));
	}
	else
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Could not access mod.io subsystem, shutting down silently"));
		Handler.ExecuteIfBound(false);
	}
#endif
}

bool UModioUGCProvider::IsProviderEnabled_Implementation()
{
	const bool bEnabled = []() {
#if WITH_EDITOR
		const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
		return UGCSettings && UGCSettings->bEnableUGCProviderInEditor;
#elif MODIO_UGC_SUPPORTED_PLATFORM
		return true;
#else
		return false;
#endif
	}();

	if (!bEnabled)
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Mod.io UGC provider is disabled"));
	}

	return bEnabled;
}

FModUGCPathMap UModioUGCProvider::GetInstalledUGCPaths_Implementation()
{
#if MODIO_UGC_SUPPORTED_PLATFORM
	TMap<FString, FGenericModID> UGCPathsToModIDs;
	if (bSuccessfullyInitializedModio)
	{
		if (UModioSubsystem* Subsystem = GEngine->GetEngineSubsystem<UModioSubsystem>())
		{
			UE_LOG(LogModioUGC, Log, TEXT("Getting UGC paths from installed mod.io mods"));

			TMap<FModioModID, FModioModCollectionEntry> InstalledMods = Subsystem->QueryUserInstallations(true);
			for (const TPair<FModioModID, FModioModCollectionEntry>& Mod : InstalledMods)
			{
				UE_LOG(LogModioUGC, Log, TEXT("Found installed mod '%lld' at path '%s'"), GetUnderlyingValue(Mod.Key),
					   *Mod.Value.GetPath());
				UGCPathsToModIDs.Add(Mod.Value.GetPath(), FGenericModID(GetUnderlyingValue(Mod.Key)));
			}
		}
	}
	return FModUGCPathMap(MoveTemp(UGCPathsToModIDs));
#else
	UE_LOG(LogModioUGC, Error, TEXT("Mod.io UGC provider not supported on this platform"));
	return FModUGCPathMap();
#endif
}