/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "UGC/UGCSubsystem.h"
#include "Misc/EngineVersionComparison.h"
#include "ModioUGC.h"
#if UGC_SUPPORTED_PLATFORM
	#if MODIO_UGC_SUPPORTED_PLATFORM
		#include "ModioSubsystem.h"
	#endif
	#include "ModioSettings.h"
#endif
#include "Algo/AllOf.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "HAL/PlatformFileManager.h"
#include "IPlatformFilePak.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "ModioUGCSettings.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/MemoryReader.h"
#include "ShaderCodeLibrary.h"
#include "Subsystems/SubsystemCollection.h"
#include "Templates/Invoke.h"
#include "UGC/ModioUGCProvider.h"
#include "UGC/Types/UGC_Metadata.h"
#include "UGC/UGCProvider.h"
#include "UGC/Utilities/PakFileHelpers.h"

bool GetModMountPoint(TSharedPtr<IPlugin> Plugin, FString& RootPath, FString& ContentPath)
{
	if (!Plugin.IsValid())
	{
		return false;
	}

	// Root path will be "/YourPluginName/"
	// Content path will be "/YourPluginName/Content/"

	RootPath = Plugin->GetMountedAssetPath();
	ContentPath = RootPath / "Content/";

	return true;
}

void UUGCSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if UGC_SUPPORTED_PLATFORM
	// Do nothing if we are cooking or running a commandlet
	if (GIsCookerLoadingPackage || IsRunningCommandlet())
	{
		UE_LOG(LogModioUGC, Warning,
			   TEXT("Skipping UGC initialization and scan because we are cooking or running a commandlet"));
		return;
	}

	const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
	if (UGCProvider.GetObject() && IUGCProvider::Execute_IsProviderEnabled(UGCProvider.GetObject()) && UGCSettings &&
		UGCSettings->bAutoInitializeUGCProvider)
	{
	#if MODIO_UGC_SUPPORTED_PLATFORM
		Collection.InitializeDependency(UModioSubsystem::StaticClass());
		SetUGCProvider(NewObject<UModioUGCProvider>());

		InitializeUGCProvider(FOnUGCProviderInitializedDelegate());
	#endif
	}

#else
	UE_LOG(LogModioUGC, Warning, TEXT("Skipping UGC initialization because platform is not supported"));
#endif
}

void UUGCSubsystem::SetUGCProvider(TScriptInterface<IUGCProvider> NewProvider)
{
	UGCProvider = NewProvider;
}

void UUGCSubsystem::InitializeUGCProvider(const FOnUGCProviderInitializedDelegate& InHandler)
{
	if (UGCProvider.GetObject())
	{
		OnUGCProviderInitializedHandler = InHandler;

		// We want to run some internal logic when initializing first before returning to the caller
		FOnUGCProviderInitializedDelegate Handler;
		Handler.BindDynamic(this, &UUGCSubsystem::OnUGCProviderInitialized);
		IUGCProvider::Execute_InitializeProvider(UGCProvider.GetObject(), Handler);
	}
	else
	{
		InHandler.ExecuteIfBound(false);
	}
}

void UUGCSubsystem::OnUGCProviderInitialized(bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogModioUGC, Log, TEXT("UGC provider initialized"));
#if UE_VERSION_OLDER_THAN(5, 3, 0)
		FCoreDelegates::OnPakFileMounted2
#else
		FCoreDelegates::GetOnPakFileMounted2()
#endif
			.AddLambda([](const IPakFile& Pakfile) {
				FPakFileContentsIterator Iterator {Pakfile.PakGetPakFilename()};
				Pakfile.PakVisitPrunedFilenames(Iterator);
			});

		UAssetManager::CallOrRegister_OnAssetManagerCreated(
			FSimpleMulticastDelegate::FDelegate::CreateWeakLambda(this, [this]() { RefreshUGC(); }));
	}
	else
	{
		UE_LOG(LogModioUGC, Error, TEXT("UGC provider failed to initialize"));
	}

	OnUGCProviderInitializedHandler.ExecuteIfBound(bSuccess);
	OnUGCProviderInitializedHandler.Clear();
}

void UUGCSubsystem::Deinitialize()
{
	Super::Deinitialize();
#if UGC_SUPPORTED_PLATFORM
	if (GEngine && !IsEngineExitRequested() && UGCProvider.GetObject() &&
		IUGCProvider::Execute_IsProviderEnabled(UGCProvider.GetObject()))
	{
		FOnUGCProviderDeinitializedDelegate Handler;
		Handler.BindDynamic(this, &UUGCSubsystem::OnUGCProviderDeinitialized);
		IUGCProvider::Execute_DeinitializeProvider(UGCProvider.GetObject(), Handler);
	}
#endif
}

void UUGCSubsystem::DeinitializeUGCProvider(const FOnUGCProviderDeinitializedDelegate& InHandler)
{
	if (UGCProvider.GetObject())
	{
		OnUGCProviderDeinitializedHandler = InHandler;

		// We want to run some internal logic when deinitializing first before returning to the caller
		FOnUGCProviderDeinitializedDelegate Handler;
		Handler.BindDynamic(this, &UUGCSubsystem::OnUGCProviderDeinitialized);
		IUGCProvider::Execute_DeinitializeProvider(UGCProvider.GetObject(), Handler);
	}
	else
	{
		InHandler.ExecuteIfBound(false);
	}
}

void UUGCSubsystem::DeinitializeUGCProvider_Internal()
{
	FOnUGCProviderDeinitializedDelegate Handler;
	Handler.BindDynamic(this, &UUGCSubsystem::OnUGCProviderDeinitialized);
	IUGCProvider::Execute_DeinitializeProvider(UGCProvider.GetObject(), Handler);
}

void UUGCSubsystem::OnUGCProviderDeinitialized(bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogModioUGC, Log, TEXT("UGC provider deinitialized"));
	}
	else
	{
		UE_LOG(LogModioUGC, Error, TEXT("UGC provider failed to deinitialize"));
	}

	OnUGCProviderDeinitializedHandler.ExecuteIfBound(bSuccess);
	OnUGCProviderDeinitializedHandler.Clear();
}

bool UUGCSubsystem::IsProviderEnabled()
{
	if (UGCProvider.GetObject())
	{
		return IUGCProvider::Execute_IsProviderEnabled(UGCProvider.GetObject());
	}
	return false;
}

void UUGCSubsystem::RefreshUGC()
{
#if UGC_SUPPORTED_PLATFORM
	// Do nothing if we are cooking or running a commandlet
	if (GIsCookerLoadingPackage || IsRunningCommandlet())
	{
		return;
	}

	if (!UGCProvider.GetObject())
	{
		UE_LOG(LogModioUGC, Warning, TEXT("UGC provider is not available, skipping UGC refresh"));
		return;
	}

	if (!IUGCProvider::Execute_IsProviderEnabled(UGCProvider.GetObject()))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("UGC provider is not enabled, skipping UGC refresh"));
		return;
	}

	{
		TArray<FUGCPackage> InternalUGCPackages = UGCPackages.Array();

		for (FUGCPackage& UGCPackage : InternalUGCPackages)
		{
			UnloadUGC(UGCPackage);
		}
	}

	const FModUGCPathMap UGCPathMap = IUGCProvider::Execute_GetInstalledUGCPaths(UGCProvider.GetObject());
	for (const TPair<FString, FGenericModID>& UGCPath : UGCPathMap.PathToModIDMap)
	{
		AddUGCFromPath(UGCPath.Key);
	}

	bool bWasAnyUGCLoaded = false;
	IPluginManager::Get().RefreshPluginsList();
	for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetDiscoveredPlugins())
	{
		if (!Plugin->IsEnabled())
		{
			TOptional<FGenericModID> AssociatedModID;
			for (const TPair<FString, FGenericModID>& UGCPath : UGCPathMap.PathToModIDMap)
			{
				if (FPaths::IsUnderDirectory(Plugin->GetBaseDir(), UGCPath.Key))
				{
					AssociatedModID = UGCPath.Value;
					break;
				}
			}
			if (LoadUGC(Plugin, AssociatedModID))
			{
				bWasAnyUGCLoaded = true;
			}
		}
	}

	// Unmount mods whose descriptor files no longer exist
	{
		TSet<FName> ModsToRemove;
		for (const FName& LoadedPluginName : LoadedUGCPlugins)
		{
			const FString PluginDescriptorFile = LoadedPluginName.ToString();
			if (!FPaths::FileExists(PluginDescriptorFile))
			{
				ModsToRemove.Add(LoadedPluginName);
			}
		}

		TArray<FUGCPackage> InternalUGCPackages = UGCPackages.Array();

		// Unmount missing mods
		for (FUGCPackage& UGCPackage : InternalUGCPackages)
		{
			if (ModsToRemove.Find(FName(UGCPackage.DescriptorPath)))
			{
				UnmountUGCPackage(UGCPackage);
			}
		}
	}

	if (bWasAnyUGCLoaded)
	{
		OnUGCPackagesChanged.Broadcast();
	}

#endif
}

bool UUGCSubsystem::LoadUGC(TSharedPtr<IPlugin> LoadedPlugin, TOptional<FGenericModID> RawModID)
{
#if UGC_SUPPORTED_PLATFORM
	if (!LoadedPlugin)
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Attempting to call LoadUGC on a null plugin!"));
		return false;
	}
	if (LoadedPlugin->GetLoadedFrom() == EPluginLoadedFrom::Project && LoadedPlugin->GetDescriptor().Category == "UGC")
	{
		if (!LoadedUGCPlugins.Contains(FName(LoadedPlugin->GetDescriptorFileName())))
		{
			UE_LOG(LogModioUGC, Verbose, TEXT("Loading UGC plugin %s from %s"), *LoadedPlugin->GetName(),
				   *LoadedPlugin->GetDescriptorFileName());

			LoadedUGCPlugins.Add(FName(LoadedPlugin->GetDescriptorFileName()));
			IPluginManager::Get().MountNewlyCreatedPlugin(LoadedPlugin->GetName());

			// We register an additional mount point so that we can dynamically unload assets correctly prior to
			// uninstallation
			FString RootPath;
			FString ContentPath;
			GetModMountPoint(LoadedPlugin, RootPath, ContentPath);

			FPackageName::RegisterMountPoint(RootPath, ContentPath);
			FUGCPackage ModPackage {LoadedPlugin.ToSharedRef(), RawModID};

			// If we failed during the package object creation, unmount this piece of UGC straight away
			if (ModPackage.MountState != EUGCPackageMountState::EUPMS_Mounted)
			{
				UnloadUGC(ModPackage);
				return false;
			}

			UGCPackages.Add(ModPackage);

			if (ModPackage.PackageMetadata)
			{
				for (FPrimaryAssetTypeInfo PrimaryTypeInfo : ModPackage.PackageMetadata->PrimaryAssetTypesToScan)
				{
					RegisteredPackagesToPrimaryAssetTypesMap.Add(ModPackage, PrimaryTypeInfo.PrimaryAssetType);
				}
			}
			return true;
		}
		else
		{
			UE_LOG(LogModioUGC, VeryVerbose, TEXT("UGC plugin located at %s already loaded, skipping"),
				   *LoadedPlugin->GetDescriptorFileName());
		}
	}
#endif
	return false;
}

bool UUGCSubsystem::UnloadUGC(FUGCPackage Package)
{
#if UGC_SUPPORTED_PLATFORM
	UE_LOG(LogModioUGC, Verbose, TEXT("Unloading UGC plugin %s"), *Package.FriendlyName);
	bool _ = Package.UnloadAssets();
	UnmountUGCPackage(Package, true);

	RegisteredPackagesToPrimaryAssetTypesMap.Remove(Package);

	IPluginManager::Get().RefreshPluginsList();

	return true;
#endif
	return false;
}

bool UUGCSubsystem::UnloadUGCByModID(FGenericModID ModID)
{
#if UGC_SUPPORTED_PLATFORM
	FUGCPackage Package;
	if (GetUGCPackageByModID(ModID, Package))
	{
		return UnloadUGC(Package);
	}
	else
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Failed to unload UGC package by ModID since it was not found"));

		// Still return true, as the package is already in unloaded state
		return true;
	}
#endif
	return false;
}

bool UUGCSubsystem::IsUGCCompatible(const FString& UPluginFilePath)
{
#if UGC_SUPPORTED_PLATFORM
	const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
	if (!(UGCSettings && UGCSettings->bPerformUGCCheckVersion))
	{
		// If we don't want to check the version, we can skip the rest of the checks
		return true;
	}

	// Read the uplugin file
	FString UPluginContent;
	if (!FFileHelper::LoadFileToString(UPluginContent, *UPluginFilePath))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Failed to read uplugin file for compatibility check: %s"), *UPluginFilePath);
		return false;
	}

	// Parse JSON
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(UPluginContent);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Failed to parse uplugin JSON for compatibility check: %s"),
			   *UPluginFilePath);
		return false;
	}

	// Check if engine version field exists
	FString UGCEngineVersionString;
	FEngineVersion UGCEngineVersion;
	if (!JsonObject->TryGetStringField(TEXT("EngineVersion"), UGCEngineVersionString))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("UGC plugin missing engine version info: %s"), *UPluginFilePath);
		return false;
	}
	if (!FEngineVersion::Parse(UGCEngineVersionString, UGCEngineVersion))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Failed to parse engine version from uplugin: %s"), *UGCEngineVersionString);
		return false;
	}

	FEngineVersion CurrentEngineVersion = FEngineVersion::Current();
	UE_LOG(LogModioUGC, VeryVerbose, TEXT("Checking UGC plugin %s Engine version. Project: %s, Plugin: %s."),
		   *UPluginFilePath, *CurrentEngineVersion.ToString(), *UGCEngineVersionString);

	FString EngineComponent = CurrentEngineVersion.ToString(EVersionComponent::Major);
	FString UGCComponent = UGCEngineVersion.ToString(EVersionComponent::Major);
	if (UGCSettings->bPerformUGCCheckVersionComponentMajor && (EngineComponent != UGCComponent))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Major Engine Version mismatch. Project: %s, Plugin: %s."), *EngineComponent,
			   *UGCComponent);
		return false;
	}

	EngineComponent = CurrentEngineVersion.ToString(EVersionComponent::Minor);
	UGCComponent = UGCEngineVersion.ToString(EVersionComponent::Minor);
	if (UGCSettings->bPerformUGCCheckVersionComponentMinor && (EngineComponent != UGCComponent))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Minor Engine Version mismatch. Project: %s, Plugin: %s."), *EngineComponent,
			   *UGCComponent);
		return false;
	}

	EngineComponent = CurrentEngineVersion.ToString(EVersionComponent::Patch);
	UGCComponent = UGCEngineVersion.ToString(EVersionComponent::Patch);
	if (UGCSettings->bPerformUGCCheckVersionComponentPatch && (EngineComponent != UGCComponent))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Patch Engine Version mismatch. Project: %s, Plugin: %s."), *EngineComponent,
			   *UGCComponent);
		return false;
	}

	EngineComponent = CurrentEngineVersion.ToString(EVersionComponent::Changelist);
	UGCComponent = UGCEngineVersion.ToString(EVersionComponent::Changelist);
	if (UGCSettings->bPerformUGCCheckVersionComponentChangelist && (EngineComponent != UGCComponent))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Changelist Engine Version mismatch. Project: %s, Plugin: %s."),
			   *EngineComponent, *UGCComponent);
		return false;
	}

	EngineComponent = CurrentEngineVersion.ToString(EVersionComponent::Branch);
	UGCComponent = UGCEngineVersion.ToString(EVersionComponent::Branch);
	if (UGCSettings->bPerformUGCCheckVersionVersionComponentBranch && (EngineComponent != UGCComponent))
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Branch Engine Version mismatch. Project: %s, Plugin: %s."), *EngineComponent,
			   *UGCComponent);
		return false;
	}

	return true;
#endif
	return false;
}

void UUGCSubsystem::AddUGCFromPath(const FString& Path)
{
#if UGC_SUPPORTED_PLATFORM
	UE_LOG(LogModioUGC, Log, TEXT("Searching for UGC plugins at '%s'"), *Path);
	TArray<FString> PluginPaths;
	FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively(PluginPaths, *Path, TEXT(".uplugin"));
	bool bAddSearchPath = false;
	for (const FString& PluginPath : PluginPaths)
	{
		// Check compatibility first
		if (!IsUGCCompatible(PluginPath))
		{
			continue;
		}

		FString PluginName = FPaths::GetBaseFilename(PluginPath);
		if (TSharedPtr<IPlugin> FoundPlugin = IPluginManager::Get().FindPlugin(PluginName))
		{
			UE_LOG(LogModioUGC, Error,
				   TEXT("The plugin '%s' has already been discovered and enabled (%s). The new attempt using another "
						"path will not be added (%s)."),
				   *PluginName, *FoundPlugin->GetBaseDir(), *PluginPath);
			continue;
		}

		if (IPluginManager::Get().AddToPluginsList(PluginPath))
		{
			bAddSearchPath = true;
			UE_LOG(LogModioUGC, Log, TEXT("Added UGC plugin '%s' from '%s'"), *PluginName, *PluginPath);
		}
	}
	if (bAddSearchPath)
	{
		IPluginManager::Get().AddPluginSearchPath(Path, false);
	}
#endif
}

bool UUGCSubsystem::NativeQueryIsModEnabled(FGenericModID ModID)
{
#if UGC_SUPPORTED_PLATFORM
	if (const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>())
	{
		// If the feature is disabled, mods are enabled at all times
		if (!UGCSettings->bEnableModEnableDisableFeature)
		{
			return true;
		}
	}

	if (ModEnabledStateProvider)
	{
		return IModEnabledStateProvider::Execute_QueryIsModEnabled(ModEnabledStateProvider.GetObject(), ModID);
	}
	else
	{
		return true;
	}
#else
	return false;
#endif
}

bool UUGCSubsystem::NativeRequestModEnabledStateChange(FGenericModID ID, bool bNewEnabledState)
{
#if UGC_SUPPORTED_PLATFORM
	if (const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>())
	{
		// Disallow enable/disable state changes if the feature is disabled
		if (!UGCSettings->bEnableModEnableDisableFeature)
		{
			return false;
		}
	}

	if (ModEnabledStateProvider)
	{
		if (IModEnabledStateProvider::Execute_RequestModEnabledStateChange(ModEnabledStateProvider.GetObject(), ID,
																		   bNewEnabledState))
		{
			OnModEnabledStateChanged.Broadcast(ID, bNewEnabledState);
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

bool UUGCSubsystem::GetUGCPackageByModID(FGenericModID ModID, FUGCPackage& UGCPackage) const
{
#if UGC_SUPPORTED_PLATFORM
	for (const FUGCPackage& CurrentPackage : UGCPackages)
	{
		if (CurrentPackage.ModID.IsSet() && CurrentPackage.ModID.GetValue() == ModID)
		{
			UGCPackage = CurrentPackage;
			return true;
		}
	}
#endif
	return false;
}

void UUGCSubsystem::EnumerateAllUGCPackages(const UGCPackageEnumeratorFn& Enumerator) const
{
#if UGC_SUPPORTED_PLATFORM
	if (const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>())
	{
		// @TODO: Refactor so we can parameterize whether or not to include disabled packages
		if (UGCSettings->bEnableModEnableDisableFeature && ModEnabledStateProvider)
		{
			Algo::AllOf(UGCPackages,
						[Enumerator, ModEnabledStateProvider = ModEnabledStateProvider](const FUGCPackage& Package) {
							// Only packages with an associated mod ID are supported by enable/disable
							if (Package.ModID.IsSet())
							{
								// Only enumerate enabled packages
								if (IModEnabledStateProvider::Execute_QueryIsModEnabled(
										ModEnabledStateProvider.GetObject(), Package.ModID.GetValue()))
								{
									return Enumerator(Package);
								}
								else
								{
									// If the package was disabled, continue
									return true;
								}
							}
							return Enumerator(Package);
						});
			return;
		}
		Algo::AllOf(UGCPackages, Enumerator);
	}
#endif
}

void UUGCSubsystem::K2_EnumerateAllUGCPackages(const FUGCPackageEnumeratorDelegate& Enumerator) const
{
	EnumerateAllUGCPackages([Enumerator](const FUGCPackage& Package) {
		if (Enumerator.IsBound())
		{
			return Enumerator.Execute(Package);
		}
		else
		{
			return false;
		}
	});
}

void UUGCSubsystem::UnloadAllUGCPackages()
{
	TArray<FUGCPackage> UGCPackagesToUnmount;
	EnumerateAllUGCPackages([this, &UGCPackagesToUnmount](const FUGCPackage& Package) {
		UGCPackagesToUnmount.Add(Package);
		return true;
	});
	for (FUGCPackage& Package : UGCPackagesToUnmount)
	{
		UnloadUGC(Package);
	}
}

void UUGCSubsystem::UnmountUGCPackageByModID(FGenericModID ModID, bool bRemoveUGCPackage)
{
#if UGC_SUPPORTED_PLATFORM
	// UnmountUGCPackage modifies the UGCPackages array, so we need to make a copy to iterate over
	TArray<FUGCPackage> UGCPackagesCopy = UGCPackages.Array();

	// Not using EnumerateAllUGCPackages because we want mutable refs
	for (FUGCPackage& CurrentPackage : UGCPackagesCopy)
	{
		if (CurrentPackage.ModID.IsSet() && CurrentPackage.ModID.GetValue() == ModID)
		{
			UnmountUGCPackage(CurrentPackage, bRemoveUGCPackage);
		}
	}
#endif
}

void UUGCSubsystem::UnmountUGCPackage(FUGCPackage& Package, bool bRemoveUGCPackage)
{
#if UGC_SUPPORTED_PLATFORM
	UnmountUGCPackage_Internal(Package);

	if (bRemoveUGCPackage)
	{
		UGCPackages.Remove(Package);
		LoadedUGCPlugins.Remove(FName(Package.AssociatedPlugin->GetDescriptorFileName()));
	}

	OnUGCPackagesChanged.Broadcast();
#endif
}

void UUGCSubsystem::UnmountUGCPackage_Internal(FUGCPackage& Package)
{
#if UGC_SUPPORTED_PLATFORM
	FScopedPlatformPakFileOverride PlatformPakFile {};

	UE_LOG(LogModioUGC, Verbose, TEXT("Unmounting UGC Package %s"), *Package.FriendlyName);

	for (const FString& PakFile : Package.MountedPakFilePaths)
	{
		UE_LOG(LogModioUGC, VeryVerbose, TEXT("Unmounting UGC pak file %s"), *PakFile);
		PlatformPakFile->Unmount(*PakFile);
	}

	Package.MountState = EUGCPackageMountState::EUPMS_Unmounted;

	if (!Package.AssociatedPlugin)
	{
		UE_LOG(LogModioUGC, Warning, TEXT("Associated plugin for UGC package %s is null!"), *Package.FriendlyName);
		return;
	}

	UE_LOG(LogModioUGC, Verbose, TEXT("Removing AssetRegistry entries for %s"), *Package.FriendlyName);

	#if !WITH_EDITOR
	//  Remove the automatically-added, incorrect content mount point that the Plugin manager added.
	FPackageName::UnRegisterMountPoint(Package.AssociatedPlugin->GetMountedAssetPath(),
									   Package.AssociatedPlugin->GetContentDir());
	#endif

	// Remove our manually added mount point.
	FString RootPath;
	FString ContentPath;
	if (GetModMountPoint(Package.AssociatedPlugin, RootPath, ContentPath))
	{
		FPackageName::UnRegisterMountPoint(RootPath, ContentPath);
	}

	if (Package.AssociatedPlugin->IsEnabled())
	{
	#if WITH_EDITOR
		FPackageName::FOnContentPathDismountedEvent PathDismountedEvent = FPackageName::OnContentPathDismounted();
		FPackageName::OnContentPathDismounted().Clear();
	#endif

		TArray<FString> FileNames;
		FPlatformFileManager::Get().GetPlatformFile().FindFilesRecursively(
			FileNames, *Package.AssociatedPlugin->GetBaseDir(), TEXT(".uplugin"));
		for (const FString& PluginFileName : FileNames)
		{
			FText OutFailReason;
			IPluginManager::Get().RemoveFromPluginsList(PluginFileName, &OutFailReason);
		}

		FText FailReason;
		// Unmounts and disables the plugin
		if (IPluginManager::Get().UnmountExplicitlyLoadedPlugin(Package.AssociatedPlugin->GetName(), &FailReason))
		{
			IPluginManager::Get().RefreshPluginsList();
		}
		else
		{
			UE_LOG(LogModioUGC, Error, TEXT("Plugin %s cannot be unloaded: %s"), *Package.AssociatedPlugin->GetName(),
				   *FailReason.ToString());
		}
	#if WITH_EDITOR
		FPackageName::OnContentPathDismounted() = PathDismountedEvent;
	#endif
	}
#endif
}

void UUGCSubsystem::AddUGCChangedHandler(const FOnUGCPackagesChangedDelegate& Handler)
{
	OnUGCPackagesChanged.AddUnique(Handler);
}

void UUGCSubsystem::RemoveUGCChangedHandler(const FOnUGCPackagesChangedDelegate& Handler)
{
	OnUGCPackagesChanged.Remove(Handler);
}

void UUGCSubsystem::SetUGCEnabledStateProvider(TScriptInterface<IModEnabledStateProvider> NewProvider)
{
	const UModioUGCSettings* UGCSettings = GetDefault<UModioUGCSettings>();
	if (UGCSettings && UGCSettings->bEnableModEnableDisableFeature)
	{
		ModEnabledStateProvider = NewProvider;
	}
	else
	{
		ModEnabledStateProvider = nullptr;
	}
}

bool UUGCSubsystem::IsUGCFeatureEnabled(EUGCSubsystemFeature Feature)
{
#if UGC_SUPPORTED_PLATFORM
	const UModioSettings* ModioConfiguration = GetDefault<UModioSettings>();
	if (ModioConfiguration)
	{
		switch (Feature)
		{
			case EUGCSubsystemFeature::EUSF_ModDownvote:
				return ModioConfiguration->bEnableModDownvoteFeature;
			case EUGCSubsystemFeature::EUSF_Monetization:
				return ModioConfiguration->bEnableMonetizationFeature;
			case EUGCSubsystemFeature::EUSF_ModEnableDisable:
				return ModioConfiguration->bEnableModEnableDisableFeature;
			default:
				return false;
		}
	}
	else
	{
		return false;
	}
#else
	return false;
#endif
}

TArray<FName> UUGCSubsystem::GetPackageNamesFromUGCPackage(const FUGCPackage& UGCPackage) const
{
#if UGC_SUPPORTED_PLATFORM
	FScopedPlatformPakFileOverride PlatformPakFile {};
	TArray<FName> PackageNames;
	if (UGCPackage.LoadedAssetRegistryState)
	{
		UGCPackage.LoadedAssetRegistryState->GetPackageNames(PackageNames);
	}
	return PackageNames;
#else
	return TArray<FName>();
#endif
}

void UUGCSubsystem::RemoveModEnabledStateChangeHandler(const FModEnabledStateChangeHandler& Handler)
{
	OnModEnabledStateChanged.Remove(Handler);
}

void UUGCSubsystem::AddModEnabledStateChangeHandler(const FModEnabledStateChangeHandler& Handler)
{
	OnModEnabledStateChanged.AddUnique(Handler);
}