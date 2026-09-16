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
#include "Widgets/SCompoundWidget.h"
#include "../UGCTemplateSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"

/**
 * 
 */
class MODIOUGCEDITOR_API SUGCTemplateSubsitutionWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUGCTemplateSubsitutionWidget)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void BuildFor(const TSharedPtr<FUGCTemplateInfo> Template);
	void BuildFor(const FUGCTemplateInfo& Template);

	TSharedPtr<SButton> ConfirmButton;
	TSharedPtr<SVerticalBox> SubContainer;
	TSharedPtr<SWindow> ParentWindow;

	UPROPERTY()
	TMap<FString, TSharedPtr<class SEditableTextBox>> SubWidgetMapping;

	void GetSubs(TMap<FString, FString>& OutSubs);

private:

	FSlateBrush* BackgroundBrush;
	FSlateBrush* PanelBackgroundBrush;
	FSlateBrush* HeaderBackgroundBrush;
};
