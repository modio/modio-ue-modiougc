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

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ModioUGCEditorStyle.h"

class FModioUGCEditorCommands : public TCommands<FModioUGCEditorCommands>
{
public:
	FModioUGCEditorCommands()
		: TCommands<FModioUGCEditorCommands>(TEXT("ModioUGCEditor"),
											 NSLOCTEXT("Contexts", "ModioUGCEditor", "ModioUGCEditor Plugin"),
											 NAME_None, FModioUGCEditorStyle::GetStyleSetName())
	{}

	// TCommands<> interface
	virtual void RegisterCommands() override;

	TArray<TSharedPtr<FUICommandInfo>> RegisterUGCCommands(const TArray<TSharedRef<class IPlugin>>& UGCList) const;
	void UnregisterUGCCommands(TArray<TSharedPtr<FUICommandInfo>>& UICommands) const;

public:
	TSharedPtr<FUICommandInfo> CreateUGCAction;
	TSharedPtr<FUICommandInfo> PackageUGCAction;
};
