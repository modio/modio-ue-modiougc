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
#include "DesktopPlatformModule.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SModioEditorWindowCompoundWidget.h"

/**
 *
 */
class MODIOUGCEDITOR_API SModioEditorPackageUGCWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SModioEditorPackageUGCWidget) {}
	SLATE_ARGUMENT(SModioEditorWindowCompoundWidget*, ParentWindow)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

private:
	TSharedPtr<class FModioUGCPackager> UGCPackager;

protected:
	/** Gets the output package path (directory to export the packaged plugin to). */
	FText GetOutputPackagePath() const;

	/** Generates the widget for the combo box with string items. */
	TSharedRef<SWidget> GenerateStringComboBoxWidget(TSharedPtr<FString> Item);

	/** Generates the widget for the combo box with plugin items. */
	TSharedRef<SWidget> GeneratePluginComboBoxWidget(TSharedPtr<IPlugin> Item);

	/** Gets the selected platform name (e.g. Windows, Android, etc.). */
	FText GetSelectedPlatformName() const;

	void OnPluginSelected(TSharedPtr<IPlugin> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnPlatformSelected(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo);
	void OnOutputPackagePathTextChanged(const FString& InText);

	/** Executes when the package path browse button is clicked (opens a directory picker). */
	FReply HandleOnPackagePathBrowseButtonClicked();

	/** Executes when the package button is pressed (and initiates the packaging process). */
	FReply HandleOnPackageButtonPressed();

protected:
	/** The plugin combo box (shows all available Game Features-based plugins). */
	TSharedPtr<SComboBox<TSharedPtr<IPlugin>>> PluginComboBox;

	/** The platform combo box (e.g. Windows, Android, etc.). */
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PlatformComboBox;

	/** The package path input (directory to export the packaged plugin to). */
	TSharedPtr<SEditableTextBox> OutputPackagePathInput;

	/** The package path browse button (to open a directory picker). */
	TSharedPtr<SButton> PackagePathBrowseButton;

	/** The package button. */
	TSharedPtr<SButton> PackageButton;
};
