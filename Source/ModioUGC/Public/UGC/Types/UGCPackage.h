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
#include "Engine/AssetManagerTypes.h"
#include "GenericModID.h"
#include "UGC_Metadata.h"

#include "UGCPackage.generated.h"

struct FStreamableHandle;
class IPlugin;
class FPakPlatformFile;

UENUM(BlueprintType)
enum class EUGCPackageMountState : uint8
{
	EUPMS_Unmounted,
	EUPMS_Mounted
};

/**
 * Override for the platform file to allow for pak file mounting/unmounting within the scope (RAII)
 */
class MODIOUGC_API FScopedPlatformPakFileOverride
{
	FPakPlatformFile* PakPlatformFile = nullptr;
	IPlatformFile* OriginalPlatformFile = nullptr;

public:
	FScopedPlatformPakFileOverride();
	~FScopedPlatformPakFileOverride();
	bool IsValid() const;
	FPakPlatformFile* operator->() const;
};

/**
 * Structure containing data associated with this package
 */
USTRUCT(BlueprintType)
struct MODIOUGC_API FUGCPackage
{
	GENERATED_BODY()

	/**
	 * Information about this asset to register to the Asset Manager for discovery
	 */
	UPROPERTY()
	TWeakObjectPtr<UUGC_Metadata> PackageMetadata;

	/**
	 * Path to the descriptor file for the UGC package (e.g., "Folder/Subfolder/RedSpaceship.uplugin").
	 */
	UPROPERTY()
	FString DescriptorPath;

	/**
	 * Path to the UGC package itself (e.g., "/RedSpaceship").
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString PackagePath;

	/**
	 * Path to the Content directory within the package path (e.g., "/RedSpaceship/Content").
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString ContentPath;

	/**
	 * Engine version the UGC package was built for. Can be empty.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString EngineVersion;

	/**
	 * Author of the UGC package. Can be empty.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString Author;

	/**
	 * Description of the UGC package. Can be empty.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString Description;

	/**
	 * Friendly name of the UGC package (e.g., "RedSpaceship").
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	FString FriendlyName;

	/**
	 * Mounted pak file paths for the UGC package. Used to keep track of mounted pak files for the package, such as to
	 * unmount them when the package is removed.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	TArray<FString> MountedPakFilePaths;

	/**
	 * Mount state of the UGC package.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|UGCPackage")
	EUGCPackageMountState MountState = EUGCPackageMountState::EUPMS_Unmounted;

	/**
	 * Plugin associated with the UGC package.
	 */
	TSharedPtr<IPlugin> AssociatedPlugin;

	/**
	 * Asset registry state for the UGC package. Used to get asset data for the package.
	 */
	TSharedPtr<class FAssetRegistryState> LoadedAssetRegistryState;

	/**
	 * Mod ID associated with the UGC package. Can be empty.
	 */
	TOptional<FGenericModID> ModID;

	FUGCPackage() {}
	FUGCPackage(const TSharedRef<IPlugin> Plugin, TOptional<FGenericModID> ModID = {},
				TFunction<FString(FString&)> FilePathSanitizationFn = nullptr);

	bool operator==(const FUGCPackage& Other) const;
	bool operator!=(const FUGCPackage& Other) const;

	friend uint32 GetTypeHash(const FUGCPackage& Key)
	{
		return HashCombine(GetTypeHash(Key.ModID.Get(FGenericModID())), GetTypeHash(Key.AssociatedPlugin));
	}

	bool UnloadAssets();

private:
	TSharedPtr<FStreamableHandle> MetadataDataHandle;

	/**
	 * Perform all load operations for assets in the UGC package
	 */
	bool LoadAssets();

	/**
	 * Register the primary assets of the UGC package to the AssetManager.
	 */
	bool RegisterPrimaryAssets();

	/**
	 * Load the asset registry for the UGC package.
	 */
	bool LoadAssetRegistry();

	/**
	 * Load the shader library for the UGC package. This is relevant when the material shader code is shared.
	 */
	bool LoadShaderLibrary() const;

	/**
	 * Load the UGC metadata.
	 */
	TWeakObjectPtr<UUGC_Metadata> LoadMetadata(const FString& Path);

	/**
	 * Unregister the primary assets of the UGC package from the AssetManager.
	 */
	bool UnregisterPrimaryAssets();

	/**
	 * Unload the shader library for the UGC package. This is relevant when the material shader code is shared.
	 */
	bool UnloadShaderLibrary() const;

	/**
	 * Unload the UGC metadata.
	 */
	void UnloadMetadata();
};

/**
 * Blueprint accessible wrapper around a UGC Package
 * Needed if the consumer wants a UObject-based UGC Package, for example in UMG when using a list view
 */
UCLASS(BlueprintType, Category = "mod.io|UGC")
class MODIOUGC_API UUGCPackageWrapper : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * UGC Package
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "mod.io|UGCPackage")
	FUGCPackage Underlying;
};
