/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io UE Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue/blob/main/LICENSE>)
 *
 */

#include "Widgets/SModioEditorCreateUGCWidget.h"

#include "AssetToolsModule.h"
#include "EditorDirectories.h"
#include "Features/IPluginsEditorFeature.h"
#include "Framework/Notifications/NotificationManager.h"
#include "GameProjectGenerationModule.h"
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"
#include "ModioUGCPluginWizardDefinition.h"
#include "PluginUtils.h"
#include "ProjectDescriptor.h"
#include "SlateOptMacros.h"
#include "SourceCodeNavigation.h"
#include "Styling/ToolBarStyle.h"
#include "Widgets/Input/SDirectoryPicker.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "ModioEditorCreateUGCWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SModioEditorCreateUGCWidget::Construct(const FArguments& InArgs,
											TSharedPtr<IPluginWizardDefinition> InPluginWizardDefinition)
{
	ParentWindow = InArgs._ParentWindow;
	PluginWizardDefinition = InPluginWizardDefinition;

	if (!PluginWizardDefinition.IsValid())
	{
		PluginWizardDefinition = MakeShared<FModioUGCPluginWizardDefinition>(IsContentOnlyProject());
	}
	check(PluginWizardDefinition.IsValid());

	// Set the plugin base to Mods directory
	PluginFolderPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Mods"));

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
				.Text(FText::Format(LOCTEXT("ModioCreateUGCHeader", "Create {0}"), FText::FromString(InArgs._ParentWindow->ModioGameInfo->UgcName)))
				.Font(InArgs._ParentWindow->GetTextStyle("EmbossedText", "Normal", 16))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 20.f)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("CreateBlankUGC",
							"Create a blank Unreal Plugin file to be used for your {0}."), FText::FromString(InArgs._ParentWindow->ModioGameInfo->UgcName)))
					
				.Font(InArgs._ParentWindow->GetTextStyle("EmbossedText", "Normal", 8))
				.Justification(ETextJustify::Center)
			] 

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SVerticalBox) 
				+ SVerticalBox::Slot()
				.Padding(FMargin(ParentWindow->PanelPadding))
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage).Image(ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
					.Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.FillWidth(1)
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("ModName", "Mod Name"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SNew(SEditableTextBox)
							.MinDesiredWidth(256.f)
							.OnTextChanged(this, &SModioEditorCreateUGCWidget::OnModNameTextChanged)

						]
					]
				] 

				+ SVerticalBox::Slot()
				.Padding(FMargin(ParentWindow->PanelPadding))
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage)
						.Image(ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
				.	Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Author", "Author"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SNew(SEditableTextBox)
							.MinDesiredWidth(256.f)
							.OnTextChanged_Lambda([this](const FText& NewText) { CreatedBy = NewText.ToString(); })
						]
					]
				] 

				+ SVerticalBox::Slot()
				.Padding(FMargin(ParentWindow->PanelPadding))
				.AutoHeight()
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SImage)
						.Image(ParentWindow->PanelBackgroundBrush)
					]
					+ SOverlay::Slot()
				.	Padding(8.f)
					[
						SNew(SHorizontalBox) 
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.FillWidth(1)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Description", "Description"))
							.Justification(ETextJustify::Left)
						]
						+ SHorizontalBox::Slot()
						.Padding(FMargin(8, 4, 4, 4))
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Center)
						.FillWidth(2)
						[
							SNew(SBox)
							.MinDesiredHeight(50)
							.MaxDesiredHeight(50)
							[
								SNew(SMultiLineEditableTextBox)
								.OnTextChanged_Lambda([this](const FText& NewText) { Description = NewText.ToString(); })
							]
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(FMargin(0, 12))
			.VAlign(VAlign_Bottom)
			[
				SNew(SSeparator)
				.Thickness(1.0f)
				.SeparatorImage(ParentWindow->PanelBackgroundBrush)
			]

			+ SVerticalBox::Slot()
			.Padding(ParentWindow->PanelPadding)
			.AutoHeight()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(ParentWindow->PanelBackgroundBrush)
				]
				+ SOverlay::Slot()
				.Padding(8.f)
				.VAlign(VAlign_Fill)
				[
					SNew(SHorizontalBox) 
					+ SHorizontalBox::Slot()
					.Padding(FMargin(8, 4, 4, 4))
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.FillWidth(1)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AuthorURL", "Author URL"))
						.Justification(ETextJustify::Left)
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(8, 4, 4, 4))
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Center)
					.FillWidth(2)
					[
						SNew(SEditableTextBox)
						.MinDesiredWidth(256.f)
						.OnTextChanged_Lambda([this](const FText& NewText) { CreatedByURL = NewText.ToString() ;})
					]
				]
			] 

			+ SVerticalBox::Slot()
			.Padding(FMargin(ParentWindow->PanelPadding))
			.AutoHeight()
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(ParentWindow->PanelBackgroundBrush)
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
						.Text(LOCTEXT("BetaVersion", "Beta Version"))
						.Justification(ETextJustify::Left)
					]
					+ SHorizontalBox::Slot()
					.Padding(FMargin(8, 4, 4, 4))
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Center)
					.FillWidth(2)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState) { bIsBetaVersion = NewState == ECheckBoxState::Checked; })
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
			.Padding(ParentWindow->PanelPadding)
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
					SNew(SButton)
					.ContentPadding(InArgs._ParentWindow->BottomButtonPadding)
					.Text(LOCTEXT("CreatePackage", "Create Mod Package"))
					.IsEnabled_Lambda([this]() { return !PluginFolderPath.IsEmpty() && !ModNameText.IsEmpty(); })
					.OnClicked(this, &SModioEditorCreateUGCWidget::OnCreatePluginClicked)
				]
			]
		]
	];
	// clang-format on
}

FString SModioEditorCreateUGCWidget::GetModfilePath() const
{
	return PluginFolderPath;
}

void SModioEditorCreateUGCWidget::OnBackButtonPressed() {}

void SModioEditorCreateUGCWidget::OnModNameTextChanged(const FText& InText)
{
	ModNameText = InText;
}

// From SNewPluginWizard.cpp
bool SModioEditorCreateUGCWidget::IsContentOnlyProject()
{
	const FProjectDescriptor* CurrentProject = IProjectManager::Get().GetCurrentProject();
	return CurrentProject == nullptr || CurrentProject->Modules.Num() == 0 ||
		   !FGameProjectGenerationModule::Get().ProjectHasCodeFiles();
}

// Adapted from SNewPluginWizard.cpp
FReply SModioEditorCreateUGCWidget::OnCreatePluginClicked()
{
	if (!ensure(!PluginFolderPath.IsEmpty() && !ModNameText.IsEmpty()))
	{
		// Don't even try to assemble the path or else it may be relative to the binaries folder!
		return FReply::Unhandled();
	}

	TSharedPtr<FPluginTemplateDescription> Template = PluginWizardDefinition->GetSelectedTemplate();
	if (!Template.IsValid())
	{
		// TODO: HARDCODED PULLING THE DEFAULT TEMPLATE FOR NOW
		Template = PluginWizardDefinition->GetTemplatesSource()[0];

		// TODO: PluginWizard needs to have a default template
		PluginWizardDefinition->OnTemplateSelectionChanged(Template, ESelectInfo::Direct);
	}

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	if (AssetToolsModule.Get().IsDiscoveringAssetsInProgress())
	{
		bIsWaitingForAssetDiscoveryToFinish = true;
		AssetToolsModule.Get().OpenDiscoveringAssetsDialog(
			IAssetTools::FOnAssetsDiscovered::CreateSP(this, &SModioEditorCreateUGCWidget::DiscoveringAssetsCalback));
		return FReply::Unhandled();
	}

	const FString PluginName = ModNameText.ToString();
	const bool bHasModules = PluginWizardDefinition->HasModules();

	FPluginUtils::FNewPluginParamsWithDescriptor CreationParams;
	CreationParams.TemplateFolders = PluginWizardDefinition->GetFoldersForSelection();
	CreationParams.Descriptor.bCanContainContent = Template->bCanContainContent;

	if (bHasModules)
	{
		CreationParams.Descriptor.Modules.Add(FModuleDescriptor(*PluginName,
																PluginWizardDefinition->GetPluginModuleDescriptor(),
																PluginWizardDefinition->GetPluginLoadingPhase()));
	}

	CreationParams.Descriptor.FriendlyName = PluginName;
	CreationParams.Descriptor.Version = 1;
	CreationParams.Descriptor.VersionName = TEXT("1.0");
	CreationParams.Descriptor.Category = TEXT("Other");

	PluginWizardDefinition->GetPluginIconPath(/*out*/ CreationParams.PluginIconPath);

	CreationParams.Descriptor.CreatedBy = CreatedBy;
	CreationParams.Descriptor.CreatedByURL = CreatedByURL;
	CreationParams.Descriptor.Description = Description;
	CreationParams.Descriptor.bIsBetaVersion = bIsBetaVersion;

	FText FailReason;
	FPluginUtils::FLoadPluginParams LoadParams;
	LoadParams.bEnablePluginInProject = true;
	LoadParams.bUpdateProjectPluginSearchPath = true;
	LoadParams.bSelectInContentBrowser = bSelectInContentBrowser;
	LoadParams.OutFailReason = &FailReason;

	Template->CustomizeDescriptorBeforeCreation(CreationParams.Descriptor);

	TSharedPtr<IPlugin> NewPlugin =
		FPluginUtils::CreateAndLoadNewPlugin(PluginName, PluginFolderPath, CreationParams, LoadParams);
	const bool bSucceeded = NewPlugin.IsValid();

	PluginWizardDefinition->PluginCreated(PluginName, bSucceeded);

	if (bSucceeded)
	{
		// Let the template create additional assets / modify state after creation
		Template->OnPluginCreated(NewPlugin);

		FNotificationInfo Info(FText::Format(LOCTEXT("PluginCreatedSuccessfully", "'{0}' was created successfully."),
											 FText::FromString(PluginName)));
		Info.bUseThrobber = false;
		Info.ExpireDuration = 8.0f;
		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);

		if (bHasModules)
		{
			FSourceCodeNavigation::OpenModuleSolution();
		}

		ParentWindow->DrawToolLanding();

		return FReply::Handled();
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, FailReason, LOCTEXT("UnableToCreatePlugin", "Unable to create plugin"));
		return FReply::Unhandled();
	}
}

void SModioEditorCreateUGCWidget::DiscoveringAssetsCalback()
{
	bIsWaitingForAssetDiscoveryToFinish = false;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE