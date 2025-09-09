/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue/blob/main/LICENSE>)
 *
 */

#include "Widgets/SModioEditorPackageUGCWidget.h"

#include "ModioUGCEditor.h"
#include "ModioUGCPackager.h"
#include "SListViewSelectorDropdownMenu.h"
#include "SlateOptMacros.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/SModioEditorWindowCompoundWidget.h"

#define LOCTEXT_NAMESPACE "ModioEditorCreateUGCWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SModioEditorPackageUGCWidget::Construct(const FArguments& InArgs)
{
	UGCPackager = MakeShared<FModioUGCPackager>();

	UGCPackager->PlatformsSource.Empty();
	for (const auto& Pair : FDataDrivenPlatformInfoRegistry::GetAllPlatformInfos())
	{
		if (Pair.Value.bIsFakePlatform || Pair.Value.bEnabledForUse == false)
		{
			continue;
		}

		if (!InArgs._ParentWindow->ModioGameInfo->PlatformSupport.ContainsByPredicate(
				[Pair](FModioGamePlatform platform) {
					FString PlatformEnum = UEnum::GetValueAsString(platform.Platform);
					PlatformEnum.RemoveFromStart(TEXT("EModioModfilePlatform::"));
					return PlatformEnum == Pair.Key.ToString();
				}))
		{
			continue;
		}

		UGCPackager->PlatformsSource.Add(MakeShared<FString>(Pair.Key.ToString()));
	}

	// Block and unblock window
	UGCPackager->OnPackagingStatusChanged.BindLambda([this, CompoundWindow = InArgs._ParentWindow](bool bIsPackaging) {
		CompoundWindow->SetEnabled(!bIsPackaging);
	});

	TArray<TSharedRef<IPlugin>> AvailablePlugins;
	UGCPackager->FindAvailablePlugins(AvailablePlugins);

	UGCPackager->PluginsSource.Empty();

	for (const TSharedRef<IPlugin>& Plugin : AvailablePlugins)
	{
		UGCPackager->PluginsSource.Add(Plugin);
	}

	UGCPackager->LoadSavedSettings();

	FText SelectPluginText =
		FText::Format(LOCTEXT("SelectPluginText", "Package your Unreal Plugin for exporting as {0}"),
					  FText::FromString(InArgs._ParentWindow->ModioGameInfo->UgcName));
	FText PlatformToolTip = LOCTEXT("PlatformToolTip", "The platform to package the plugin for");
	FText PackagePluginText = LOCTEXT("PackagePluginText", "Package");
	FText OutputPackagePathText = LOCTEXT("OutputPackagePathText", "Output path");
	FText SelectOutputPackagePathText = LOCTEXT("SelectOutputPackagePathText", "Select output path");

	// clang-format off
	ChildSlot
		[
			SNew(SBox)
			.MinDesiredWidth(900)
			.MinDesiredHeight(500)
			[
				SNew(SVerticalBox) 
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.f, 0.f, 0.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("ModioPackageUGCTitle", "Package UGC"), FText::FromString(InArgs._ParentWindow->ModioGameInfo->UgcName)))
					.Font(InArgs._ParentWindow->HeaderLargeTextStyle)
				] 
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 20.f)
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(SelectPluginText)
					.Justification(ETextJustify::Center)
				] 
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.Padding(InArgs._ParentWindow->PanelPadding)
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(InArgs._ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
					.Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModioPackageUGCSelect", "Select a Plugin to Package"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SAssignNew(PluginComboBox, SComboBox<TSharedPtr<IPlugin>>)
							.OptionsSource(&UGCPackager->PluginsSource)
							.InitiallySelectedItem([this]() -> TSharedPtr<IPlugin> {
								if (TSharedPtr<IPlugin> SelectedPlugin = UGCPackager->GetSelectedPlugin())
								{
									return SelectedPlugin;
								}
								return nullptr;
							}())
							.ToolTipText(PlatformToolTip)
							.OnSelectionChanged(this, &SModioEditorPackageUGCWidget::OnPluginSelected)
							.OnGenerateWidget(this, &SModioEditorPackageUGCWidget::GeneratePluginComboBoxWidget)
							[
								SNew(STextBlock)
								.Text_Lambda([this]() 
								{
									TSharedPtr<IPlugin> SelectedPlugin = UGCPackager->GetSelectedPlugin();
									if (SelectedPlugin.IsValid())
									{
										return FText::FromString(SelectedPlugin->GetName());
									}
									return LOCTEXT("ModioUGCManagerEditor_NoPluginsAvailable",
											"No plugins available");
								})
								.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.Padding(InArgs._ParentWindow->PanelPadding)
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(InArgs._ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
					.Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModioPackageUGCPlatform", "What platform to Package for"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SAssignNew(PlatformComboBox, SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&UGCPackager->PlatformsSource)
							.InitiallySelectedItem([this]() -> TSharedPtr<FString> 
							{
								FText SelectedPlatformName = GetSelectedPlatformName();
								if (!SelectedPlatformName.IsEmpty())
								{
									const TSharedPtr<FString>* FoundPlatformName =
										UGCPackager->PlatformsSource.FindByPredicate(
											[this, SelectedPlatformName](
												const TSharedPtr<FString>& PlatformName) 
											{
												if (SelectedPlatformName.ToString().Equals(
														*PlatformName))
												{
													return true;
												}
												return false;
											});
									if (FoundPlatformName)
									{
										return *FoundPlatformName;
									}
								}
								return nullptr;
							}())
							.ToolTipText(PlatformToolTip)
							.OnSelectionChanged(this, &SModioEditorPackageUGCWidget::OnPlatformSelected)
							.OnGenerateWidget(this, &SModioEditorPackageUGCWidget::GenerateStringComboBoxWidget)
							[
								SNew(STextBlock)
								.Text(this, &SModioEditorPackageUGCWidget::GetSelectedPlatformName)
								.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
						]
					]
				]
				+ SVerticalBox::Slot()
				.VAlign(VAlign_Fill)
				.HAlign(HAlign_Fill)
				.Padding(InArgs._ParentWindow->PanelPadding)
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(InArgs._ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
					.Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModioPackageUGCOutput", "Package Output Path"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SNew(SDirectoryPicker)
							.OnDirectoryChanged(this, &SModioEditorPackageUGCWidget::OnOutputPackagePathTextChanged)
							.Directory(UGCPackager->SelectedSettings.OutputPackagePath)
						]
					]
				]

			+ SVerticalBox::Slot()
			.Padding(FMargin(0, 12))
			.VAlign(VAlign_Bottom)
			[
				SNew(SSeparator)
				.SeparatorImage(InArgs._ParentWindow->BoldSeperatorBrush)
				.Thickness(1.0f)
			]

			+ SVerticalBox::Slot()
			.VAlign(VAlign_Bottom)
			.Padding(InArgs._ParentWindow->PanelPadding)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding(FMargin(8))
				.FillWidth(1)
				[
					SNew(SButton)
					.ContentPadding(InArgs._ParentWindow->BottomButtonPadding)
					.Text(LOCTEXT("Back", "Back"))
					.OnClicked_Lambda([this, CompoundWindow = InArgs._ParentWindow]() 
					{
						CompoundWindow->DrawToolLanding();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(FMargin(8))
				.FillWidth(2)
				[
					SAssignNew(PackageButton, SButton)
					.OnClicked(this, &SModioEditorPackageUGCWidget::HandleOnPackageButtonPressed)
					.IsEnabled_Lambda([this]() -> bool 
					{
						FText UnusedValidationErrorMessage;
						return UGCPackager->ValidateSelectedSettings(UnusedValidationErrorMessage);
					})
					.ToolTipText_Lambda([this]() -> FText 
					{
						FText ValidationErrorMessage;
						if (!UGCPackager->ValidateSelectedSettings(ValidationErrorMessage))
						{
							return ValidationErrorMessage;
						}
						return FText::GetEmpty();
					})
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Fill)
					.ContentPadding(InArgs._ParentWindow->BottomButtonPadding)
					.ForegroundColor(FSlateColor::UseForeground())
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						.VAlign(VAlign_Center)
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(PackagePluginText)
							.Justification(ETextJustify::Center)
						]
					]
				]
			]
		]
	];

	// clang-format on
}

FText SModioEditorPackageUGCWidget::GetOutputPackagePath() const
{
	return UGCPackager->GetOutputPackagePath();
}

TSharedRef<SWidget> SModioEditorPackageUGCWidget::GenerateStringComboBoxWidget(TSharedPtr<FString> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedRef<SWidget> SModioEditorPackageUGCWidget::GeneratePluginComboBoxWidget(TSharedPtr<IPlugin> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item->GetName()); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

FText SModioEditorPackageUGCWidget::GetSelectedPlatformName() const
{
	return UGCPackager->GetSelectedPlatformName();
}

void SModioEditorPackageUGCWidget::OnPluginSelected(TSharedPtr<IPlugin> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UGCPackager->SelectedSettings.Plugin = SelectedItem;
	UGCPackager->SaveSettings();
}

void SModioEditorPackageUGCWidget::OnPlatformSelected(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
	UGCPackager->SelectedSettings.PlatformName = *SelectedItem;
	UGCPackager->SaveSettings();
}

void SModioEditorPackageUGCWidget::OnOutputPackagePathTextChanged(const FString& InText)
{
	UGCPackager->SelectedSettings.OutputPackagePath = InText;
	UGCPackager->SaveSettings();
}

FReply SModioEditorPackageUGCWidget::HandleOnPackagePathBrowseButtonClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(PackageButton.ToSharedRef());

		FString FolderName;
		const bool bFolderSelected = DesktopPlatform->OpenDirectoryDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(PackageButton),
			LOCTEXT("PackageUGCOutputFolderDialogTitle", "Select UGC output directory").ToString(),
			UGCPackager->SelectedSettings.OutputPackagePath, FolderName);

		UGCPackager->SelectedSettings.OutputPackagePath = FolderName;
		UGCPackager->SaveSettings();
	}
	else
	{
		UE_LOG(ModioUGCEditor, Error, TEXT("Failed to open directory dialog"));
	}

	return FReply::Handled();
}

FReply SModioEditorPackageUGCWidget::HandleOnPackageButtonPressed()
{
	UGCPackager->PackagePlugin();
	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE