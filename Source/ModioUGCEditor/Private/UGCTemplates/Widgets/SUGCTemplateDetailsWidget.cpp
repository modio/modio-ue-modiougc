// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SUGCTemplateDetailsWidget.h"
#include "SlateOptMacros.h"
#include "UGCTemplates/Widgets/SUGCTemplateSubsitutionWidget.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "../../SlateCore/Public/Types/SlateStructs.h"
#include "../../Engine/Public/Slate/DeferredCleanupSlateBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "SUGCTemplateDetailsWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUGCTemplateDetailsWidget::Construct(const FArguments& InArgs)
{
	auto BoldFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);

	//clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.Padding(8.0f)
		[
			SNew(SBox)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.Padding(8.0f)
				.SizeParam(FAuto())
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					[
						SAssignNew(IconImage, SImage)
						.DesiredSizeOverride(FVector2D(128.0f, 128.0f))
					]
				]
				+SVerticalBox::Slot()
				.SizeParam(FAuto())
				[
					SAssignNew(NameText, STextBlock)
					.Text(LOCTEXT("UGCTemplateTemplateNameLabel", "Template Name"))
					.Font(BoldFont)
				]
				+SVerticalBox::Slot()
				.SizeParam(FAuto())
				[
					SAssignNew(VersionText, STextBlock)
					.Text(LOCTEXT("UGCTemplateTemplateVersionLabel", "Template Version"))
				]
				+SVerticalBox::Slot()
				.Padding(0.0f, 10.0f)
				.SizeParam(FAuto())
				[
					SAssignNew(DescriptionText, STextBlock)
					.Text(LOCTEXT("UGCTemplateTemplateDescriptionLabel", "Template Description"))
				]
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(ParamsBox, SBox)
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Fill)
			//[
			//	//SNew(STextBlock).Text(FText::FromString("Template params"))
			//	MakeParametersWidget(InArgs._TemplateInfo)
			//]
	]];
	//clang-format on

	if (InArgs._TemplateInfo != nullptr)
	{
		Refresh(*(InArgs._TemplateInfo));
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SUGCTemplateDetailsWidget::Refresh(FUGCTemplateInfo TemplateInfo)
{
	if (IconImage == nullptr || NameText == nullptr || VersionText == nullptr || DescriptionText == nullptr)
	{
		return;
	}

	ThumbnailBrush = FDeferredCleanupSlateBrush::CreateBrush(TemplateInfo.Thumbnail, FVector2D(128.0f, 128.0f)).ToSharedPtr();
	IconImage->SetImage(ThumbnailBrush->GetSlateBrush());
	NameText->SetText(FText::FromString(TemplateInfo.Name));
	VersionText->SetText(FText::FromString(TemplateInfo.Descriptor->TemplateVersion));
	DescriptionText->SetText(FText::FromString(TemplateInfo.Descriptor->Description));

	ParamsBox->SetContent(MakeParametersWidget(TemplateInfo));
}

void SUGCTemplateDetailsWidget::GetSubs(TMap<FString, FString>& OutSubs)
{
	if (SubstitutionWidget == nullptr)
	{
		return;
	}

	SubstitutionWidget->GetSubs(OutSubs);
}

TSharedRef<SWidget> SUGCTemplateDetailsWidget::MakeParametersWidget(FUGCTemplateInfo TemplateInfo)
{
	auto ParamsWidget = SNew(SUGCTemplateSubsitutionWidget);
	ParamsWidget.Get().BuildFor(TemplateInfo);
	
	SubstitutionWidget = ParamsWidget.ToSharedPtr();

	if (TemplateInfo.Descriptor != nullptr && TemplateInfo.Descriptor->SubstitutionParameters.Num() == 0)
	{
		SubstitutionWidget->SetVisibility(EVisibility::Collapsed);
	}
	
	return ParamsWidget;

}

#undef LOCTEXT_NAMESPACE