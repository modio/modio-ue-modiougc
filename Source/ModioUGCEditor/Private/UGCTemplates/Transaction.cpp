// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Transaction.h"
#include "UGCTemplates/TransactionStep.h"

void UTransaction::Begin()
{
	ensureMsgf(Buffer.Num() <= 0, TEXT("Trying to being new transaction while one is currently active"));
}

void UTransaction::Finish()
{
	ClearBuffer();
}

void UTransaction::Cancel()
{
	for (int32 i = Buffer.Num() - 1; i >= 0; --i)
	{
		Buffer[i]->Revert();
	}
	ClearBuffer();
}

int32 UTransaction::Add(UTransactionStep* Step) 
{
	if (Buffer.Contains(Step))
	{
		//log error
		return INDEX_NONE;
	}

	return Buffer.Add(Step);
}

void UTransaction::ClearBuffer()
{
	Buffer.Empty();
}
