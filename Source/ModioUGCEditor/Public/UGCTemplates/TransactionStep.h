// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TransactionStep.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class MODIOUGCEDITOR_API UTransactionStep : public UObject
{
	GENERATED_BODY()
	
public:

	virtual void Revert() PURE_VIRTUAL(UTransactionStep::Revert, );

};

/**
 *
 */
UCLASS()
class MODIOUGCEDITOR_API UTransactionStepCreateFile : public UTransactionStep
{
	GENERATED_BODY()

public:
	virtual void Revert() override;

	static UTransactionStepCreateFile* Make(FString InFilename);

private:

	FString Filename;
};

/**
 *
 */
UCLASS()
class MODIOUGCEDITOR_API UTransactionStepCreatePlugin : public UTransactionStep
{
	GENERATED_BODY()

public:
	virtual void Revert() override;

	static UTransactionStepCreatePlugin* Make(TSharedPtr<class IPlugin> InPlugin);

private:

	TSharedPtr<class IPlugin> Plugin;
};

/**
 *
 */
UCLASS()
class MODIOUGCEDITOR_API UTransactionStepRenameFile : public UTransactionStep
{
	GENERATED_BODY()

public:
	virtual void Revert() override;

	static UTransactionStepRenameFile* Make(FString InOldName, FString InNewName);

private:
	
	FString OldName;
	FString NewName;

};