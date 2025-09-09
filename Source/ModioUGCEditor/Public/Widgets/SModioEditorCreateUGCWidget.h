/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue/blob/main/LICENSE>)
 *
 */

#pragma once

#include "CoreMinimal.h"

#include "Features/IPluginsEditorFeature.h"
#include "IPluginWizardDefinition.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Input/SFilePathPicker.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SModioEditorWindowCompoundWidget.h"

/**
 *
 */
class MODIOUGCEDITOR_API SModioEditorCreateUGCWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SModioEditorCreateUGCWidget) {}
	SLATE_ARGUMENT(SModioEditorWindowCompoundWidget*, ParentWindow)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<IPluginWizardDefinition> InPluginWizardDefinition = nullptr);

private:
	SModioEditorWindowCompoundWidget* ParentWindow;

	/** The current plugin wizard definition */
	TSharedPtr<IPluginWizardDefinition> PluginWizardDefinition;

	FString GetModfilePath() const;

	void OnBackButtonPressed();

	void OnModNameTextChanged(const FText& InText);

	bool IsContentOnlyProject();

	FReply OnCreatePluginClicked();
	void DiscoveringAssetsCalback();

	FString PluginBaseDir;
	FString PluginFolderPath;
	FText ModNameText;
	FString CreatedBy;
	FString CreatedByURL;
	FString Description;
	bool bIsBetaVersion;

	bool bSelectInContentBrowser;
	bool bIsWaitingForAssetDiscoveryToFinish;
};
