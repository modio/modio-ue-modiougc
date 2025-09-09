/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCPackager.h"
#include "Developer/DesktopPlatform/Public/DesktopPlatformModule.h"
#include "FileHelpers.h"
#include "HAL/Platform.h"
#include "IUATHelperModule.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/DataDrivenPlatformInfoRegistry.h"
#include "Misc/EngineVersionComparison.h"
#include "Misc/PackageName.h"
#include "ModioUGCEditor.h"
#include "ModioUGCEditorStyle.h"
#include "Settings/PlatformsMenuSettings.h"
#include "Settings/ProjectPackagingSettings.h"
#include "UGC/Types/UGC_Metadata.h"
#include "UObject/SavePackage.h"
#include "Widgets/SWidget.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "ModioUGCPackager"

static const FString CookFlavor = "";
static const FString ConfigurationName = "Shipping";
static const bool bCompress = false;
static const FString ReleaseVersionName = "BaseGameRelease";

FModioUGCPackager::FModioUGCPackager() {}

FModioUGCPackager::~FModioUGCPackager() {}

bool FModioUGCPackager::IsAllContentSaved(TSharedRef<IPlugin> Plugin)
{
	bool bAllContentSaved = true;

	TArray<UPackage*> UnsavedPackages;
	FEditorFileUtils::GetDirtyContentPackages(UnsavedPackages);
	FEditorFileUtils::GetDirtyWorldPackages(UnsavedPackages);

	if (UnsavedPackages.Num() > 0)
	{
		FString PluginBaseDir = Plugin->GetBaseDir();

		for (UPackage* Package : UnsavedPackages)
		{
			FString PackageFilename;
			if (FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFilename))
			{
				if (PackageFilename.Find(PluginBaseDir) == 0)
				{
					bAllContentSaved = false;
					break;
				}
			}
		}
	}

	return bAllContentSaved;
}

FString FModioUGCPackager::MakeUATCommand(const FString& UProjectFile, const FName& PlatformNameIni,
										  const FString& StageDirectory)
{
	const FDataDrivenPlatformInfo& DDPI = FDataDrivenPlatformInfoRegistry::GetPlatformInfo(PlatformNameIni);
	FString UBTPlatformString = DDPI.UBTPlatformString;

	FString CommandLine = FString::Printf(TEXT("BuildCookRun -project=\"%s\" -noP4"), *UProjectFile);

	CommandLine += FString::Printf(TEXT(" -clientconfig=%s -serverconfig=%s"), *ConfigurationName, *ConfigurationName);

	// UAT should be compiled already
	CommandLine += " -nocompile -nocompileeditor";

	CommandLine += FApp::IsEngineInstalled() ? TEXT(" -installed") : TEXT("");

	CommandLine += " -utf8output";

	CommandLine += " -platform=" + UBTPlatformString;

	if (CookFlavor.Len() > 0)
	{
		CommandLine += " -cookflavor=" + CookFlavor;
	}

	CommandLine += " -build -cook -CookCultures=en -unversionedcookedcontent -pak";

	if (bCompress)
	{
		CommandLine += " -compressed";
	}

	// Taken from TurnkeySupportModule.cpp
	{
		UProjectPackagingSettings* AllPlatformPackagingSettings = GetMutableDefault<UProjectPackagingSettings>();

#if UE_VERSION_NEWER_THAN(5, 2, 0)
		UPlatformsMenuSettings* PlatformsSettings = GetMutableDefault<UPlatformsMenuSettings>();
#endif

		bool bIsProjectBuildTarget = false;
		const FTargetInfo* BuildTargetInfo =
#if UE_VERSION_NEWER_THAN(5, 2, 0)
			PlatformsSettings->
#else
			AllPlatformPackagingSettings->
#endif
			GetBuildTargetInfoForPlatform(PlatformNameIni, bIsProjectBuildTarget);

		// Only add the -Target=... argument for code projects. Content projects will return
		// UnrealGame/UnrealClient/UnrealServer here, but may need a temporary target generated to enable/disable
		// plugins. Specifying -Target in these cases will cause packaging to fail, since it'll have a different name.
		if (BuildTargetInfo && bIsProjectBuildTarget)
		{
			CommandLine += FString::Printf(TEXT(" -target=%s"), *BuildTargetInfo->Name);
		}

		// optional settings
		if (AllPlatformPackagingSettings->bSkipEditorContent)
		{
			CommandLine += TEXT(" -SkipCookingEditorContent");
		}

		if (AllPlatformPackagingSettings->FullRebuild)
		{
			CommandLine += TEXT(" -clean");
		}
	}

	CommandLine += " -stage";

	CommandLine += FString::Printf(TEXT(" -stagingdirectory=\"%s\""), *StageDirectory);

	return CommandLine;
}

FString FModioUGCPackager::MakeUATParams_BaseGame(const FString& UProjectFile)
{
	FString OutParams = FString::Printf(TEXT(" -package -createreleaseversion=\"%s\""), *ReleaseVersionName);

	TArray<FString> Result;
	TArray<FString> ProjectMapNames;

	const FString WildCard = FString::Printf(TEXT("*%s"), *FPackageName::GetMapPackageExtension());

	// Scan all Content folder, because not all projects follow Content/Maps convention
	IFileManager::Get().FindFilesRecursive(
		ProjectMapNames, *FPaths::Combine(FPaths::GetPath(UProjectFile), TEXT("Content")), *WildCard, true, false);

	for (int32 i = 0; i < ProjectMapNames.Num(); i++)
	{
		Result.Add(FPaths::GetBaseFilename(ProjectMapNames[i]));
	}

	Result.Sort();

	if (Result.Num() > 0)
	{
		// Our goal is to only have assets from inside the actual plugin content folder.
		// In order for Unreal to only put these assets inside the pak and not assets from /Game we have to specify all
		// maps from /Game to the command line for UAT. Format: -map=Value1+Value2+Value3

		OutParams += " -map=";

		for (int32 i = 0; i < Result.Num(); i++)
		{
			OutParams += Result[i];

			if (i + 1 < Result.Num())
			{
				OutParams += "+";
			}
		}
	}

	return OutParams;
}

FString FModioUGCPackager::MakeUATParams_DLC(const FString& DLCName)
{
	FString CommandLine = FString::Printf(TEXT(" -basedonreleaseversion=\"%s\""), *ReleaseVersionName);

	CommandLine += " -stagebasereleasepaks";

	CommandLine += FString::Printf(TEXT(" -DLCName=\"%s\""), *DLCName);

	return CommandLine;
}

bool FModioUGCPackager::IsShareMaterialShaderCodeEnabled()
{
	FString IniFile = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));

	bool bEnabled = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bShareMaterialShaderCode"), bEnabled,
					 IniFile);
	return bEnabled;
}

void FModioUGCPackager::SetShareMaterialShaderCodeEnabled(bool bEnabled)
{
	FString IniFile = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));

	// Set the value and flush to ensure it's written to disk and not just in memory
	GConfig->SetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bShareMaterialShaderCode"), bEnabled,
					 IniFile);
	GConfig->Flush(false, IniFile);
}

bool FModioUGCPackager::IsIoStoreEnabled()
{
	bool bEnabled = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bUseIoStore"), bEnabled, GGameIni);
	return bEnabled;
}

FText FModioUGCPackager::GetOutputPackagePath() const
{
	return FText::FromString(SelectedSettings.OutputPackagePath);
}

FString FModioUGCPackager::GetProjectPath()
{
	if (FPaths::IsProjectFilePathSet())
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	}
	if (FApp::HasProjectName())
	{
		FString ProjectPath = FPaths::ProjectDir() / FApp::GetProjectName() + TEXT(".uproject");
		if (FPaths::FileExists(ProjectPath))
		{
			return ProjectPath;
		}
		ProjectPath = FPaths::RootDir() / FApp::GetProjectName() / FApp::GetProjectName() + TEXT(".uproject");
		if (FPaths::FileExists(ProjectPath))
		{
			return ProjectPath;
		}
	}
	return FString();
}

void FModioUGCPackager::EnablePackageWidgets(bool bEnable)
{
	bCanPackage = bEnable;
	OnPackagingStatusChanged.ExecuteIfBound(!bEnable);
}

bool FModioUGCPackager::CanPackage() const
{
	return bCanPackage;
}

bool FModioUGCPackager::ValidateSelectedSettings(FText& OutValidationErrorMessage) const
{
	if (!SelectedSettings.Plugin.IsValid())
	{
		OutValidationErrorMessage = LOCTEXT("PackageUGCError_NoPluginSelected", "No plugin selected");
		return false;
	}

	if (SelectedSettings.PlatformName.IsEmpty())
	{
		OutValidationErrorMessage = LOCTEXT("PackageUGCError_NoPlatformSelected", "No platform selected");
		return false;
	}

	const TSharedPtr<FString>* FoundPlatformName =
		PlatformsSource.FindByPredicate([this](const TSharedPtr<FString>& PlatformName) {
			if (SelectedSettings.PlatformName == *PlatformName)
			{
				return true;
			}
			return false;
		});

	if (!FoundPlatformName)
	{
		OutValidationErrorMessage = LOCTEXT("PackageUGCError_InvalidPlatformSelected", "Invalid platform selected");
		return false;
	}

	if (SelectedSettings.OutputPackagePath.IsEmpty())
	{
		OutValidationErrorMessage = LOCTEXT("PackageUGCError_NoOutputPath", "No output path specified");
		return false;
	}

	if (!FPaths::ValidatePath(SelectedSettings.OutputPackagePath))
	{
		OutValidationErrorMessage = LOCTEXT("PackageUGCError_InvalidOutputPath", "Invalid output path specified");
		return false;
	}

	return true;
}

void FModioUGCPackager::LoadSavedSettings()
{
	if (GConfig)
	{
		GConfig->GetString(TEXT("ModioUGC.Core"), TEXT("OutputPackagePath"), SelectedSettings.OutputPackagePath,
						   GEditorPerProjectIni);

		bool bPlatformNameRetrieved = GConfig->GetString(TEXT("ModioUGC.Core"), TEXT("PlatformName"),
														 SelectedSettings.PlatformName, GEditorPerProjectIni);

		if (!bPlatformNameRetrieved && PlatformsSource.Num() > 0)
		{
			SelectedSettings.PlatformName = *PlatformsSource[0];
		}

		FString PluginName;
		GConfig->GetString(TEXT("ModioUGC.Core"), TEXT("PluginName"), PluginName, GEditorPerProjectIni);

		TSharedPtr<IPlugin>* SelectedPluginInternal =
			PluginsSource.FindByPredicate([this, PluginName](const TSharedPtr<IPlugin>& Plugin) {
				if (Plugin.IsValid() && Plugin->GetName() == PluginName)
				{
					return true;
				}
				return false;
			});

		if (SelectedPluginInternal)
		{
			SelectedSettings.Plugin = TSharedPtr<IPlugin>(*SelectedPluginInternal);
		}
		else if (PluginsSource.Num() > 0)
		{
			SelectedSettings.Plugin = PluginsSource[0];
		}
	}
}

void FModioUGCPackager::SaveSettings()
{
	if (GConfig)
	{
		GConfig->SetString(TEXT("ModioUGC.Core"), TEXT("OutputPackagePath"), *SelectedSettings.OutputPackagePath,
						   GEditorPerProjectIni);

		GConfig->SetString(TEXT("ModioUGC.Core"), TEXT("PlatformName"), *SelectedSettings.PlatformName,
						   GEditorPerProjectIni);

		if (SelectedSettings.Plugin)
		{
			GConfig->SetString(TEXT("ModioUGC.Core"), TEXT("PluginName"), *SelectedSettings.Plugin->GetName(),
							   GEditorPerProjectIni);
		}

		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

TSharedPtr<FJsonObject> DeserializePluginJson(const FString& Text, FText* OutFailReason = nullptr)
{
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		if (OutFailReason)
		{
			*OutFailReason =
				FText::Format(LOCTEXT("PackageUGCError_FailedToReadDescriptorFile", "Failed to read file. {0}"),
							  FText::FromString(Reader->GetErrorMessage()));
		}
		return TSharedPtr<FJsonObject>();
	}
	return JsonObject;
}

TSharedPtr<IPlugin> FModioUGCPackager::GetSelectedPlugin() const
{
	const TSharedPtr<IPlugin>* FoundPlugin = PluginsSource.FindByPredicate([this](const TSharedPtr<IPlugin>& Plugin) {
		if (SelectedSettings.Plugin.IsValid() && SelectedSettings.Plugin->GetVersePath() == *Plugin->GetVersePath() &&
			SelectedSettings.Plugin->GetName() == *Plugin->GetName())
		{
			return true;
		}
		return false;
	});

	if (FoundPlugin)
	{
		return *FoundPlugin;
	}

	if (PluginsSource.Num() > 0)
	{
		return PluginsSource[0];
	}

	return nullptr;
}

void FModioUGCPackager::PackagePlugin()
{
	FText ValidationErrorMessage;
	if (!ValidateSelectedSettings(ValidationErrorMessage))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
							 FText::Format(LOCTEXT("PackageUGCError_ValidationFailed", "Validation failed: {0}"),
										   ValidationErrorMessage));
		return;
	}

	if (!IsAllContentSaved(GetSelectedPlugin().ToSharedRef()))
	{
		FEditorFileUtils::SaveDirtyPackages(true, true, true);
	}

	if (!IsAllContentSaved(GetSelectedPlugin().ToSharedRef()))
	{
		FText PackageModError = FText::Format(
			LOCTEXT("PackageUGCError_UnsavedContent", "You must save all assets in {0} before you can share it."),
			FText::FromString(GetSelectedPlugin()->GetName()));

		FMessageDialog::Open(EAppMsgType::Ok, PackageModError);
		return;
	}

	FString PackagePath = *SelectedSettings.Plugin->GetMountedAssetPath().LeftChop(1);
	// e.g. "/RedSpaceship/UUGC_Metadata.UUGC_Metadata"
	FString PreferredDataPath = PackagePath / UUGC_Metadata::GetDefaultAssetName();

	// Load it from disk
	UObject* AssetObject = StaticLoadObject(UUGC_Metadata::StaticClass(), nullptr, *PreferredDataPath);
	if (UUGC_Metadata* Metadata = Cast<UUGC_Metadata>(AssetObject))
	{
		Metadata->DebugLogValues();
		Metadata->UnrealVersion = FEngineVersion::Current().ToString();
		UE_LOG(ModioUGCEditor, Log, TEXT("Setting Metadata `UnrealVersion` to `%s`"), *Metadata->UnrealVersion);

		Metadata->bIoStoreEnabled = IsIoStoreEnabled();
		UE_LOG(ModioUGCEditor, Log, TEXT("Setting Metadata `bIoStoreEnabled` to `%hhd`"), Metadata->bIoStoreEnabled);

		// Mark dirty so it saves properly
		Metadata->MarkPackageDirty();

		Metadata->Modify();
		Metadata->PostEditChange();

		if (UPackage* Package = Metadata->GetOutermost())
		{
			// Ensure the package is saved to disk
			FString PackageFilename;
			if (FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFilename,
																  FPackageName::GetAssetPackageExtension()))
			{
				FSavePackageArgs SavePackageArgs;
				SavePackageArgs.TopLevelFlags = EObjectFlags::RF_Standalone;
				UPackage::SavePackage(Package, Metadata, *PackageFilename, SavePackageArgs);
				UE_LOG(ModioUGCEditor, Log, TEXT("Successfully saved package to '%s'"), *PackageFilename);
			}
			else
			{
				UE_LOG(ModioUGCEditor, Error, TEXT("Failed to resolve package filename for '%s'"), *Package->GetName());
			}
		}
	}
	else if (AssetObject)
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("Failed to load metadata asset at %s"), *PreferredDataPath);
		FText PackageModError =
			FText::Format(LOCTEXT("PackageUGCError_MissingMetadataAsset", "Could not find metadata asset in {0}."),
						  FText::FromString(GetSelectedPlugin()->GetName()));
		FMessageDialog::Open(EAppMsgType::Ok, PackageModError);
		return;
	}
	else
	{
		UE_LOG(ModioUGCEditor, Warning, TEXT("Could not load or cast metadata asset at %s"), *PreferredDataPath);

		FText PackageModError =
			FText::Format(LOCTEXT("PackageUGCError_MissingMetadataAsset", "Could not find metadata asset in {0}."),
						  FText::FromString(GetSelectedPlugin()->GetName()));
		FMessageDialog::Open(EAppMsgType::Ok, PackageModError);
		return;
	}

	// Disable the window while we're packaging
	EnablePackageWidgets(false);
	const FString ProjectFile = GetProjectPath();

	// Start a UAT task to cook the project, which we need before cooking the Plugin.
	CookProject(GetOutputPackagePath().ToString(), ProjectFile, FName(GetSelectedPlatformName().ToString()))
		.Next([this, ProjectFile](bool bSucceeded) {
			if (!bSucceeded)
			{
				EnablePackageWidgets(true);
				return;
			}

			auto CopyPackagedPlugin = [this](bool bSucceeded) {
				if (!bSucceeded)
				{
					EnablePackageWidgets(true);
					return;
				}

				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				FString StagingDirectory =
					FPaths::Combine(SelectedSettings.OutputPackagePath, *SelectedSettings.Plugin->GetName());

				const FString UPluginPath = [this, &PlatformFile](const FString& StagingDirectory) {
					TArray<FString> FoundFiles;
					PlatformFile.FindFilesRecursively(FoundFiles, *StagingDirectory, TEXT(".uplugin"));
					if (FoundFiles.Num() == 1)
					{
						return FoundFiles[0];
					}
					return FString();
				}(StagingDirectory);

				const FString ContentDirectory = [this, &PlatformFile](const FString& PluginDirectory) {
					if (PlatformFile.DirectoryExists(*FPaths::Combine(PluginDirectory, FString("Content"))))
					{
						return FPaths::Combine(PluginDirectory, FString("Content"));
					}
					return FString();
				}(FPaths::GetPath(UPluginPath));

				if (UPluginPath.IsEmpty() || ContentDirectory.IsEmpty())
				{
					UE_LOG(ModioUGCEditor, Error,
						   TEXT("Failed to find plugin or content directory in staging directory '%s'"),
						   *StagingDirectory);
					EnablePackageWidgets(true);
					return;
				}

				if (!CorrectUPluginFile(UPluginPath))
				{
					UE_LOG(ModioUGCEditor, Error, TEXT("Failed to correct UPlugin file '%s'"), *UPluginPath);
					EnablePackageWidgets(true);
					return;
				}

				const FString ModRootName =
					FString::Printf(TEXT("%s_%s"), *SelectedSettings.Plugin->GetName(), *SelectedSettings.PlatformName);
				const FString ModRootPath = FPaths::Combine(SelectedSettings.OutputPackagePath, ModRootName);
				const FString UPluginPath_CopyTo = FPaths::Combine(ModRootPath, FPaths::GetCleanFilename(UPluginPath));
				const FString ContentDirectory_CopyTo = FPaths::Combine(ModRootPath, FString("Content"));

				// Create the UGC root directory (to place the .uplugin and content directory in)
				{
					if (PlatformFile.DirectoryExists(*ModRootPath))
					{
						PlatformFile.DeleteDirectoryRecursively(*ModRootPath);
					}
					if (!PlatformFile.CreateDirectoryTree(*ModRootPath))
					{
						UE_LOG(ModioUGCEditor, Error, TEXT("Failed to create UGC root directory '%s'"), *ModRootPath);
						EnablePackageWidgets(true);
						return;
					}
				}

				// Move the .uplugin file to the UGC root directory
				{
					if (PlatformFile.FileExists(*UPluginPath_CopyTo))
					{
						PlatformFile.DeleteFile(*UPluginPath_CopyTo);
					}
					PlatformFile.MoveFile(*UPluginPath_CopyTo, *UPluginPath);
					UE_LOG(ModioUGCEditor, Log, TEXT("Moved .uplugin file '%s' to '%s'"), *UPluginPath,
						   *UPluginPath_CopyTo);
				}

				// Move the content directory to the UGC root directory
				{
					if (PlatformFile.DirectoryExists(*ContentDirectory_CopyTo))
					{
						PlatformFile.DeleteDirectoryRecursively(*ContentDirectory_CopyTo);
					}
					PlatformFile.CopyDirectoryTree(*ContentDirectory_CopyTo, *ContentDirectory, true);
					PlatformFile.DeleteDirectoryRecursively(*ContentDirectory);
					UE_LOG(ModioUGCEditor, Log, TEXT("Moved content directory '%s' to '%s'"), *ContentDirectory,
						   *ContentDirectory_CopyTo);
				}

				// Delete the staging directory
				PlatformFile.DeleteDirectoryRecursively(*StagingDirectory);

				UE_LOG(ModioUGCEditor, Log, TEXT("Successfully packaged plugin '%s' for platform '%s' to '%s'"),
					   *SelectedSettings.Plugin->GetName(), *SelectedSettings.PlatformName, *ModRootPath);
				UE_LOG(ModioUGCEditor, Log, TEXT("You can now upload the UGC packaged in '%s' to mod.io"),
					   *ModRootPath);

				EnablePackageWidgets(true);

				// Convenience open the output directory to allow manual zipping
				FPlatformProcess::ExploreFolder(*ModRootPath);
			};

			const FString OutputDirectory =
				FPaths::Combine(SelectedSettings.OutputPackagePath, *SelectedSettings.Plugin->GetName());

			// After the base game cook is completed, start a UAT task to package the Plugin itself
			CookPlugin_DLC(SelectedSettings.Plugin.ToSharedRef(), OutputDirectory, ProjectFile,
						   FName(SelectedSettings.PlatformName))
				.Next(CopyPackagedPlugin);
		});
}

TFuture<bool> FModioUGCPackager::CookProject(const FString& OutputDirectory, const FString& UProjectFile,
											 const FName& PlatformNameIni)
{
	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();

	if (FPaths::DirectoryExists(FPaths::ProjectDir() / "Releases" / ReleaseVersionName / PlatformNameIni.ToString()))
	{
		UE_LOG(ModioUGCEditor, Log, TEXT("Existing release found, skipping base game cook"));
		Promise->SetValue(true);
	}
	else
	{
		const FString StagingDirectory = FPaths::Combine(OutputDirectory, TEXT("__TMP_STAGING__"));
		const FString CookProjectCommand =
			MakeUATCommand(UProjectFile, PlatformNameIni, StagingDirectory) + MakeUATParams_BaseGame(UProjectFile);

		IUATHelperModule::Get().CreateUatTask(
			CookProjectCommand, FText::FromString(PlatformNameIni.ToString()),
			FText::FromString("Packaging Base Game..."), FText::FromString("Packaging Base Game..."),
			FModioUGCEditorStyle::Get().GetBrush(TEXT("ModioUGCEditor.PackageUGCAction")), nullptr,
			[Promise, StagingDirectory](FString TaskResult, double TimeSec) {
				AsyncTask(ENamedThreads::GameThread, [Promise, TaskResult, StagingDirectory]() {
					// Clean up the staging directory
					IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
					PlatformFile.DeleteDirectoryRecursively(*StagingDirectory);

					if (TaskResult == "Completed")
					{
						UE_LOG(ModioUGCEditor, Log, TEXT("Successfully cooked project for UGC"));
						Promise->SetValue(true);
					}
					else
					{
						UE_LOG(ModioUGCEditor, Error, TEXT("Failed to cook project for UGC: %s"), *TaskResult);
						Promise->SetValue(false);
					}
				});
			});
	}

	return Promise->GetFuture();
}

TFuture<bool> FModioUGCPackager::CookPlugin_DLC(TSharedRef<IPlugin> Plugin, const FString& OutputDirectory,
												const FString& UProjectFile, const FName& PlatformNameIni)
{
	bool bWasUsingSharedShaderCode = IsShareMaterialShaderCodeEnabled();
	SetShareMaterialShaderCodeEnabled(false);

	TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();

	const FString CookPluginCommand =
		MakeUATCommand(UProjectFile, PlatformNameIni, OutputDirectory) + MakeUATParams_DLC(*Plugin->GetName());

	FText PackagingText = FText::Format(LOCTEXT("ModioUGCManagerEditor_PackagePluginTaskName", "Packaging {0}"),
										FText::FromString(Plugin->GetName()));

	IUATHelperModule::Get().CreateUatTask(
		CookPluginCommand, FText::FromString(PlatformNameIni.ToString()), PackagingText, PackagingText,
		FModioUGCEditorStyle::Get().GetBrush(TEXT("ModioUGCEditor.PackageUGCAction")), nullptr,
		[Promise, Plugin, bWasUsingSharedShaderCode](FString TaskResult, double TimeSec) {
			AsyncTask(ENamedThreads::GameThread, [Promise, Plugin, TaskResult, bWasUsingSharedShaderCode]() {
				// Reset our share shader code value to what it was prior to cooking
				SetShareMaterialShaderCodeEnabled(bWasUsingSharedShaderCode);

				if (TaskResult == "Completed")
				{
					UE_LOG(ModioUGCEditor, Log, TEXT("Successfully packaged plugin '%s' as UGC"), *Plugin->GetName());
					Promise->SetValue(true);
				}
				else
				{
					UE_LOG(ModioUGCEditor, Error, TEXT("Failed to package plugin '%s' as UGC: %s"), *Plugin->GetName(),
						   *TaskResult);
					Promise->SetValue(false);
				}
			});
		});

	return Promise->GetFuture();
}

bool FModioUGCPackager::CorrectUPluginFile(const FString& UPluginFilePath)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *UPluginFilePath))
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("Failed to read .uplugin file '%s'"), *UPluginFilePath);
		return false;
	}

	FText OutFailReason;
	TSharedPtr<FJsonObject> JsonObject = DeserializePluginJson(Text, &OutFailReason);
	if (!JsonObject.IsValid())
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("Failed to deserialize .uplugin file '%s': %s"), *UPluginFilePath,
			   *OutFailReason.ToString());
		return false;
	}

	bool bExplicitlyLoaded = false;
	JsonObject->TryGetBoolField(TEXT("ExplicitlyLoaded"), bExplicitlyLoaded);
	if (bExplicitlyLoaded)
	{
		UE_LOG(ModioUGCEditor, Log, TEXT("Plugin '%s' is already marked as explicitly loaded"), *UPluginFilePath);
		return true;
	}

	JsonObject->SetBoolField(TEXT("ExplicitlyLoaded"), true);

	FString CurrentEngineVersion = FEngineVersion::Current().ToString();
	JsonObject->SetStringField(TEXT("EngineVersion"), CurrentEngineVersion);

	FString NewText;
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NewText);
		if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
		{
			UE_LOG(ModioUGCEditor, Error, TEXT("Failed to serialize .uplugin file '%s'"), *UPluginFilePath);
			return false;
		}
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.DeleteFile(*UPluginFilePath);

	if (!FFileHelper::SaveStringToFile(NewText, *UPluginFilePath))
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("Failed to write .uplugin file '%s'"), *UPluginFilePath);
		return false;
	}

	UE_LOG(ModioUGCEditor, Log, TEXT("Successfully marked plugin '%s' as explicitly loaded"), *UPluginFilePath);
	return true;
}

void FModioUGCPackager::FindAvailablePlugins(TArray<TSharedRef<IPlugin>>& OutAvailableGameMods)
{
	OutAvailableGameMods.Empty();

	// Find available game mods from the list of discovered plugins

	for (TSharedRef<IPlugin> Plugin : IPluginManager::Get().GetDiscoveredPlugins())
	{
		// All game project plugins that are marked as mods are valid
		if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project && Plugin->GetType() == EPluginType::Mod)
		{
			OutAvailableGameMods.AddUnique(Plugin);
		}
	}
}

FText FModioUGCPackager::GetSelectedPlatformName() const
{
	const TSharedPtr<FString>* FoundPlatformName =
		PlatformsSource.FindByPredicate([this](const TSharedPtr<FString>& PlatformName) {
			if (SelectedSettings.PlatformName == *PlatformName)
			{
				return true;
			}
			return false;
		});

	if (FoundPlatformName)
	{
		return FText::FromString(**FoundPlatformName);
	}

	if (PlatformsSource.Num() > 0)
	{
		return FText::FromString(*PlatformsSource[0]);
	}

	UE_LOG(ModioUGCEditor, Error, TEXT("No platforms available for packaging"));
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
