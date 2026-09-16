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
#include "Widgets/SCompoundWidget.h"
#include "SUGCTemplateSubsitutionWidget.h"
#include "../../Engine/Public/Slate/DeferredCleanupSlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

/**
 * 
 */
class MODIOUGCEDITOR_API SUGCTemplateDetailsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SUGCTemplateDetailsWidget)
	{}
	SLATE_ARGUMENT(TSharedPtr<FUGCTemplateInfo>, TemplateInfo)
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void Refresh(FUGCTemplateInfo TemplateInfo);

	void GetSubs(TMap<FString, FString>& OutSubs);

private:

	TSharedRef<SWidget> MakeParametersWidget(FUGCTemplateInfo TemplateInfo);

private:

	TSharedPtr<SImage> IconImage;
	TSharedPtr<STextBlock> NameText;
	TSharedPtr<STextBlock> VersionText;
	TSharedPtr<STextBlock> DescriptionText;
	TSharedPtr<SBox> ParamsBox;
	TSharedPtr<SUGCTemplateSubsitutionWidget> SubstitutionWidget;

	TSharedPtr<FDeferredCleanupSlateBrush> ThumbnailBrush;

};
