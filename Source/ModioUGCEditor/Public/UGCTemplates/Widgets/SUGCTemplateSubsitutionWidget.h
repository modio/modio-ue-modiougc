// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../UGCTemplateSubsystem.h"

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
