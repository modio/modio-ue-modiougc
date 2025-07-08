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

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ModioUGCSettings.generated.h"

/**
 * @docpublic
 * @brief Settings for the mod.io UGC plugin
 */
UCLASS(Config = Game, DefaultConfig)
class MODIOUGC_API UModioUGCSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	//~ Begin UDeveloperSettings Interface
	virtual FName GetContainerName() const override
	{
		return FName("Project");
	}
	virtual FName GetCategoryName() const override
	{
		return FName("Plugins");
	}
	virtual FName GetSectionName() const override
	{
		return FName("mod.io UGC");
	}
#if WITH_EDITOR
	virtual FText GetSectionText() const override
	{
		return NSLOCTEXT("mod.io", "ModioUGCSettings", "mod.io UGC");
	}
	virtual FText GetSectionDescription() const override
	{
		return NSLOCTEXT("mod.io", "ModioUGCSettingsDescription", "Configure the mod.io UGC plugin");
	}
#endif
	//~ End UDeveloperSettings Interface

	/**
	 * @brief Whether we should automatically perform initialization of the underlying UGC provider (mod.io) or defer
	 * that to gameplay code
	 */
	UPROPERTY(Config, EditAnywhere, Meta = (DisplayName = "Auto Initialize UGC Provider"), Category = "UGC Provider")
	bool bAutoInitializeUGCProvider = false;

#if WITH_EDITORONLY_DATA
	/**
	 * @brief Whether we should enable the underlying UGC provider (mod.io) in-editor. Should be used for local
	 * development only, never for shipped editor builds given to players. This option potentially results in
	 * modifications to the CachedAssetRegistry for the editor and should not be enabled by default, only to improve
	 * iteration times when testing UGC functionality. With this option disabled, UGC will not be loaded in-editor.
	 */
	UPROPERTY(Config, EditAnywhere,
			  Meta = (DisplayName = "Enable UGC Provider in Editor", ConfigRestartRequired = true),
			  Category = "UGC Provider")
	bool bEnableUGCProviderInEditor = false;
#endif

	/**
	 * @brief Whether we should enable the feature for users being able to enable or disable specific mods,
	 * persisting that information into their user profile save data so it roams to other devices
	 * Note: ModioUGC does not handle this functionality in gameplay automatically
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Enable Mod Enable/Disable support"), Category = "Project")
	bool bEnableModEnableDisableFeature = false;

	/**
	 * @brief Whether we should perform a check of the version of Unreal Engine that was used for UGC plugins that are
	 * loaded against the current version of Unreal Engine being run, or the version that was used to build the game if
	 * this is a release.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Enable UGC version compatibility check"),
			  Category = "Version Compatibility")
	bool bPerformUGCCheckVersion = true;

	/**
	 * @brief Include the Major Version Component in the UGC version compatibility check.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Check Component Major"), Category = "Version Compatibility")
	bool bPerformUGCCheckVersionComponentMajor = true;

	/**
	 * @brief Include the `Minor` Version Component in the UGC version compatibility check.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Check Component Minor"), Category = "Version Compatibility")
	bool bPerformUGCCheckVersionComponentMinor = true;

	/**
	 * @brief Include the `Patch` Version Component in the UGC version compatibility check.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Check Component Patch"), Category = "Version Compatibility")
	bool bPerformUGCCheckVersionComponentPatch = false;

	/**
	 * @brief Include the `Changelist` Version Component in the UGC version compatibility check.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Check Component Changelist"),
			  Category = "Version Compatibility")
	bool bPerformUGCCheckVersionComponentChangelist = false;

	/**
	 * @brief Include the `Branch` Version Component in the UGC version compatibility check.
	 */
	UPROPERTY(Config, EditAnywhere, meta = (DisplayName = "Check Component Branch"), Category = "Version Compatibility")
	bool bPerformUGCCheckVersionVersionComponentBranch = false;
};