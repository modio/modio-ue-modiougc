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
#include "Interfaces/IPluginManager.h"
#include "UObject/NoExportTypes.h"
#include "UGCTemplateSubsystem.h"
#include "UGCTemplateExportDetails.generated.h"

/**
 * 
 */
UCLASS()
class MODIOUGCEDITOR_API UUGCTemplateExportDetails : public UObject
{
	GENERATED_BODY()

public:

	UUGCTemplateExportDetails();

	UPROPERTY(EditDefaultsOnly, Category = "Mod Template Details")
	FText Name;

	UPROPERTY(EditDefaultsOnly, Category = "Mod Template Details")
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, Category = "Mod Template Version")
	FString Version;

	//UPROPERTY(EditDefaultsOnly, Category = "Mod Template Version", meta=(DisplayName="Is Beta?"))
	bool bIsBeta = false;

	UPROPERTY(EditDefaultsOnly, Category = "Parameters")
	TSet<FString> SubstitutionParameters;

	UPROPERTY(VisibleDefaultsOnly, Category = "Export Settings")
	FDirectoryPath Directory;

	UPROPERTY()
	FUGCPluginInfo ModToExport;

	UPROPERTY()
	FString Category;

	TSharedPtr<IPlugin> Context = nullptr;

};
