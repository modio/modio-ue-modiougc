/*
 *  Copyright (C) 2024-2026 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue/blob/main/LICENSE>)
 *
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/NoExportTypes.h"
#include "UGCTemplateDescriptor.generated.h"

UENUM(BlueprintType)
enum class EUGCTemplateType: uint8
{
	TT_Item UMETA(DisplayName = "Item"),
	TT_Mod	UMETA(DisplayName = "Mod")
};

USTRUCT(BlueprintType)
struct FChildTemplateEntry
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString TemplateName;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString DisplayName;
};

USTRUCT(BlueprintType)
struct FUGCChildObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	TSubclassOf<UDataAsset> Class;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString Name;
};

/**
 * 
 */
UCLASS(BlueprintType)
class MODIOUGCEDITOR_API UUGCTemplateDescriptor : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "mod.io|Mods|Templates")
	int32 Version = CurrentVersion;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	EUGCTemplateType Type;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	TSet<FString> SubstitutionParameters;

	//UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FChildTemplateEntry> ChildTemplates; 

	//UPROPERTY(BlueprintReadOnly, EditAnywhere)
	TArray<FUGCChildObject> ChildObjects;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString Description = "Missing Description";

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString Category = "";

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "mod.io|Mods|Templates")
	FString TemplateVersion = "";

	static bool Load(TArray<uint8> Data, UUGCTemplateDescriptor* Descriptor);
	static bool Save(const UUGCTemplateDescriptor* Descriptor, TArray<uint8>& Data);

	static constexpr int32 CurrentVersion = 1;

	static const FString VersionFieldName;
	static const FString TypeFieldName;
	static const FString ParamArrayFieldName;
	static const FString ChildTemplatesFieldName;
	static const FString ChildObjectsFieldName;
	static const FString DescriptionFieldName;
	static const FString TemplateVersionFieldName;
	static const FString CategoryFieldName;

};
