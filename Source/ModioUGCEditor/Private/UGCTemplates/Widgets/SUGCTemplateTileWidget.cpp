// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SUGCTemplateTileWidget.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SlateOptMacros.h"
#include "Engine/Texture2D.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUGCTemplateTileWidget::Construct(const FArguments& InArgs)
{
	BackgroundBrush = new FSlateBrush();
#if ENGINE_MAJOR_VERSION >= 5
	BackgroundBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.f);
#else
	BackgroundBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.f);
#endif
	BackgroundBrush->DrawAs = ESlateBrushDrawType::RoundedBox;
	BackgroundBrush->OutlineSettings.CornerRadii = FVector4(4.0f);
	BackgroundBrush->OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

	TemplateInfo = InArgs._TemplateInfo;

	ThumbnailBrush = FDeferredCleanupSlateBrush::CreateBrush(TemplateInfo.Thumbnail, FVector2D(64, 64)).ToSharedPtr();

	// clang-format off
	ChildSlot
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SImage)
			.Image(BackgroundBrush)
		]
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(64)
				.WidthOverride(64)
				[
					SNew(SImage)
					.Image(ThumbnailBrush->GetSlateBrush())
				]
			]
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(30)
				.VAlign(VAlign_Top)
				.Padding(5.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TemplateInfo.Name))
				]
			]
		]
	];
	// clang-format on
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
