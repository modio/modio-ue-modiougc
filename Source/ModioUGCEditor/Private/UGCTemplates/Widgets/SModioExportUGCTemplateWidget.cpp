// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SModioExportUGCTemplateWidget.h"
#include "SlateOptMacros.h"
#include "Editor/PropertyEditor/Private/SDetailsView.h"
#include "UGCTemplates/UGCTemplateExportDetails.h"
#include "../../Slate/Public/Widgets/Input/SSegmentedControl.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "UGCTemplates/UGCTemplateSubsystem.h"
#include "ModioUGCEditor.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "SModioExportUGCTemplateWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SModioExportUGCTemplateWidget::Construct(const FArguments& InArgs)
{
	ExportDetails = NewObject<UUGCTemplateExportDetails>();
	ExportDetails->Context = InArgs._Context;
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = false;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = false;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
		DetailsViewArgs.bShowScrollBar = true;
	}
	TemplateExportDetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	TemplateExportDetailsView->SetObject(ExportDetails);

	ChildSlot
	[
		SNew(SOverlay) 
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				//Mode buttons
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Padding(8.0f)
				[
					CreateTypeTabs()
				]
			]
			+SVerticalBox::Slot()
			[
				TemplateExportDetailsView.ToSharedRef()
			] 
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				//Create buttons
				SNew(SBox)
				.Padding(8.0f)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					[
						SNew(SButton)
						.Text(LOCTEXT("UGCTemplateExportCreateLabel", "Create"))
						.IsEnabled(this, &SModioExportUGCTemplateWidget::IsCreateEnabled)
						.OnClicked(this, &SModioExportUGCTemplateWidget::OnCreateClicked)
					] 
					+SHorizontalBox::Slot()
					[
						SNew(SButton)
						.Text(LOCTEXT("UGCTemplateExportCancelLabel", "Cancel"))
						.OnClicked(this, &SModioExportUGCTemplateWidget::OnCancelClicked)
					]
				]
			]
		]
	];

}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SModioExportUGCTemplateWidget::OnTemplateTypeChanged(EUGCTemplateType Value)
{
	Mode = Value;
}

TSharedRef<SWidget> SModioExportUGCTemplateWidget::CreateTypeTabs()
{
	auto TemplateTypeSelector =
		SNew(SSegmentedControl<EUGCTemplateType>)
			.OnValueChanged(this, &SModioExportUGCTemplateWidget::OnTemplateTypeChanged)
			.TextStyle(FAppStyle::Get(), "DialogButtonText")
			.MaxSegmentsPerLine(2);

	TemplateTypeSelector->AddSlot(EUGCTemplateType::TT_Item, false).Text(LOCTEXT("UGCTemplateExportTypeTabLabelItem", "Item Template"));
	TemplateTypeSelector->AddSlot(EUGCTemplateType::TT_Mod, false).Text(LOCTEXT("UGCTemplateExportTypeTabLabelMod", "Mod Template"));
	TemplateTypeSelector->RebuildChildren();
	TemplateTypeSelector->SetValue(EUGCTemplateType::TT_Item);

	return TemplateTypeSelector;
}

FReply SModioExportUGCTemplateWidget::OnCreateClicked()
{
	FUGCPluginInfo ModToExport = ExportDetails->ModToExport;

	UUGCTemplateSubsystem* TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	bool bSuccessfulExport = TemplateSubsystem->ExportUGCTemplate(ModToExport, ExportDetails->Name.ToString(), MakeTemplateDescriptor(ExportDetails));

	if (bSuccessfulExport)
	{
		FString TemplateDirectory = TemplateSubsystem->GetTemplateDirectory();
		FNotificationInfo Info(LOCTEXT("UGCTemplateExportSuccessNotificationText", "Succcess"));
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;
		Info.ExpireDuration = 10.0f;
		Info.HyperlinkText = FText::FromString("Open Folder");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([TemplateDirectory]() {
			const FString HyperlinkTarget = FString("file://") / TemplateDirectory;
			FPlatformProcess::LaunchURL(*HyperlinkTarget, nullptr, nullptr);
		});
		Info.SubText = LOCTEXT("UGCTemplateExportSuccessNotificationText", "New UGC template created");

		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);

		FModioUGCEditorModule& ModioUGCEditorModule = FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
		ModioUGCEditorModule.DismissUGCTemplateWindow();
	}
	else
	{
		FNotificationInfo Info(LOCTEXT("UGCTemplateExportFailNotificationText", "Failed"));
		//Info.Image = FAppStyle::GetBrush(TEXT("Icons.Error"));
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;
		Info.ExpireDuration = 10.0f;
		Info.HyperlinkText = FText::FromString("Open Log");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([]()
			{
				//FMessageLog("LoadErrors").Open(EMessageSeverity::Info, true);
				FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog"));
			}
		);
		Info.SubText = LOCTEXT("UGCTemplateExportFailNotificationText", "Could not create new template");

		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
	}

	return FReply::Handled();
}

FReply SModioExportUGCTemplateWidget::OnCancelClicked()
{
	FModioUGCEditorModule& ModioUGCEditorModule = FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
	ModioUGCEditorModule.DismissUGCTemplateWindow();

	return FReply::Handled();
}

UUGCTemplateDescriptor* SModioExportUGCTemplateWidget::MakeTemplateDescriptor(class UUGCTemplateExportDetails* InExportDetails)
{
	UUGCTemplateDescriptor* TemplateDescriptor = NewObject<UUGCTemplateDescriptor>();

	TemplateDescriptor->Type = Mode;
	TemplateDescriptor->SubstitutionParameters = InExportDetails->SubstitutionParameters;
	TemplateDescriptor->Description = InExportDetails->Description.ToString();
	TemplateDescriptor->TemplateVersion = InExportDetails->Version;
	TemplateDescriptor->Category = InExportDetails->Category;

	return TemplateDescriptor;
}

bool SModioExportUGCTemplateWidget::IsCreateEnabled() const 
{
	return	!ExportDetails->Name.IsEmptyOrWhitespace() && 
			!ExportDetails->Description.IsEmptyOrWhitespace() &&
			 ExportDetails->ModToExport.Plugin != nullptr;
}

#undef LOCTEXT_NAMESPACE