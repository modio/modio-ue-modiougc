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
#include "Interfaces/IPluginManager.h"
#include "../UGCTemplateSubsystem.h"
#include "../UGCTemplateDescriptor.h"
#include "SUGCTemplateCategoryTile.h"
#include "../UGCTemplateSettings.h"
#include "../../Slate/Public/Widgets/Views/STileView.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandList.h"
#include "Types/SlateEnums.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"

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
	{
	}
	SLATE_ARGUMENT(EUGCTemplateType, Mode)
	SLATE_ARGUMENT(TSharedPtr<IPlugin>, Context)
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
	TSharedPtr<class SComboBox<TSharedPtr<FUGCPluginInfo>>> ModTemplateModSelectionBox;

	TSharedPtr<class IDetailsView> ExportDescriptorDetails;
	class UUGCTemplateDescriptor* ExportDescriptor;
	//TSharedPtr<class UUGCTemplateDescriptor> ExportDescriptor;

	TSharedPtr<SEditableTextBox> NewModNameInput;

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

	////
	TSharedRef<SWidget> CreateCatergoriesWidget();
	TSharedRef<SWidget> CreateTemplatesWidget();
	TSharedRef<SWidget> CreateDetailsWidget();
	TSharedRef<SWidget> CreateOptionsWidget();

	FReply OnCreateClicked();
	FReply OnCancelClicked();

	void LoadCategories();

	void OnListSelectionChanged(TSharedPtr<FUGCTemplateInfo> Item, ESelectInfo::Type SelectInfo);
	void OnCategoryClicked(TSharedPtr<FUGCTemplateCategoryView> CategoryTile, ESelectInfo::Type SelectInfo);

	TSharedPtr<SEditableTextBox> NewNameText;
	TSharedPtr<class SUGCTemplateDetailsWidget> TemplateDetailsWidget;
	TSharedPtr<class STileView<TSharedPtr<FUGCTemplateInfo>>> TemplateView;
	TArray<TSharedPtr<FUGCTemplateCategoryView>> Categories;

	EUGCTemplateType Mode = EUGCTemplateType::TT_Mod;
	TSharedPtr<FUGCTemplateInfo> SelectedTemplate;
	TArray<TSharedPtr<FUGCTemplateInfo>> FilteredTemplateOptions;

	TSharedRef<SWidget> GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item);
	TSharedPtr<FUGCPluginInfo> GetPluginInfoFrom(TSharedPtr<IPlugin> Context);

private:

	void SetDefaultTemplateSelection();

	void DisplayNewModNotification(bool bSuccess, const TArray<FString>& FocusPaths);
	void DisplayAddItemNotification(bool bSuccess, const TArray<FString>& FocusPaths);
};
