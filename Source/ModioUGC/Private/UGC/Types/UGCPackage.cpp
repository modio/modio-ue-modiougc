/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/Types/UGCPackage.h"

#include "AssetRegistry/AssetRegistryState.h"
#include "Engine/AssetManager.h"
#include "HAL/PlatformFileManager.h"
#include "IPlatformFilePak.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/EngineVersionComparison.h"
#include "ModioUGC.h"
#include "ShaderCodeLibrary.h"
#include "UGC/Types/UGC_Metadata.h"
#include "UGC/Utilities/PakFileHelpers.h"

FScopedPlatformPakFileOverride::FScopedPlatformPakFileOverride()
{
	// Try to retrieve an existing PakFile overlay
	PakPlatformFile =
		static_cast<FPakPlatformFile*>(FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));
	if (!PakPlatformFile)
	{
		// There wasn't one (i.e we're most likely in the editor) so add our own to the overlay chain
		PakPlatformFile = static_cast<FPakPlatformFile*>(
			FPlatformFileManager::Get().GetPlatformFile(FPakPlatformFile::GetTypeName()));
		if (PakPlatformFile)
		{
			// Fetch the original top-level overlay and wrap our new one around it
			OriginalPlatformFile = &FPlatformFileManager::Get().GetPlatformFile();
			PakPlatformFile->Initialize(OriginalPlatformFile, TEXT("-UseIoStore"));
			FPlatformFileManager::Get().SetPlatformFile(*PakPlatformFile);
		}
	}
}

FScopedPlatformPakFileOverride::~FScopedPlatformPakFileOverride()
{
	// Restore the original platform file overlay if we had to create a new one
	if (IsValid() && (OriginalPlatformFile != nullptr))
	{
		FPlatformFileManager::Get().SetPlatformFile(*OriginalPlatformFile);
	}
}

bool FScopedPlatformPakFileOverride::IsValid() const
{
	return PakPlatformFile != nullptr;
}

FPakPlatformFile* FScopedPlatformPakFileOverride::operator->() const
{
	checkf(IsValid(), TEXT("Attempting to access an invalid PakPlatformFile"));
	return PakPlatformFile;
}

FUGCPackage::FUGCPackage(const TSharedRef<IPlugin> Plugin, TOptional<FGenericModID> ModID /*= {}*/,
						 TFunction<FString(FString&)> FilePathSanitizationFn /*= nullptr*/)
	: AssociatedPlugin(Plugin),
	  ModID(ModID)
{
	FScopedPlatformPakFileOverride PlatformPakFile {};

	DescriptorPath = Plugin->GetDescriptorFileName();
	PackagePath = *Plugin->GetMountedAssetPath().LeftChop(1);
	ContentPath = *Plugin->GetContentDir();
	if (FilePathSanitizationFn)
	{
		DescriptorPath = FilePathSanitizationFn(DescriptorPath);
		PackagePath = FilePathSanitizationFn(PackagePath);
		ContentPath = FilePathSanitizationFn(ContentPath);
	}
	EngineVersion = *Plugin->GetDescriptor().EngineVersion;
	Author = *Plugin->GetDescriptor().CreatedBy;
	Description = *Plugin->GetDescriptor().Description;
	FriendlyName = *Plugin->GetDescriptor().FriendlyName;

	TArray<FString> FoundPaks;
	FPakFileSearchVisitor PakVisitor(FoundPaks);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString PathToSearch = ContentPath;
	UE_LOG(LogModioUGC, Verbose, TEXT("Searching directory for pak files %s"), *PathToSearch);
	PlatformFile.IterateDirectoryRecursively(*PathToSearch, PakVisitor);

	if (FoundPaks.Num() == 0)
	{
		UE_LOG(LogModioUGC, Error, TEXT("UGC `%s` does not contain any pak files within %s."), *FriendlyName,
			   *PathToSearch);
		MountState = EUGCPackageMountState::EUPMS_Unmounted;
		return;
	}
	else
	{
		UE_LOG(LogModioUGC, Verbose, TEXT("UGC `%s` contains %i pak files within %s."), *FriendlyName, FoundPaks.Num(),
			   *PathToSearch);
	}
	FString MountPoint = Plugin->GetMountedAssetPath();

	for (const FString& PakPath : FoundPaks)
	{
		UE_LOG(LogModioUGC, VeryVerbose, TEXT("Attempting to mount UGC pak file %s at %s"), *PakPath, *MountPoint);
		if (PlatformPakFile->Mount(*PakPath, 4, *MountPoint))
		{
			MountedPakFilePaths.Add(PakPath);
			UE_LOG(LogModioUGC, VeryVerbose, TEXT("Mounted UGC pak file %s at %s"), *PakPath, *MountPoint);
		}
		else
		{
			UE_LOG(LogModioUGC, VeryVerbose, TEXT("Failed to mount UGC pak file %s at %s"), *PakPath, *MountPoint);
		}
	}

	if (LoadAssets())
	{
		MountState = EUGCPackageMountState::EUPMS_Mounted;
	}
}

bool FUGCPackage::operator==(const FUGCPackage& Other) const
{
	// All other fields on the package are derived from these members
	return ModID == Other.ModID && AssociatedPlugin == Other.AssociatedPlugin;
}

bool FUGCPackage::operator!=(const FUGCPackage& Other) const
{
	return !(*this == Other);
}

bool FUGCPackage::UnloadAssets()
{
	// Note: AssetRegistry is automatically unloaded when the package is unmounted
	return UnregisterPrimaryAssets() && UnloadShaderLibrary();
}

bool FUGCPackage::LoadAssets()
{
	return LoadAssetRegistry() && RegisterPrimaryAssets() && LoadShaderLibrary();
}

bool FUGCPackage::LoadAssetRegistry()
{
	const FString AssetRegistryFilePath = PackagePath / TEXT("AssetRegistry.bin");
	FAssetRegistryState PluginAssetRegistry;
	if (FAssetRegistryState::LoadFromDisk(*AssetRegistryFilePath, FAssetRegistryLoadOptions(), PluginAssetRegistry))
	{
		LoadedAssetRegistryState = MakeShared<class FAssetRegistryState>(MoveTemp(PluginAssetRegistry));

		// For debugging purposes, log out all the package names and assets within this UGC package.
		if (UE_LOG_ACTIVE(LogModioUGC, VeryVerbose))
		{
			TArray<FName> PackageNames;
			LoadedAssetRegistryState->GetPackageNames(PackageNames);
			if (PackageNames.IsEmpty())
			{
				UE_LOG(LogModioUGC, Error, TEXT("UGC plugin %s AssetRegistry has no packages"), *FriendlyName);
				return false;
			}

			for (const auto& PN : PackageNames)
			{
				UE_LOG(LogModioUGC, VeryVerbose, TEXT("UGC plugin %s: AssetRegistry contains package %s"),
					   *FriendlyName, *PN.ToString());
			}

			TArray<FAssetData> AssetList;
			LoadedAssetRegistryState->GetAllAssets({}, AssetList);

			if (AssetList.IsEmpty())
			{
				UE_LOG(LogModioUGC, Error, TEXT("UGC plugin %s AssetRegistry has no assets"), *FriendlyName);
				return false;
			}

			for (const FAssetData& Asset : AssetList)
			{
				UE_LOG(LogModioUGC, VeryVerbose, TEXT("UGC plugin %s: AssetRegistry contains asset %s"), *FriendlyName,
					   *Asset.GetFullName());
			}

			PackageNames.Empty();
			AssetList.Empty();
		}

		UE_LOG(LogModioUGC, Verbose, TEXT("AssetRegistry for %s loaded from %s. Contains %i assets."), *FriendlyName,
			   *AssetRegistryFilePath, LoadedAssetRegistryState.Get()->GetNumAssets());
		IAssetRegistry::GetChecked().AppendState(*LoadedAssetRegistryState.Get());
	}
	else
	{
		UE_LOG(LogModioUGC, Error, TEXT("Failed to load plugin asset registry state %s"), *AssetRegistryFilePath);
		return false;
	}

	return true;
}

bool FUGCPackage::RegisterPrimaryAssets()
{
	if (!AssociatedPlugin)
	{
		UE_LOG(LogModioUGC, Error, TEXT("Unable to register primary assets on a null plugin."));
		return false;
	}

	// e.g. "/RedSpaceship/UUGC_Metadata.UUGC_Metadata"
	FString PreferredDataPath = PackagePath / UUGC_Metadata::GetDefaultAssetName();
	PackageMetadata = LoadMetadata(PreferredDataPath);
	if (!PackageMetadata.IsValid())
	{
		UE_LOG(LogModioUGC, Warning,
			   TEXT("UGC does not contain metadata. This content cannot be registered to the AssetManager, but can "
					"still be loaded using the AssetRegistry."));
	}
	else
	{
		PackageMetadata->DebugLogValues();

		bool bUseIoStore = false;
		GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bUseIoStore"), bUseIoStore, GGameIni);
#if WITH_EDITOR
		// When consuming in the editor, we just care about what the UGC was cooked with.
		if (PackageMetadata->bIoStoreEnabled)
		{
			UE_LOG(LogModioUGC, Error,
				   TEXT("bUseIoStore was enabled for UGC %s when cooked. This UGC will not be mounted in editor."),
				   *FriendlyName);
			return false;
		}
#else
		if (bUseIoStore != PackageMetadata->bIoStoreEnabled)
		{
			UE_LOG(
				LogModioUGC, Error,
				TEXT("bUseIoStore values mismatch between UGC (%s) and Base Game (%s). This UGC will not be mounted."),
				(PackageMetadata->bIoStoreEnabled ? TEXT("TRUE") : TEXT("FALSE")),
				(bUseIoStore ? TEXT("TRUE") : TEXT("FALSE")));
			return false;
		}
#endif

		if (PackageMetadata->PrimaryAssetTypesToScan.Num() == 0)
		{
			UE_LOG(LogModioUGC, Warning,
				   TEXT("UGC metadata hs no primary asset types assigned, so nothing will be registered to the "
						"AssetManager."));
			return true;
		}

		FString PluginRootPath = TEXT("/") + AssociatedPlugin->GetName() + TEXT("/");
		UAssetManager& LocalAssetManager = UAssetManager::Get();
		const IAssetRegistry& LocalAssetRegistry = LocalAssetManager.GetAssetRegistry();
		const bool bForceSynchronousScan = !LocalAssetRegistry.IsLoadingAssets();

		// Use our metadata to scan our list of primary asset types
		for (FPrimaryAssetTypeInfo PrimaryTypeInfo : PackageMetadata->PrimaryAssetTypesToScan)
		{
			// This function also fills out runtime data on the copy
			if (!LocalAssetManager.ShouldScanPrimaryAssetType(PrimaryTypeInfo))
			{
				continue;
			}

			LocalAssetManager.ScanPathsForPrimaryAssets(
				PrimaryTypeInfo.PrimaryAssetType, PrimaryTypeInfo.AssetScanPaths, PrimaryTypeInfo.AssetBaseClassLoaded,
				PrimaryTypeInfo.bHasBlueprintClasses, PrimaryTypeInfo.bIsEditorOnly, bForceSynchronousScan);

			LocalAssetManager.InvalidatePrimaryAssetDirectory();
			LocalAssetManager.RefreshPrimaryAssetDirectory(true);
		}
	}

	return true;
}

bool FUGCPackage::UnregisterPrimaryAssets()
{
	if (!AssociatedPlugin)
	{
		UE_LOG(LogModioUGC, Error, TEXT("Unable to unregister primary assets on a null plugin."));
		return false;
	}

	if (PackageMetadata == nullptr)
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Unable to unregister primary assets with no metadata."));
		return true; // Still return true as UGC without metadata is still valid
	}

	if (!PackageMetadata->IsValidLowLevel())
	{
		UE_LOG(LogModioUGC, Error, TEXT("Unable to unregister primary assets with invalid metadata."));
		return false;
	}

	for (FPrimaryAssetTypeInfo PrimaryTypeInfo : PackageMetadata->PrimaryAssetTypesToScan)
	{
		FString PluginRootPath = TEXT("/") + AssociatedPlugin->GetName() + TEXT("/");
		UAssetManager& LocalAssetManager = UAssetManager::Get();
		IAssetRegistry& LocalAssetRegistry = LocalAssetManager.GetAssetRegistry();
		const bool bForceSynchronousScan = !LocalAssetRegistry.IsLoadingAssets();

		// This function also fills out runtime data on the copy
		if (!LocalAssetManager.ShouldScanPrimaryAssetType(PrimaryTypeInfo))
		{
			continue;
		}

		LocalAssetManager.RemoveScanPathsForPrimaryAssets(
			PrimaryTypeInfo.PrimaryAssetType, PrimaryTypeInfo.AssetScanPaths, PrimaryTypeInfo.AssetBaseClassLoaded,
			PrimaryTypeInfo.bHasBlueprintClasses, PrimaryTypeInfo.bIsEditorOnly);

		LocalAssetManager.InvalidatePrimaryAssetDirectory();
		LocalAssetManager.RefreshPrimaryAssetDirectory(true);

		// For debugging purposes, check if the primary assets were correctly removed
		// TODO: Make it conditional
		if (UE_LOG_ACTIVE(LogModioUGC, VeryVerbose))
		{
			TArray<FPrimaryAssetId> AssetsAfter;
			LocalAssetManager.GetPrimaryAssetIdList(PrimaryTypeInfo.PrimaryAssetType, AssetsAfter);
			for (const FPrimaryAssetId& AssetId : AssetsAfter)
			{
				FSoftObjectPath AssetPath = LocalAssetManager.GetPrimaryAssetPath(AssetId);
				if (AssetPath.ToString().StartsWith(PluginRootPath))
				{
					UE_LOG(LogModioUGC, Error, TEXT("1. Failed to remove UGC type %s (still present in %s)"),
						   *PrimaryTypeInfo.PrimaryAssetType.ToString(), *AssetPath.ToString());
				}
			}

			if (LocalAssetManager.GetPrimaryAssetTypeInfo(FPrimaryAssetType(PrimaryTypeInfo.PrimaryAssetType),
														  PrimaryTypeInfo))
			{
				for (const FString& AssetScanPath : PrimaryTypeInfo.AssetScanPaths)
				{
					if (AssetScanPath.StartsWith(PluginRootPath))
					{
						UE_LOG(LogModioUGC, Error, TEXT("2. Failed to remove UGC type %s (still present in %s)"),
							   *PrimaryTypeInfo.PrimaryAssetType.ToString(), *AssetScanPath);
					}
				}
			}
		}
	}

	UnloadMetadata();
	LoadedAssetRegistryState.Reset();

	return true;
}

bool FUGCPackage::LoadShaderLibrary() const
{
	if (!AssociatedPlugin)
	{
		UE_LOG(LogModioUGC, Error, TEXT("Unable to load shader library on a null plugin."));
		return false;
	}

	bool bArchive = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bShareMaterialShaderCode"), bArchive,
					 GGameIni);
	bool bEnable = !FPlatformProperties::IsServerOnly() && FApp::CanEverRender() && bArchive;
	if (bEnable)
	{
#if UE_VERSION_NEWER_THAN(5, 3, 0)
		FShaderCodeLibrary::OpenPluginShaderLibrary(*AssociatedPlugin);
#else
		if (AssociatedPlugin->CanContainContent() && AssociatedPlugin->IsEnabled())
		{
			// load any shader libraries that may exist in this plugin
			FShaderCodeLibrary::OpenLibrary(AssociatedPlugin->GetName(), AssociatedPlugin->GetContentDir());
		}
#endif
	}

	return true;
}

bool FUGCPackage::UnloadShaderLibrary() const
{
	if (!AssociatedPlugin)
	{
		UE_LOG(LogModioUGC, Error, TEXT("Unable to unload shader library on a null plugin."));
		return false;
	}
	bool bArchive = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bShareMaterialShaderCode"), bArchive,
					 GGameIni);
	bool bEnable = !FPlatformProperties::IsServerOnly() && FApp::CanEverRender() && bArchive;
	if (bEnable)
	{
		FShaderCodeLibrary::CloseLibrary(AssociatedPlugin->GetName());
	}

	return true;
}


TWeakObjectPtr<UUGC_Metadata> FUGCPackage::LoadMetadata(const FString& InPath)
{
	if (FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(InPath)))
	{
		MetadataDataHandle = UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(FSoftObjectPath(InPath));

		if (MetadataDataHandle.IsValid())
		{
			// Synchronous for now
			MetadataDataHandle->WaitUntilComplete(0.0f, /*bLogErrors*/ false);

			UObject* LoadedObj = MetadataDataHandle->GetLoadedAsset();
			UUGC_Metadata* LoadedData = Cast<UUGC_Metadata>(LoadedObj);

			if (LoadedData && LoadedData->IsValidLowLevel())
			{
				// Return a weak pointer to avoid keeping it alive.
				return TWeakObjectPtr<UUGC_Metadata>(LoadedData);
			}
		}
	}

	UE_LOG(LogModioUGC, Error, TEXT("Unable to load metadata at %s"), *InPath);
	return nullptr;
}


void FUGCPackage::UnloadMetadata()
{
	if (MetadataDataHandle.IsValid())
	{
		MetadataDataHandle->ReleaseHandle(); // Unloads the asset if no other handles reference it
		MetadataDataHandle.Reset(); // Releases our reference to the handle
	}

	PackageMetadata.Reset();
	
	// Ensure async completes and run GC
	FlushAsyncLoading();
	CollectGarbage(RF_NoFlags);
}