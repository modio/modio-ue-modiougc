// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Transaction.generated.h"


/**
 * 
 */
UCLASS()
class MODIOUGCEDITOR_API UTransaction : public UObject
{
	GENERATED_BODY()
	
public:

	void Begin();
	void Finish();
	void Cancel();

	int32 Add(class UTransactionStep* Step);

private:

	void ClearBuffer();

private:

	UPROPERTY()
	TArray<class UTransactionStep*> Buffer;

};
