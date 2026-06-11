// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "UObject/CoreRedirects.h"
#include "UGCTemplateSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogModioUGCTemplates, Log, All);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTemplateLogMessage, FString);

USTRUCT(BlueprintType)
struct FUGCTemplateInfo
{
	GENERATED_BODY();
public:
	
	UPROPERTY(BlueprintReadOnly, Category = "mod.io|Mods|Templates")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "mod.io|Mods|Templates")
	FString Path;

	UPROPERTY(BlueprintReadOnly, Category = "mod.io|Mods|Templates")
	class UUGCTemplateDescriptor* Descriptor;
};

USTRUCT(BlueprintType)
struct FUGCPluginInfo
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, Category = "mod.io|Mods|Templates")
	FString Name;

	TSharedPtr<IPlugin> Plugin;
};

USTRUCT()
struct FFileRenameData
{
	GENERATED_BODY()

public:

	FString SubstitutionBase;
	FString File;

};

/**
 * 
 */
UCLASS(BlueprintType)
class MODIOUGCEDITOR_API UUGCTemplateSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	/** Implement this for initialization of instances of the system */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Implement this for deinitialization of instances of the system */
	virtual void Deinitialize() override;

	static FString FormatLogMessageInternal(const TCHAR* Format, ...);

	//DiscoverTemplates
	//// Search for templates from Template Sources (TBD)
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	void DiscoverTemplates(TArray<FUGCTemplateInfo>& Templates);

	//DiscoverUGC
	//// Search for UGC plugins
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	void DiscoverUGC(TArray<FUGCPluginInfo>& UGCPlugins);

	//CreateFromTemplate
	//// Read contents of zip
	//// Create copies of files at destination
	//// Load new plugin
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	bool CreateUGCFromTemplate(FString NewModName, const FUGCTemplateInfo& TemplateInfo, const TMap<FString, FString>& Substitutions);

	//AddTemplateItem
	//// Read contents from file
	//// Add new files to existing mod
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	bool AddUGCTemplateItemTo(const FUGCPluginInfo& Mod, const FUGCTemplateInfo& TemplateInfo, const TMap<FString, FString>& Substitutions);
	
	//ExportTemplate
	//// Create Zip from selected plugin
	//// Add to Template Source directory
	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	bool ExportUGCTemplate(const FUGCPluginInfo& Mod, class UUGCTemplateDescriptor* TemplateDescriptor, bool bAutoReplaceName = false);

	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	void GetTemplates(TArray<FUGCTemplateInfo>& Templates) const;

	UFUNCTION(BlueprintCallable, Category = "mod.io|Mods|Templates")
	void GetSubstitutionsFor(const FUGCTemplateInfo& TemplateInfo, TSet<FString>& Subtitutions);

	//UPROPERTY(BlueprintAssignable)
	FOnTemplateLogMessage OnTemplateLogMessage;

	bool IsValidModName(FString ModName, FString& FailReason);

private:

	FString GetTemplateDirectory() const;
	bool AddUGCTemplateItemTo_Internal(const FUGCPluginInfo& Mod, const FUGCTemplateInfo& TemplateInfo, const TMap<FString, FString>& Substitutions);
	bool ExtractTemplateDescriptor(FUGCTemplateInfo& TemplateInfo);
	bool ExtractFilesFromTemplate(class FZipArchiveReader& Reader, FString DestinationDirectory, const TMap<FString, FString>& Substitutions);
	bool ExtractChildTemplateItems(const FUGCPluginInfo& Mod, const TArray<struct FChildTemplateEntry>& ChildTemplates, const TMap<FString, FString>& Substitutions);
	bool CreateChildObjects(const FUGCPluginInfo& Mod, const TArray<struct FUGCChildObject>& ChildObjects, const TMap<FString, FString>& Substitutions);

	void VerifyTemplates(TArray<FUGCTemplateInfo>& Templates, TArray<FText>& Errors);

	void GatherSubsitutionsFor(const FUGCTemplateInfo& TemplateInfo, TSet<FString>& Subtitutions, FString SubtitutionRoot);
	FString ResolveSubstitutions(FString Base, const TMap<FString, FString>& Subsitutions);
	FString ResolveSubstitutionsWithBase(FString Base, const TMap<FString, FString>& Subsitutions, FString SubBase);

	bool FindTemplateInfo(FString TemplateName, FUGCTemplateInfo& TemplateInfo);
	FString GetCurrentSubstitutionBase() const;

	bool GetChildObjectPackageName(const FUGCPluginInfo& Mod, FString& OutPackageName);

	void FixAssetReferences(const FUGCTemplateInfo& TemplateInfo, const TSharedPtr<IPlugin>& Plugin);
	void FindPluginAssets(const FString& PluginMountPath, TArray<FAssetData>& OutAssets);
	void AddPluginPackageRedirects(const FUGCTemplateInfo& TemplateInfo, const TSharedPtr<IPlugin>& Plugin,
								   const TArray<FAssetData>& PluginAssets);
	void SaveAssets(const TArray<FAssetData>& Assets);

	void AddFileForRename(FString Filename);
	bool RenameAssets(const TMap<FString, FString>& Substitutions);

	template<typename... Args>
	void LogInfo(const FString& Format, Args&&... args);

	template<typename... Args>
	void LogWarning(const FString& Format, Args&&... args);

	template<typename... Args>
	void LogError(const FString& Format, Args&&... args);

	template<typename... Args>
	static FString FormatLogMessage(const FString& Format, Args&&... args);

	void BroadcastLog(ELogVerbosity::Type Verbosity, FString Message);

private:
	
	UPROPERTY()
	TArray<FUGCTemplateInfo> CachedTemplates;

	UPROPERTY()
	class UTransaction* Transaction = nullptr;
	
	TArray<struct FChildTemplateEntry> IncludeStack;
	TArray<FString> FilesToScan;
	TArray<FFileRenameData> FilesToRename;

	TArray<FCoreRedirect> AssetsToRedirect;
};

template<typename... Args>
FString UUGCTemplateSubsystem::FormatLogMessage(const FString& Format, Args&&... args)
{
	FString FormattedMessage;
	if (Format.Len() > 0)
	{
		return FormatLogMessageInternal(*Format, std::forward<Args>(args)...);
	}
	return FString();
}

template<typename... Args>
void UUGCTemplateSubsystem::LogInfo(const FString& Format, Args&&... args)
{
	auto FormattedMessage = FormatLogMessage(Format, std::forward<Args>(args)...);
	UE_LOG(LogModioUGCTemplates, Log, TEXT("%s"), *FormattedMessage);
	BroadcastLog(ELogVerbosity::Log, FormattedMessage);
}

template<typename... Args>
void UUGCTemplateSubsystem::LogWarning(const FString& Format, Args&&... args)
{
	auto FormattedMessage = FormatLogMessage(Format, std::forward<Args>(args)...);
	UE_LOG(LogModioUGCTemplates, Warning, TEXT("%s"), *FormattedMessage);
	BroadcastLog(ELogVerbosity::Warning, FormattedMessage);
}

template<typename... Args>
void UUGCTemplateSubsystem::LogError(const FString& Format, Args&&... args)
{
	auto FormattedMessage = FormatLogMessage(Format, std::forward<Args>(args)...);
	UE_LOG(LogModioUGCTemplates, Error, TEXT("%s"), *FormattedMessage);
	BroadcastLog(ELogVerbosity::Error, FormattedMessage);
}