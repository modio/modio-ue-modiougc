// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SUGCTemplateSubsitutionWidget.h"
#include "SlateOptMacros.h"

#define LOCTEXT_NAMESPACE "ModioEditorUGCTemplateWidget"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUGCTemplateSubsitutionWidget::Construct(const FArguments& InArgs)
{
	BackgroundBrush = new FSlateBrush();
#if ENGINE_MAJOR_VERSION >= 5
	BackgroundBrush->TintColor = FLinearColor(0.01f, 0.01f, 0.01f, 1.f);
#else
	BackgroundBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.f);
#endif

	PanelBackgroundBrush = new FSlateBrush();
	PanelBackgroundBrush->TintColor = FLinearColor(0.015f, 0.015f, 0.015f, 1.f);

	HeaderBackgroundBrush = new FSlateBrush();
	HeaderBackgroundBrush->TintColor = FLinearColor(0.025f, 0.025f, 0.025f, 1.f);

	// clang-format off

	ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage).Image(BackgroundBrush)
		]
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.Padding(8.f)
			.AutoHeight()
			[
				SAssignNew(SubContainer, SVerticalBox)
				+SVerticalBox::Slot()
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SImage)
						.Image(HeaderBackgroundBrush)
					]
					+SOverlay::Slot()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SubstituionParameterHeader", "Set values for template parameters"))
						.Justification(ETextJustify::Center)
					]
				]
			]
			+SVerticalBox::Slot()
			.Padding(2.5f)
			.AutoHeight()
			[
				SNew(SBox)
				.MinDesiredHeight(50)
				.MaxDesiredHeight(50)
				[
					SAssignNew(ConfirmButton, SButton)
					.Text(LOCTEXT("Confirm", "Confirm"))
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Center)
				]
			]
		]
	];
	
	// clang-format on
}

void SUGCTemplateSubsitutionWidget::BuildFor(const TSharedPtr<FUGCTemplateInfo> Template)
{
	if (!SubContainer.IsValid())
	{
		return;
	}

	TSet<FString> Substitutions;
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	TemplateSubsystem->GetSubstitutionsFor(*(Template.Get()), Substitutions);
	for (auto& Sub : Substitutions)
	{
		// clang-format off
		TSharedPtr<SEditableTextBox> SubInput = nullptr;
		SubContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image(PanelBackgroundBrush)
			]
			+SOverlay::Slot()
			[
				SNew(SStackBox)
				.Orientation(Orient_Horizontal)
				+SStackBox::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Sub))
				]
				+SStackBox::Slot()
				.Padding(8.f)
				[
					SAssignNew(SubInput, SEditableTextBox)
				]
			]
		];
		// clang-format on

		if (SubInput != nullptr)
		{
			SubWidgetMapping.Add(Sub, SubInput);
		}
	}
}

void SUGCTemplateSubsitutionWidget::GetSubs(TMap<FString, FString>& OutSubs)
{
	//TMap<FString, FString> Subs;

	for (auto& SubMapping : SubWidgetMapping)
	{
		OutSubs.Add({SubMapping.Key, SubMapping.Value->GetText().ToString()});
	}

	//return Subs;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE