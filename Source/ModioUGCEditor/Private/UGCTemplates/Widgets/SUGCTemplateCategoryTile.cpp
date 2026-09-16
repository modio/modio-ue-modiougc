// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SUGCTemplateCategoryTile.h"
#include "UGCTemplates/UGCTemplateSettings.h"
#include "SlateOptMacros.h"
#include "../../Engine/Public/Slate/DeferredCleanupSlateBrush.h"
#include "../../SlateCore/Public/Brushes/SlateDynamicImageBrush.h"
#include "Engine/Texture2D.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SUGCTemplateCategoryTile"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SUGCTemplateCategoryTile::Construct(const FArguments& InArgs)
{
	CategoryInfo = InArgs._CategoryInfo;

	FText CategoryLabel = (CategoryInfo != nullptr ? CategoryInfo->DisplayName : LOCTEXT("UGCTemplateCategoryTileMissingCategoryName", "<Missing Category Name>"));

	GenerateImageBrush();

	//clang-format off
	ChildSlot
	[
		SNew(SOverlay) + SOverlay::Slot().Padding(2.0f)
		[
			SNew(SBox).HeightOverride(64.0f)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SNew(SImage)
					.Image(BackgroundBrush->GetSlateBrush())
				] 
				+SOverlay::Slot()
				.VAlign(VAlign_Bottom)
				.Padding(5.0f)
				[
					SNew(STextBlock)
					.Text(CategoryLabel)
					.ShadowOffset(FVector2D(1, 1))
					.ShadowColorAndOpacity(FLinearColor(0, 0, 0, .75))
					.Visibility(EVisibility::HitTestInvisible)
				]
			]
		]
	];
	//clang-format on
}

FString SUGCTemplateCategoryTile::GetCategory() const
{
	if (CategoryInfo == nullptr)
	{
		return "";
	}

	return CategoryInfo->Category;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SUGCTemplateCategoryTile::GenerateImageBrush()
{
	UTexture2D* Image = TSoftObjectPtr<UTexture2D>(CategoryInfo->Image).LoadSynchronous();
	BackgroundBrush = FDeferredCleanupSlateBrush::CreateBrush(Image, FVector2D(64, 64)).ToSharedPtr();

	FSlateBrush* Brush = const_cast<FSlateBrush*>(BackgroundBrush->GetSlateBrush());
	Brush->DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush->OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	Brush->OutlineSettings.CornerRadii = FVector4(4.0f, 4.0f, 4.0f, 4.0f);
}

#undef LOCTEXT_NAMESPACE