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

#include "Async/Future.h"
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SWidget.h"

class FModioUGCPackager : public TSharedFromThis<FModioUGCPackager>
{
public:
	DECLARE_DELEGATE_OneParam(FPackagingStatusChanged, bool /*bIsPackaging*/);

	FModioUGCPackager();
	~FModioUGCPackager();

	void PackagePlugin();

	/**
	 * Cooks the project for the specified platform. This needs to be done before attempting to cook a mod/plugin as
	 * DLC.
	 */
	static TFuture<bool> CookProject(const FString& OutputDirectory, const FString& UProjectFile,
									 const FName& PlatformNameIni);

	/**
	 * Cooks the plugin as a DLC.
	 */
	static TFuture<bool> CookPlugin_DLC(TSharedRef<class IPlugin> Plugin, const FString& OutputDirectory,
										const FString& UProjectFile, const FName& PlatformNameIni);

public:
	/**
	 * Corrects the UPlugin file by setting ExplicitlyLoaded to true and updating the EngineVersion.
	 *
	 * This function is designed to be used after packaging a plugin as DLC to correct the staged
	 * UPlugin file without modifying the original source file. Setting ExplicitlyLoaded to true,
	 * is required for the packaged mod to be correctly unloaded.
	 *
	 * @param UPluginFilePath The path to the UPlugin file.
	 * @return True if the UPlugin file was corrected (or didn't need to be corrected), false otherwise.
	 */
	bool CorrectUPluginFile(const FString& UPluginFilePath);

	/** Gets all available mod plugin packages. */
	void FindAvailablePlugins(TArray<TSharedRef<IPlugin>>& OutAvailableGameMods);

	FText GetSelectedPlatformName() const;

	/**
	 * Checks if a plugin has any unsaved content.
	 *
	 * @param Plugin	The plugin to check for unsaved content.
	 * @return True if all mod content has been saved, false otherwise.
	 */
	bool IsAllContentSaved(TSharedRef<class IPlugin> Plugin);

	static FString MakeUATCommand(const FString& UProjectFile, const FName& PlatformNameIni,
								  const FString& StageDirectory);
	static FString MakeUATParams_BaseGame(const FString& UProjectFile);
	static FString MakeUATParams_DLC(const FString& DLCName);

	static bool IsShareMaterialShaderCodeEnabled();
	static void SetShareMaterialShaderCodeEnabled(bool bEnabled);
	static bool IsIoStoreEnabled();

	/** Gets the output package path (directory to export the packaged plugin to). */
	FText GetOutputPackagePath() const;

	/** Gets the project path (full path with the .uproject extension). */
	static FString GetProjectPath();

	bool CanPackage() const;

	FPackagingStatusChanged OnPackagingStatusChanged;

protected:
	void EnablePackageWidgets(bool bEnable);

public:
	/** The source of the platforms (e.g. Windows, Android, etc.). */
	TArray<TSharedPtr<FString>> PlatformsSource;

	/** The source of the plugins. */
	TArray<TSharedPtr<IPlugin>> PluginsSource;

	/** Selected settings. */
	struct
	{
		/** The selected plugin. */
		TSharedPtr<IPlugin> Plugin;

		/** The selected platform name (e.g. Windows, Android, etc.). */
		FString PlatformName;

		/** The selected package path (directory to export the packaged plugin to). */
		FString OutputPackagePath;
	} SelectedSettings;

	/**
	 * Validates the selected settings.
	 * @param OutValidationErrorMessage The validation error message.
	 * @return True if the selected settings are valid, false otherwise.
	 */
	bool ValidateSelectedSettings(FText& OutValidationErrorMessage) const;

	/**
	 * Loads the saved settings from the config file, and populates the SelectedSettings with the loaded values.
	 */
	void LoadSavedSettings();

	/**
	 * Saves the selected settings to the config file.
	 */
	void SaveSettings();

	TSharedPtr<IPlugin> GetSelectedPlugin() const;

private:
	// If this is false, packaging options will be disabled
	bool bCanPackage;
};
