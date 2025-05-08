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

#include "Delegates/Delegate.h"
#include "GameFramework/Actor.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"
#include "UGC/IModEnabledStateProvider.h"
#include "UGC/Types/GenericModID.h"
#include "UGC/Types/UGCPackage.h"
#include "UGC/Types/UGCSubsystemFeature.h"
#include "UGCProvider.h"

#include "UGCSubsystem.generated.h"

class IPlugin;

using UGCPackageEnumeratorFn = TFunction<bool(const FUGCPackage& Package)>;
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FUGCPackageEnumeratorDelegate, const FUGCPackage&, Package);

DECLARE_DYNAMIC_DELEGATE(FOnUGCPackagesChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUGCPackagesChangedMulticastDelegate);

UCLASS(Config = Game, defaultconfig)
class MODIOUGC_API UUGCSubsystem : public UEngineSubsystem, public IModEnabledStateProvider
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	/**
	 * Sets the UGC provider to use for UGC operations
	 * @param InUGCProvider The UGC provider to use
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Set UGC Provider"), Category = "mod.io|UGC")
	void SetUGCProvider(UPARAM(DisplayName = "UGC Provider") TScriptInterface<IUGCProvider> InUGCProvider);

	/**
	 * Initializes the UGC provider and scans for available UGC.
	 * @param Handler Callback for initialization success.
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC")
	void InitializeUGCProvider(const FOnUGCProviderInitializedDelegate& Handler);

	/**
	 * Scans for any unregistered UGC plugins and adds them to the internal registry
	 * Will emit a UGCChanged event if any new UGC plugins were added
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC")
	void RefreshUGC();

	/**
	 * Gets the UGC package associated with the provided mod ID
	 *
	 * @param ModID the ID of the mod to get the package for
	 * @param UGCPackage the package to populate with the UGC package data
	 * @return true if the package was found, false otherwise
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Get UGC Package By Mod ID", CompactNodeTitle = "Get UGC Package"),
			  Category = "mod.io|UGC")
	bool GetUGCPackageByModID(FGenericModID ModID, FUGCPackage& UGCPackage) const;

	/**
	 * Invokes the provided functor on every FUGCPackage in the registry
	 *
	 * @param Enumerator functor to invoke
	 */
	void EnumerateAllUGCPackages(const UGCPackageEnumeratorFn& Enumerator) const;

	/**
	 * Invokes the provided delegate on every FUGCPackage in the registry
	 *
	 * @param Enumerator The bound delegate to invoke
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, meta = (DisplayName = "Enumerate All UGC Packages"),
			  Category = "mod.io|UGC")
	void K2_EnumerateAllUGCPackages(const FUGCPackageEnumeratorDelegate& Enumerator) const;

	/**
	 * Adds UGC from a path. If successful, the UGC package should be available in discovered plugins
	 *
	 * @param Path The path to the UGC package, such as "C:\Users\Public\mod.io\6532\mods\4119758". This path must
	 * contain a .uplugin file in the root directory, and .pak file in "Content/Paks/[PLATFORM]" directory
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add UGC From Path"), Category = "mod.io|UGC")
	void AddUGCFromPath(const FString& Path);

	/**
	 * Deinitializes the UGC provider.
	 * @param Handler Callback for deinitialization success.
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC")
	void DeinitializeUGCProvider(const FOnUGCProviderDeinitializedDelegate& Handler);

	/**
	 * Indicates whether the UGC provider is enabled
	 * @return bool indicating whether the UGC provider is enabled
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Is Provider Enabled"), Category = "mod.io|UGC")
	bool IsProviderEnabled();

	/**
	 * Unloads all UGC Packages from the registry
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Unload All UGC Packages"), Category = "mod.io|UGC")
	void UnloadAllUGCPackages();

	/**
	 * Unmounts a UGC Package from the registry based on the provided mod ID
	 *
	 * @param ModID the ID of the package to unmount
	 * @param bRemoveUGCPackage should the package be removed from the registry altogether after unmounting? pass true
	 * for uninstallation
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Unmount UGC Package By Mod ID"), Category = "mod.io|UGC")
	void UnmountUGCPackageByModID(FGenericModID ModID,
								  UPARAM(DisplayName = "Remove UGC Package") bool bRemoveUGCPackage = false);

	/**
	 * Unmounts a UGC Package from the registry
	 * @param bRemoveUGCPackage should the package be removed from the registry altogether after unmounting? pass true
	 * for uninstallation
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Unmount UGC Package"), Category = "mod.io|UGC")
	void UnmountUGCPackage(UPARAM(ref) FUGCPackage& Package,
						   UPARAM(DisplayName = "Remove UGC Package") bool bRemoveUGCPackage = false);

	/**
	 * Registers a delegate to receive callbacks when UGC packages are added or removed from the registry
	 * @param Handler Delegate to invoke
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add UGC Changed Handler"), Category = "mod.io|UGC")
	void AddUGCChangedHandler(const FOnUGCPackagesChangedDelegate& Handler);

	/**
	 * Unbinds a delegate so it no longer receives callbacks on package registry alteration
	 *
	 * @param Handler Delegate to remove
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Remove UGC Changed Handler"), Category = "mod.io|UGC")
	void RemoveUGCChangedHandler(const FOnUGCPackagesChangedDelegate& Handler);

	/**
	 * Registers a delegate for notifications when a mod's enabled state changes
	 * @param Handler Delegate to be notified
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC|Events|Mod Enabled State Provider")
	void AddModEnabledStateChangeHandler(const FModEnabledStateChangeHandler& Handler);

	/**
	 * Unregisters a delegate for notifications when a mod's enabled state changes
	 * @param Handler Delegate to be removed from the notification list
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC|Events|Mod Enabled State Provider")
	void RemoveModEnabledStateChangeHandler(const FModEnabledStateChangeHandler& Handler);

	/**
	 * Allows the object to query for mod enable/disable state to be set externally
	 * @param NewProvider The new object to query via IModEnabledStateProvider
	 */
	UFUNCTION(BlueprintCallable, Category = "mod.io|UGC", meta = (DisplayName = "Set UGC Enabled State Provider"))
	void SetUGCEnabledStateProvider(TScriptInterface<IModEnabledStateProvider> NewProvider);

	/**
	 * Indicates whether a UGC subsystem feature is enabled
	 * @param Feature The feature to query
	 * @return bool indicating the state of the specified feature
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, meta = (DisplayName = "Is UGC Feature Enabled"),
			  Category = "mod.io|UGC|Features")
	bool IsUGCFeatureEnabled(EUGCSubsystemFeature Feature);

	/**
	 * Gets all package names from a UGC package from the asset registry
	 * @param UGCPackage The UGC package to get package names from
	 * @return Array of package names
	 */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Package Names From UGC Package"),
			  Category = "mod.io|UGC|Utilities")
	TArray<FName> GetPackageNamesFromUGCPackage(const FUGCPackage& UGCPackage) const;

protected:
	//~ Begin IModEnabledStateProvider Interface
	virtual bool NativeQueryIsModEnabled(FGenericModID ModID) override;
	virtual bool NativeRequestModEnabledStateChange(FGenericModID ID, bool bNewEnabledState) override;
	//~ End IModEnabledStateProvider Interface

	UPROPERTY()
	TScriptInterface<IUGCProvider> UGCProvider;

private:
	void InitializeProvider_Internal();
	void DeinitializeUGCProvider_Internal();

	/**
	 * Callback when the UGC Provider has been initialized
	 *
	 * @param bSuccess If is successfully initialized
	 */
	UFUNCTION()
	void OnUGCProviderInitialized(bool bSuccess);

	/**
	 * Callback when the UGC Provider has been deinitialized
	 *
	 * @param bSuccess If is successfully deinitialized
	 */
	UFUNCTION()
	void OnUGCProviderDeinitialized(bool bSuccess);

	/**
	 * Unmounts a package from the engine
	 *
	 * @param Package The package to unmount
	 */
	void UnmountUGCPackage_Internal(FUGCPackage& Package);

	/**
	 * Loads UGC from a plugin. This performs the necessary steps to load the plugin and add it to the UGC registry,
	 * allowing to access the UGC package and its assets
	 *
	 * @param LoadedPlugin The loaded plugin to load UGC from
	 * @param RawModID Optional raw mod ID to associate with the UGC package
	 */
	bool LoadUGC(TSharedPtr<IPlugin> LoadedPlugin, TOptional<FGenericModID> RawModID = {});

	/**
	 * Completely unloads UGC, cleaning up asset registration and mount point
	 *
	 * @param Package The loaded UGC package to unload
	 */
	bool UnloadUGC(FUGCPackage Package);

	/**
	 * Loaded UGC plugin names
	 */
	UPROPERTY(Transient)
	TSet<FName> LoadedUGCPlugins;

	/**
	 * UGC packages that have been mounted
	 */
	UPROPERTY(Transient)
	TSet<FUGCPackage> UGCPackages;

	TMultiMap<FUGCPackage, FName> RegisteredPackagesToPrimaryAssetTypesMap;

	/**
	 * Delegate to invoke when UGC packages are loaded or unloaded
	 */
	UPROPERTY()
	FOnUGCPackagesChangedMulticastDelegate OnUGCPackagesChanged;

	/**
	 * Provider for mod enabled state. Can be set externally to allow for custom mod enable/disable state handling
	 * See SetUGCEnabledStateProvider
	 */
	UPROPERTY()
	TScriptInterface<IModEnabledStateProvider> ModEnabledStateProvider;

	/**
	 * Delegate to invoke when a mod's enabled state changes
	 */
	UPROPERTY()
	FOnModEnabledStateChangeMulticastDelegate OnModEnabledStateChanged;

	/**
	 * Delegate to invoke when the provided IUGCProvider has been initialized
	 */
	UPROPERTY()
	FOnUGCProviderInitializedDelegate OnUGCProviderInitializedHandler;

	/**
	 * Delegate to invoke when the provided IUGCProvider has been deinitialized
	 */
	UPROPERTY()
	FOnUGCProviderDeinitializedDelegate OnUGCProviderDeinitializedHandler;
};