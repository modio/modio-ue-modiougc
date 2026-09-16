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
#include "Editor/PropertyEditor/Public/IDetailCustomization.h"
#include "Interfaces/IPluginManager.h"
#include "Types/SlateEnums.h"
#include "UGCTemplates/UGCTemplateSubsystem.h"
#include "UGCTemplateSettings.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SWidget.h"

/**
 * 
 */
class FUGCTemplateExportCustomisation : public IDetailCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();
	
	void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	void UpdatePluginSource();
	void UpdateCategorySource();
	TSharedRef<SWidget> GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item);
	TSharedRef<SWidget> GenerateCatgoryComboBoxWidget(TSharedPtr<FUGCTemplateCategoryView> Item);
	TSharedRef<SWidget> GenerateIconSelectorWidget(FString BasePath, FString TargetPath);
	TSharedRef<SWidget> GenerateCategoryWidgetContent();

	void OnModSelectionChanged(TSharedPtr<FUGCPluginInfo> Item, ESelectInfo::Type SelectType);
	void OnCategorySelectionChanged(TSharedPtr<FUGCTemplateCategoryView> Item, ESelectInfo::Type SelectType);
	

	TSharedPtr<class SComboBox<TSharedPtr<FUGCPluginInfo>>> ModSelectionBox;
	TSharedPtr<class SComboBox<TSharedPtr<FUGCTemplateCategoryView>>> CategorySelectionBox;
	TSharedPtr<class SBox> IconSelectorBox;

	TArray<TSharedPtr<FUGCPluginInfo>> UGCPluginSource;
	TArray<TSharedPtr<FUGCTemplateCategoryView>> CategorySource;

	class UUGCTemplateExportDetails* DetailsBeingCustomised = nullptr;

private:

	TSharedPtr<FUGCPluginInfo> GetPluginInfoFrom(TSharedPtr<IPlugin> Context);
};
