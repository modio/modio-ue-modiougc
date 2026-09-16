// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../../Slate/Public/Widgets/Input/SSegmentedControl.h"
#include "../UGCTemplateDescriptor.h"
#include "Interfaces/IPluginManager.h"


/**
 * 
 */
class MODIOUGCEDITOR_API SModioExportUGCTemplateWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SModioExportUGCTemplateWidget)
	{}
	SLATE_ARGUMENT(TSharedPtr<IPlugin>, Context)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:

	void OnTemplateTypeChanged(EUGCTemplateType Value);
	TSharedRef<SWidget> CreateTypeTabs();

	FReply OnCreateClicked();
	FReply OnCancelClicked();

	UUGCTemplateDescriptor* MakeTemplateDescriptor(class UUGCTemplateExportDetails* InExportDetails);

private:

	EUGCTemplateType Mode;
	class UUGCTemplateExportDetails* ExportDetails;

	TSharedPtr<class IDetailsView> TemplateExportDetailsView;

public:
	bool IsCreateEnabled() const;
};
