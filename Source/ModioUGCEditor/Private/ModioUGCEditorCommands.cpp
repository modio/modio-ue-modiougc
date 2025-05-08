/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCEditorCommands.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FModioUGCEditorModule"

void FModioUGCEditorCommands::RegisterCommands()
{
	UI_COMMAND(CreateUGCAction, "Create UGC", "Create a new UGC package", EUserInterfaceActionType::Button,
			   FInputGesture());
	UI_COMMAND(PackageUGCAction, "Package UGC", "Share and distribute your UGC", EUserInterfaceActionType::Button,
			   FInputGesture());
}

TArray<TSharedPtr<FUICommandInfo>> FModioUGCEditorCommands::RegisterUGCCommands(
	const TArray<TSharedRef<class IPlugin>>& UGCList) const
{
	TArray<TSharedPtr<FUICommandInfo>> AvailableUGCActions;
	AvailableUGCActions.Reserve(UGCList.Num());

	FModioUGCEditorCommands* MutableThis = const_cast<FModioUGCEditorCommands*>(this);

	for (int32 Index = 0; Index < UGCList.Num(); ++Index)
	{
		AvailableUGCActions.Add(TSharedPtr<FUICommandInfo>());
		TSharedRef<IPlugin> UGC = UGCList[Index];

		FString CommandName = "UGCEditorUGC_" + UGC->GetName();

		FUICommandInfo::MakeCommandInfo(MutableThis->AsShared(), AvailableUGCActions[Index], FName(*CommandName),
										FText::FromString(UGC->GetName()), FText::FromString(UGC->GetBaseDir()),
										FSlateIcon(), EUserInterfaceActionType::Button, FInputGesture());
	}

	return AvailableUGCActions;
}

void FModioUGCEditorCommands::UnregisterUGCCommands(TArray<TSharedPtr<FUICommandInfo>>& UICommands) const
{
	FModioUGCEditorCommands* MutableThis = const_cast<FModioUGCEditorCommands*>(this);

	for (TSharedPtr<FUICommandInfo> Command : UICommands)
	{
		FUICommandInfo::UnregisterCommandInfo(MutableThis->AsShared(), Command.ToSharedRef());
	}
}

#undef LOCTEXT_NAMESPACE
