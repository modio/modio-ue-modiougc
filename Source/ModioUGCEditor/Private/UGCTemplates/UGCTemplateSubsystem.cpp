// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/UGCTemplateSubsystem.h"
#include "FileUtilities/ZipArchiveReader.h"
#include "FileUtilities/ZipArchiveWriter.h"
#include "Interfaces/IPluginManager.h"
#include "../Plugins/Developer/PluginUtils/Source/PluginUtils/Public/PluginUtils.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "FileHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Internationalization/TextPackageNamespaceUtil.h"
#include "UGC/Types/UGC_Metadata.h"
#include "UGCTemplates/Transaction.h"
#include "UGCTemplates/TransactionStep.h"
#include "UObject/CoreRedirects.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY(LogModioUGCTemplates)

void UUGCTemplateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Transaction = NewObject<UTransaction>();
	Transaction->AddToRoot();
}

void UUGCTemplateSubsystem::Deinitialize()
{
	if (Transaction != nullptr)
	{
		Transaction->RemoveFromRoot();
		Transaction = nullptr;
	}
}

FString UUGCTemplateSubsystem::FormatLogMessageInternal(const TCHAR* Format, ...)
{
	int32 BufferSize = 128;
	TCHAR* Buffer = nullptr;
	int32 Result = -1;

	while (Result == -1)
	{
		Buffer = (TCHAR*) FMemory::Realloc(Buffer, BufferSize * sizeof(TCHAR));
		GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize - 1, Format, Format, Result);
		if (Result == -1)
		{
			BufferSize *= 2;
		}
	}

	Buffer[Result] = TEXT('\0');
	FString FormattedMessage(Buffer);
	FMemory::Free(Buffer);
	return FormattedMessage;
}

void UUGCTemplateSubsystem::DiscoverTemplates(TArray<FUGCTemplateInfo>& Templates)
{
	auto TemplatePath = GetTemplateDirectory();

	IFileManager::Get().IterateDirectory(
		*TemplatePath, 
		[this, &Templates](const TCHAR* Pathname, bool bIsDirectory)
		{
			FString Path = FString(Pathname);
			if (!bIsDirectory && FPaths::GetExtension(Path, true).Equals(".zip", ESearchCase::IgnoreCase)) 
			{
				FUGCTemplateInfo Template = {.Name = FPaths::GetBaseFilename(Path),
											 .Path = Path,
											 .Descriptor = NewObject<UUGCTemplateDescriptor>()};
				if (ExtractTemplateDescriptor(Template))
				{
					Templates.Add(Template);
				}
			}
			return true;
		});

	TArray<FText> VerificationErrors;
	VerifyTemplates(Templates, VerificationErrors);

	if (VerificationErrors.Num() > 0)
	{
		for (auto Error : VerificationErrors)
		{
			LogError(*(Error.ToString()));
		}
	}

	CachedTemplates = Templates;
}

void UUGCTemplateSubsystem::DiscoverUGC(TArray<FUGCPluginInfo>& UGCPlugins)
{
	// Find available game mods from the list of discovered plugins
	for (TSharedRef<IPlugin> Plugin : IPluginManager::Get().GetDiscoveredPlugins())
	{
		// All game project plugins that are marked as mods are valid
		if (Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project && Plugin->GetDescriptor().Category == TEXT("UGC") &&
			(Plugin->GetType() == EPluginType::Mod || Plugin->GetType() == EPluginType::Project))
		{
			UGCPlugins.Add({Plugin->GetFriendlyName(), Plugin});
		}
	}
}

bool UUGCTemplateSubsystem::CreateUGCFromTemplate(FString NewModName, const FUGCTemplateInfo& TemplateInfo, const TMap<FString, FString>& Substitutions) 
{
	Transaction->Begin();
	LogInfo("Begin CreateUGCFromTemplate: %s", *(TemplateInfo.Name));
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	{
		FString NameFailReason;
		if (!IsValidModName(NewModName, NameFailReason))
		{
			LogError("%s is an invalid name. %s", *(NewModName), *(NameFailReason));
			Transaction->Cancel();
			return false;
		}

		FZipArchiveReader Reader(PlatformFile.OpenRead(*TemplateInfo.Path));

		if (!Reader.IsValid())
		{
			return false;
		}
		
		auto TemplateFileName = TemplateInfo.Name + ".template";
		auto EmbeddedFileNames = Reader.GetFileNames();

		if (EmbeddedFileNames.Find(TemplateFileName) == INDEX_NONE)
		{
			LogError("Cannot find template file for %s", *(TemplateInfo.Name));
			Transaction->Cancel();
			return false;
		}

		EmbeddedFileNames.Remove(TemplateFileName);

		//Create destination directory if it doesn't already
		auto& FileManager = IFileManager::Get();
		auto DestinationDirectory = FPaths::Combine(FPaths::ProjectDir(), "Mods");
		//if (FileManager.DirectoryExists(*DestinationDirectory))
		//{
		//	LogError("Mod with same name [%s] already exists", *NewModName);
		//	Transaction->Cancel();
		//	return false;
		//}

		FPluginUtils::FNewPluginParamsWithDescriptor CreationParams;
		CreationParams.Descriptor.bCanContainContent = true;
		CreationParams.Descriptor.FriendlyName = NewModName;
		CreationParams.Descriptor.Version = 1;
		CreationParams.Descriptor.VersionName = TEXT("1.0");
		CreationParams.Descriptor.Category = TEXT("UGC");

		//CreationParams.Descriptor.CreatedBy = CreatedBy;
		//CreationParams.Descriptor.CreatedByURL = CreatedByURL;
		//CreationParams.Descriptor.Description = Description;
		//CreationParams.Descriptor.bIsBetaVersion = bIsBetaVersion;

		FText FailReason;
		FPluginUtils::FLoadPluginParams LoadParams;
		LoadParams.bEnablePluginInProject = true;
		LoadParams.bUpdateProjectPluginSearchPath = true;
		LoadParams.OutFailReason = &FailReason;
		auto LoadedPlugin = FPluginUtils::CreateAndLoadNewPlugin(NewModName, DestinationDirectory, CreationParams, LoadParams);
		if (LoadedPlugin == nullptr)
		{
			LogError("Could not create new mod %s", *NewModName);
			return false;
		}
		Transaction->Add(UTransactionStepCreatePlugin::Make(LoadedPlugin));
		
		TMap<FString, FString> FullSubstitutions = Substitutions;
		FullSubstitutions.Add("___UGCNAME___", NewModName);
		FullSubstitutions.Add("___MODNAME___", NewModName);

		//ExtractFilesFromTemplate(Reader, DestinationDirectory, FullSubstitutions);
		bool bSuccess = true;
		bSuccess = ExtractFilesFromTemplate(Reader, LoadedPlugin->GetBaseDir(), FullSubstitutions);
		if (!bSuccess)
		{
			//clean up
			Transaction->Cancel();
			return false;
		}

		FUGCPluginInfo NewMod;
		NewMod.Name = NewModName;
		NewMod.Plugin = LoadedPlugin;
		//bSuccess = ExtractChildTemplateItems(NewMod, TemplateInfo.Descriptor->ChildTemplates, FullSubstitutions);
		//if (!bSuccess)
		//{
		//	//clean up
		//	Transaction->Cancel();
		//	return false;
		//}

		//bSuccess = CreateChildObjects(NewMod, TemplateInfo.Descriptor->ChildObjects, FullSubstitutions);
		//if (!bSuccess)
		//{
		//	Transaction->Cancel();
		//	return false;
		//}

		FixAssetReferences(TemplateInfo, LoadedPlugin);

		//rename
		RenameAssets(FullSubstitutions);
	}

	Transaction->Finish();
	LogInfo("Successfully created new UGC mod %s from template %s", *NewModName, *(TemplateInfo.Name));

	return true;
}

bool UUGCTemplateSubsystem::AddUGCTemplateItemTo(const FUGCPluginInfo& Mod, const FUGCTemplateInfo& TemplateInfo,
												 const TMap<FString, FString>& Substitutions)
{
	Transaction->Begin();
	TMap<FString, FString> FullSubstitutions = Substitutions;
	FullSubstitutions.Add("___UGCNAME___", Mod.Name);
	FullSubstitutions.Add("___MODNAME___", Mod.Name);

	bool bSuccess = AddUGCTemplateItemTo_Internal(Mod, TemplateInfo, FullSubstitutions);
	if (!bSuccess)
	{
		Transaction->Cancel();
		return false;
	}

	//rename files
	FixAssetReferences(TemplateInfo, Mod.Plugin);
	RenameAssets(FullSubstitutions);
	Transaction->Finish();
	LogInfo("Successfully add UGC template %s to mod %s", *(TemplateInfo.Name), *(Mod.Name));
	
	return bSuccess;
}

bool UUGCTemplateSubsystem::AddUGCTemplateItemTo_Internal(const FUGCPluginInfo& Mod, const FUGCTemplateInfo& TemplateInfo, const TMap<FString, FString>& Substitutions)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	{
		FZipArchiveReader Reader(PlatformFile.OpenRead(*TemplateInfo.Path));

		if (!Reader.IsValid())
		{
			LogError("Could not open archive for %s [%s]",*(TemplateInfo.Name), *(TemplateInfo.Path));
			return false;
		}
			
		auto TemplateFileName = TemplateInfo.Name + ".template";
		auto EmbeddedFileNames = Reader.GetFileNames();

		if (EmbeddedFileNames.Find(TemplateFileName) == INDEX_NONE)
		{
			LogError("Cannot find template file for %s", *(TemplateInfo.Name));
			return false;
		}

		if (Mod.Plugin == nullptr)
		{
			LogError("Could not find plugin for mod %s", *(Mod.Name));
			return false;
		}

		auto DestinationDirectory = Mod.Plugin->GetBaseDir();
		bool bExtractSuccessful = ExtractFilesFromTemplate(Reader, DestinationDirectory, Substitutions);
		if (!bExtractSuccessful)
		{
			return false;
		}

		//TODO: Log unsuccessful attempts
		//ExtractChildTemplateItems(Mod, TemplateInfo.Descriptor->ChildTemplates, Substitutions);
		//CreateChildObjects(Mod, TemplateInfo.Descriptor->ChildObjects, Substitutions);

		//TODO: Register new uassets so they show up immediately
		return bExtractSuccessful;
	}
}

bool UUGCTemplateSubsystem::ExportUGCTemplate(const FUGCPluginInfo& Mod, class UUGCTemplateDescriptor* TemplateDescriptor, bool bAutoReplaceName /*= false*/)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	{
		LogInfo("Exporting %s template", *(Mod.Name));
		FZipArchiveWriter ZipWriter(PlatformFile.OpenWrite(*(FPaths::Combine(GetTemplateDirectory(), Mod.Name + ".zip"))));

		//write template file
		TArray<uint8> TemplateDescriptorContents;
		bool bSuccess = UUGCTemplateDescriptor::Save(TemplateDescriptor, TemplateDescriptorContents);
		if (!bSuccess)
		{
			LogError("Could not save template descriptor for %s", *(Mod.Name));
			return false;
		}
		ZipWriter.AddFile(Mod.Name + ".template", TemplateDescriptorContents, FDateTime::Now());
		LogInfo("Exporting %s", *(Mod.Name + ".template"));

		FString NameReplacement = TemplateDescriptor->Type == EUGCTemplateType::TT_Mod ? "___MODNAME___" : "___UGCNAME___";
		FString ExportDir = Mod.Plugin->GetBaseDir();
		//add files from base dir to zip
		IFileManager::Get().IterateDirectoryRecursively(
			*ExportDir, 
			[this, &ZipWriter, &PlatformFile, &ExportDir, &Mod, &bAutoReplaceName, &NameReplacement, &TemplateDescriptor](const TCHAR* Pathname,
																										bool bIsDirectory)
			{
				bool bIsMetaDataFile = FPaths::GetBaseFilename(Pathname).Equals("U" + UUGC_Metadata::StaticClass()->GetName());
				bool bIsPluginFile = FPaths::GetExtension(Pathname, true).Equals(".uplugin");
				if (!bIsDirectory && !(bIsMetaDataFile && TemplateDescriptor->Type == EUGCTemplateType::TT_Item) && !bIsPluginFile)
				{
					FString EmbeddedPath(Pathname);
					LogInfo("Exporting %s", *EmbeddedPath);
					EmbeddedPath.RightChopInline(ExportDir.Len() + 1);
					TArray<uint8> Contents;
					FFileHelper::LoadFileToArray(Contents, Pathname);
					if (bAutoReplaceName)
					{
						EmbeddedPath.ReplaceInline(*(Mod.Name), *NameReplacement);
					}
					ZipWriter.AddFile(EmbeddedPath, Contents, FDateTime::Now());
				}
				return true;
			});

	}

	return false;
}

void UUGCTemplateSubsystem::GetTemplates(TArray<FUGCTemplateInfo>& Templates) const
{
	Templates = CachedTemplates;
}

void UUGCTemplateSubsystem::GetSubstitutionsFor(const FUGCTemplateInfo& TemplateInfo, TSet<FString>& Subtitutions) 
{
	GatherSubsitutionsFor(TemplateInfo, Subtitutions, "");
}

FString UUGCTemplateSubsystem::GetTemplateDirectory() const
{
	return FPaths::Combine(FPaths::ProjectDir(), "ModTemplates");
}

bool UUGCTemplateSubsystem::ExtractTemplateDescriptor(FUGCTemplateInfo& TemplateInfo)
{
	TArray<uint8> Contents;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	{
		LogInfo("Extracting Template descriptor for %s", *(TemplateInfo.Name));
		FZipArchiveReader Reader(PlatformFile.OpenRead(*TemplateInfo.Path));

		if (!Reader.IsValid())
		{
			LogError("Could not open template archive for %s [%s]", *(TemplateInfo.Name), *(TemplateInfo.Path));
			return false;
		}

		auto TemplateFilename = TemplateInfo.Name + ".template";
		auto Names = Reader.GetFileNames();
	
		if (!Reader.TryReadFile(TemplateFilename, Contents))
		{
			LogError("Could not read contents of archive for %s [%s]", *(TemplateInfo.Name), *(TemplateInfo.Path));
			return false;
		}
	}

	return UUGCTemplateDescriptor::Load(Contents, TemplateInfo.Descriptor);
}

bool UUGCTemplateSubsystem::ExtractFilesFromTemplate(class FZipArchiveReader& Reader, FString DestinationDirectory, const TMap<FString, FString>& Substitutions)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	for (FString& EmbeddedFileName : Reader.GetFileNames())
	{
		if (FPaths::GetExtension(EmbeddedFileName, true).Equals(".template"))
		{
			//skip the template file
			continue;
		}

		TArray<uint8> Contents;
		if (!Reader.TryReadFile(EmbeddedFileName, Contents))
		{
			LogError("Could not read file %s from template archive", *EmbeddedFileName);
			return false;
		}

		if (Contents.IsEmpty())
		{
			//TODO: Assume this is a directory for the moment and continue
			continue;
		}

		FString FilePath = FPaths::Combine(DestinationDirectory, FPaths::GetPath(EmbeddedFileName));
		FString SubstitutedDestination = ResolveSubstitutions(FilePath, Substitutions);
		FString DestinationFilename = FPaths::Combine(SubstitutedDestination, FPaths::GetCleanFilename(EmbeddedFileName));
	
		FString FileExtension = FPaths::GetExtension(DestinationFilename);
		if (FileExtension.Equals("uasset") || FileExtension.Equals("umap")) 
		{
			AddFileForRename(DestinationFilename);
		}

		bool bSuccessfulWrite = FFileHelper::SaveArrayToFile(Contents, *DestinationFilename);
		if (!bSuccessfulWrite)
		{
			LogError("Could not write file %s to %s", *EmbeddedFileName, *DestinationFilename);
			return false;
		}
		LogInfo("Writing file %s", *DestinationFilename);
		Transaction->Add(UTransactionStepCreateFile::Make(DestinationFilename));
	}

	return true;
}

bool UUGCTemplateSubsystem::ExtractChildTemplateItems(const FUGCPluginInfo& Mod,
													  const TArray<FChildTemplateEntry>& ChildTemplates,
													  const TMap<FString, FString>& Substitutions)
{
	LogInfo("Extracting child templates");
	bool bSuccess = true;
	for (auto Child : ChildTemplates)
	{
		LogInfo("Extracting child template %s [%s]", *(Child.DisplayName), *(Child.TemplateName));
		FUGCTemplateInfo ChildTemplate;
		if (FindTemplateInfo(Child.TemplateName, ChildTemplate))
		{
			IncludeStack.Push(Child);
			bSuccess &= AddUGCTemplateItemTo_Internal(Mod, ChildTemplate, Substitutions);
			IncludeStack.Pop();
		}
		else
		{
			LogError("Could not find template info for template %s", *(Child.TemplateName));
			return false;
		}
	}
	return bSuccess;
}

bool UUGCTemplateSubsystem::CreateChildObjects(const FUGCPluginInfo& Mod,
											   const TArray<struct FUGCChildObject>& ChildObjects,
											   const TMap<FString, FString>& Substitutions)
{
	FString PluginRootPackagePath;
	bool bHasChildObjectPath = GetChildObjectPackageName(Mod, PluginRootPackagePath);
	if (!bHasChildObjectPath)
	{
		return false;
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IAssetTools& AssetTools = AssetToolsModule.Get();
	UDataAssetFactory* DataAssetFactory = NewObject<UDataAssetFactory>();
	if (DataAssetFactory == nullptr)
	{
		LogError("Could not create Data Asset Factory");
		return false;
	}

	TArray<UPackage*> ChildObjectPackages;
	for (auto& ChildObject : ChildObjects)
	{
		FString DataAssetName = ResolveSubstitutions(ChildObject.Name, Substitutions);
		UObject* CreatedObject =
			AssetTools.CreateAsset(DataAssetName, PluginRootPackagePath, ChildObject.Class, DataAssetFactory);
		if (CreatedObject != nullptr)
		{
			ChildObjectPackages.Add(CreatedObject->GetPackage());
		}
		else
		{
			LogError("Could not create child asset %s at %s", *DataAssetName, *PluginRootPackagePath);
			return false;
		}
	}

	UEditorLoadingAndSavingUtils::SavePackages(ChildObjectPackages, false);

	return true;
}

void UUGCTemplateSubsystem::VerifyTemplates(TArray<FUGCTemplateInfo>& Templates, TArray<FText>& Errors) 
{
	// Verify Templates
	//	Child templates exist
	//	Child templates are item templates not mod templates
	//	No loops in children

	for (auto Template : Templates)
	{
		for (auto Child : Template.Descriptor->ChildTemplates)
		{
			auto* ChildTemplate = Templates.FindByPredicate([Child](const FUGCTemplateInfo& TemplateInfo)
				{
					return Child.TemplateName == TemplateInfo.Name;
				});

			if (ChildTemplate == nullptr)
			{
				//TODO: Remove invalid template?
				Errors.Add(FText::FromString("Could not find child template " + Child.TemplateName));
				continue;
			}

			if (ChildTemplate->Descriptor->Type != EUGCTemplateType::TT_Item)
			{
				//TODO: Remove invalid templates?
				Errors.Add(FText::FromString("Child templates need to be Item Templates"));
				continue;
			}
		}
	}
}

bool UUGCTemplateSubsystem::IsValidModName(FString ModName, FString& FailReason)
{
	FText IllegalPluginNameReason;
	if (!FPluginUtils::IsValidPluginName(ModName, &IllegalPluginNameReason))
	{
		FailReason = IllegalPluginNameReason.ToString();
		return false;
	}

	return true;
}

void UUGCTemplateSubsystem::GatherSubsitutionsFor(const FUGCTemplateInfo& TemplateInfo,
												   TSet<FString>& Subtitutions, FString SubtitutionRoot)
{
	FString SubtitutionBase = (!SubtitutionRoot.IsEmpty() ? SubtitutionRoot + "." : "");
	for (auto Sub : TemplateInfo.Descriptor->SubstitutionParameters)
	{
		FString NewSub = SubtitutionBase + Sub;
		bool bAlreadyContains = false;
		Subtitutions.Add(NewSub, &bAlreadyContains);
		if (bAlreadyContains)
		{
			LogWarning("Substitution parameter %s is already defined", *NewSub);
			continue;
		}
	}

	//for (auto Child : TemplateInfo.Descriptor->ChildTemplates)
	//{
	//	auto* ChildTemplate = CachedTemplates.FindByPredicate([Child](const FUGCTemplateInfo& TemplateInfo)
	//		{
	//			return Child.TemplateName == TemplateInfo.Name; 
	//		});
	//
	//	if (ChildTemplate == nullptr)
	//	{
	//		LogWarning("Could not gather substitutions for child template %s", *(Child.TemplateName));
	//		continue;
	//	}
	//
	//	GatherSubsitutionsFor(*ChildTemplate, Subtitutions, SubtitutionBase + Child.DisplayName);
	//
	//}
}

FString UUGCTemplateSubsystem::ResolveSubstitutions(FString Base, const TMap<FString, FString>& Subsitutions)
{
	return ResolveSubstitutionsWithBase(Base, Subsitutions, GetCurrentSubstitutionBase());
}

FString UUGCTemplateSubsystem::ResolveSubstitutionsWithBase(FString Base, const TMap<FString, FString>& Subsitutions, FString SubBase)
{
	FString SubsitutedName = Base;
	FString SubstitutionBase = SubBase;

	// Substitute file names
	for (auto Substitution : Subsitutions)
	{
		FString SubKey = Substitution.Key;
		if (SubKey.RemoveFromStart(SubstitutionBase))
		{
			SubKey.RemoveFromStart(".");
		}

		SubsitutedName.ReplaceInline(*(SubKey), *(Substitution.Value), ESearchCase::CaseSensitive);
	}

	return SubsitutedName;
}

bool UUGCTemplateSubsystem::FindTemplateInfo(FString TemplateName, FUGCTemplateInfo& TemplateInfo)
{
	auto* ChildTemplate = CachedTemplates.FindByPredicate(
		[TemplateName](const FUGCTemplateInfo& TemplateInfo) { return TemplateName == TemplateInfo.Name; });

	if (ChildTemplate == nullptr)
	{
		return false;
	}

	TemplateInfo = *ChildTemplate;
	return true;
}

FString UUGCTemplateSubsystem::GetCurrentSubstitutionBase() const 
{
	TArray<FString> BaseParts;
	for (auto& Child : IncludeStack)
	{
		BaseParts.Add(Child.DisplayName);
	}

	return FString::Join(BaseParts, TEXT("."));
}

bool UUGCTemplateSubsystem::GetChildObjectPackageName(const FUGCPluginInfo& Mod, FString& OutPackageName)
{
	FString PluginContentDir = Mod.Plugin->GetContentDir();

	if (PluginContentDir.IsEmpty())
	{
		LogError("Mod plugin %s does not have a Content folder", *(Mod.Name));
		return false;
	}

	FString PluginRootPackagePath;
	FString FailureReason;
	bool bSuccessfulFilenameConversion = FPackageName::TryConvertFilenameToLongPackageName(PluginContentDir, PluginRootPackagePath, &FailureReason);
	if (!bSuccessfulFilenameConversion)
	{
		LogError("Could not convert filename %s to package name", *PluginContentDir);
		LogError(FailureReason);
		return false;
	}

	// Ensure path ends with a slash
	if (!PluginRootPackagePath.EndsWith(TEXT("/")))
	{
		PluginRootPackagePath += TEXT("/");
	}
	
	OutPackageName = PluginRootPackagePath;
	return true;
}

bool UUGCTemplateSubsystem::RenameAssets(const TMap<FString, FString>& Substitutions)
{
	TArray<FAssetRenameData> AssetRenameData;

	if (FilesToRename.Num() > 0)
	{
		LogInfo("Renaming %i assets", FilesToRename.Num());
		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
		AssetRegistry.ScanFilesSynchronous(FilesToScan, true);

		TMap<TWeakObjectPtr<UObject>, FString> AssetToOriginalFilePathMap;
		for (const FFileRenameData& FileData : FilesToRename)
		{
			FString File = FileData.File;
			TArray<FAssetData> Assets;

			FString PackageName;
			if (FPackageName::TryConvertFilenameToLongPackageName(File, PackageName))
			{
				AssetRegistry.GetAssetsByPackageName(*PackageName, Assets);
			}

			for (FAssetData AssetData : Assets)
			{
				const FString AssetName = ResolveSubstitutionsWithBase(AssetData.AssetName.ToString(), Substitutions, FileData.SubstitutionBase); // Need to figure out the substitution base now this is happening at once
				const FString AssetPath = ResolveSubstitutionsWithBase(AssetData.PackagePath.ToString(), Substitutions, FileData.SubstitutionBase);

				if (AssetName != AssetData.AssetName)
				{
					AssetToOriginalFilePathMap.Add(AssetData.GetAsset(), File);
					FAssetRenameData RenameData(AssetData.GetAsset(), AssetPath, AssetName);

					AssetRenameData.Add(RenameData);
					FString NewFileName = ResolveSubstitutionsWithBase(File, Substitutions, FileData.SubstitutionBase);
					Transaction->Add(UTransactionStepRenameFile::Make(File, NewFileName));
					LogInfo("Renaming %s to %s", *File, *NewFileName);

					if (UObject* Asset = AssetData.GetAsset())
					{
						Asset->Modify();
						TextNamespaceUtil::ClearPackageNamespace(Asset);
						TextNamespaceUtil::EnsurePackageNamespace(Asset);
					}
				}
			}
		}

		if (AssetRenameData.Num() > 0)
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
			bool bSuccessfulRename = AssetToolsModule.Get().RenameAssets(AssetRenameData);
			if (!bSuccessfulRename)
			{
				LogError("Some assets were not able to be renamed");
			}
		}
	}

	FilesToScan.Empty();
	FilesToRename.Empty();
	return true;
}

void UUGCTemplateSubsystem::BroadcastLog(ELogVerbosity::Type Verbosity, FString Message)
{
	FString OutputString(ToString(Verbosity));
	OutputString.Append(": ");
	OutputString.Append(Message);
	OutputString.Append("\n");
	OnTemplateLogMessage.Broadcast(OutputString);
}

void UUGCTemplateSubsystem::AddFileForRename(FString Filename)
{
	FilesToScan.Add(Filename);
	FilesToRename.Add({GetCurrentSubstitutionBase(), Filename});
}

void UUGCTemplateSubsystem::FixAssetReferences(const FUGCTemplateInfo& TemplateInfo, const TSharedPtr<IPlugin>& Plugin)
{
	if (!Plugin.IsValid())
	{
		return;
	}

	const FString PluginName = Plugin->GetName();
	const FString PluginMountPath = Plugin->GetMountedAssetPath();

	UE_LOG(LogModioUGCTemplates, Log, TEXT("Fixing references for plugin: %s (%s)"), *PluginName, *PluginMountPath);

	TArray<FAssetData> PluginAssets;
	FindPluginAssets(PluginMountPath, PluginAssets);

	if (PluginAssets.IsEmpty())
	{
		UE_LOG(LogModioUGCTemplates, Warning, TEXT("No assets found under %s"), *PluginMountPath);
		return;
	}

	AddPluginPackageRedirects(TemplateInfo, Plugin, PluginAssets);

	// Load the assets so that they can deserialize and encounter old references that we're redirecting. This should fix
	// up in-memory references to point to the new locations. We'll save the assets after this to fix up on disk
	// references as well.
	for (const FAssetData& AD : PluginAssets)
	{
		UE_LOG(LogModioUGCTemplates, Log, TEXT("Loading asset: %s"), *AD.ObjectPath.ToString());

		UObject* Loaded = AD.GetAsset();
		if (!Loaded)
		{
			UE_LOG(LogModioUGCTemplates, Warning, TEXT("Failed to load asset: %s"), *AD.ObjectPath.ToString());
		}
	}

	SaveAssets(PluginAssets);

	FCoreRedirects::RemoveRedirectList(AssetsToRedirect, TEXT("TemplateAssetReferencesFixup"));
	AssetsToRedirect.Empty();

	UE_LOG(LogModioUGCTemplates, Log, TEXT("Reference fixup complete for plugin: %s"), *PluginName);
}

void UUGCTemplateSubsystem::FindPluginAssets(const FString& PluginMountPath, TArray<FAssetData>& OutAssets)
{
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);

	TArray<FString> PathsToScan;
	PathsToScan.Add(*PluginMountPath);

	ARM.Get().ScanPathsSynchronous(PathsToScan, /*bForceRescan=*/true);


	FARFilter Filter;
	Filter.PackagePaths.Add(*PluginMountPath);
	Filter.bRecursivePaths = true;

	ARM.Get().GetAssets(Filter, OutAssets);
}

void UUGCTemplateSubsystem::AddPluginPackageRedirects(const FUGCTemplateInfo& TemplateInfo,
													  const TSharedPtr<IPlugin>& Plugin,
													  const TArray<FAssetData>& PluginAssets)
{
	const FString RootPath = Plugin->GetMountedAssetPath();
	const FString ContentPath = RootPath / "Content/";

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();

	for (const FAssetData& AD : PluginAssets)
	{
		TArray<FName> Dependencies;
		AssetRegistry.GetDependencies(AD.PackageName, Dependencies);
		for (const FName& Dep : Dependencies)
		{
			FString DepString = Dep.ToString();
			// UE_LOG(LogModioUGCTemplates, Log, TEXT("Asset %s depends on %s"), *AD.AssetName.ToString(), *DepString);
			//  Check if this asset has a dependency from the game contents
			if (DepString.StartsWith(TEXT("/Game/")))
			{
				// Check if the dependency is from the template's content folder
				FString SubString = FString::Printf(TEXT("/%s/Content/"), *TemplateInfo.Name);
				int32 ContentIndex = DepString.Find(SubString, ESearchCase::CaseSensitive);
				if (ContentIndex != INDEX_NONE)
				{
					UE_LOG(LogModioUGCTemplates, Log,
						   TEXT("Asset '%s' has dependency '%s' that needs to be redirected"), *AD.AssetName.ToString(),
						   *DepString);

					FString OldContentPath =
						DepString.Left(ContentIndex + SubString.Len()); // +Length to include "/<TemplateName>/Content"
					FString OldAssetPath = DepString.RightChop(
						OldContentPath.Len()); // The rest of the path after "/<TemplateName>/Content/"

					// Construct our package mapping from the old location to the new location
					FString OldPackage = DepString; // e.g. "/Game/TemplateSources/MyTemplate/Content/SomeActor"
					FString NewPackage = RootPath / OldAssetPath; // e.g. "/<PluginMount>/Any/Sub/Directory/SomeActor"
					UE_LOG(LogModioUGCTemplates, Log, TEXT("Registering redirect: %s -> %s"), *OldPackage, *NewPackage);

					AssetsToRedirect.Emplace(ECoreRedirectFlags::Type_Package, OldPackage, NewPackage);
				}
			}
		}
	}

	FCoreRedirects::AddRedirectList(AssetsToRedirect, TEXT("TemplateAssetReferencesFixup"));
}

void UUGCTemplateSubsystem::SaveAssets(const TArray<FAssetData>& Assets)
{
	for (const FAssetData& AD : Assets)
	{
		UPackage* Package = AD.GetPackage();
		if (!Package || !Package->IsDirty())
		{
			continue;
		}

		UE_LOG(LogModioUGCTemplates, Log, TEXT("Saving package: %s"), *Package->GetName());

		const FString Filename =
			FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		bool bSuccessfulSave = UPackage::SavePackage(Package, nullptr, *Filename, SaveArgs);
		if (!bSuccessfulSave)
		{
			UE_LOG(LogModioUGCTemplates, Log, TEXT("Failed to save %s"), *Package->GetName());
		}
	}
}