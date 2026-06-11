// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../UGCTemplateSubsystem.h"

class FModioEditorUGCTemplateWidgetCommands : public TCommands<FModioEditorUGCTemplateWidgetCommands>
{
public:
	FModioEditorUGCTemplateWidgetCommands();

	// TCommands<> overrides
	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> ClearLog;
};

/**
 * 
 */
class MODIOUGCEDITOR_API SModioEditorUGCTemplateWidget : public SCompoundWidget
{
public:
	SModioEditorUGCTemplateWidget();
	virtual ~SModioEditorUGCTemplateWidget() override;

	SLATE_BEGIN_ARGS(SModioEditorUGCTemplateWidget)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
	void TearDown();

private:

	void LoadResources();
	void UpdateSources();
	void NewPluginDetected(IPlugin& NewPlugin);
	FSlateFontInfo GetTextStyle(FName PropertyName, FName FaceName, int32 Size);

private:
	TSharedPtr<STextBlock> HeaderText;
	TSharedPtr<class SWidgetSwitcher> TemplateTypeSwitcher;
	TSharedPtr<class SComboBox<TSharedPtr<FUGCPluginInfo>>> ModTemplateModSelectionBox;
	TSharedPtr<class SComboBox<TSharedPtr<FUGCTemplateInfo>>> ItemTemplateModSelectionBox;

	TSharedPtr<class SComboBox<TSharedPtr<FUGCTemplateInfo>>> CreateModTemplateSelectionBox;

	TSharedPtr<class SComboBox<TSharedPtr<FUGCPluginInfo>>> TemplateExportSelectionBox;
	TSharedPtr<class IDetailsView> ExportDescriptorDetails;
	class UUGCTemplateDescriptor* ExportDescriptor;
	//TSharedPtr<class UUGCTemplateDescriptor> ExportDescriptor;

	TSharedPtr<SEditableTextBox> NewModNameInput;

	TSharedPtr<class SUGCTemplateSubsitutionWidget> CurrentSubMenu;

	TSharedPtr<class SExpandableArea> LogExpandableArea;
	TSharedPtr<class SMultiLineEditableTextBox> LogText;

	TArray<FUGCTemplateInfo> Templates;
	TArray<FUGCPluginInfo> UGCPlugins;
	TArray<TSharedPtr<FUGCTemplateInfo>> TemplatesOptions;
	TArray<TSharedPtr<FUGCTemplateInfo>> ItemTemplatesOptions;
	TArray<TSharedPtr<FUGCTemplateInfo>> ModTemplatesOptions;
	TArray<TSharedPtr<FUGCPluginInfo>> UGCPluginsOptions;

	FSlateBrush* HeaderBackgroundBrush;
	FSlateBrush* PanelBackgroundBrush;
	FSlateBrush* BackgroundBrush;
	FSlateFontInfo HeaderLargeTextStyle;
	FSlateFontInfo HeaderSmallTextStyle;
	FSlateFontInfo ButtonTextStyle;
	FSlateBrush* BoldSeperatorBrush;
	float PanelPadding = 2.5f;
	float BottomButtonPadding = 12;

	template<typename T>
	TArray<TSharedPtr<T>> ConvertToOptionsSource(TArray<T> Options)
	{
		TArray<TSharedPtr<T>> Source;
		for (auto& Option : Options)
		{
			Source.Add(MakeShareable(new T(Option)));
		}
		return Source;
	}

	void AddMessageToLog(FString Message);
	void ClearLog();

	TSharedRef<FUICommandList> LogCommandList;

	FDelegateHandle OnTemplateLogHandle;
	FDelegateHandle OnPluginCreatedHandle;
	FDelegateHandle OnPluginMountedHandle;

public:
	FReply ToModTemplates();
	FReply ToItemTemplates();
	FReply ToExportTemplate();
	FReply OnClickedAddItem();
	FReply OnClickedAddMod();
	FReply OnClickedExport();
	TSharedRef<SWidget> GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item);
	TSharedRef<SWidget> GenerateTemplateComboBoxWidget(TSharedPtr<FUGCTemplateInfo> Item);

	TSharedPtr<class SUGCTemplateSubsitutionWidget> GetSubMenuFor(TSharedPtr<FUGCTemplateInfo> Template);
};
