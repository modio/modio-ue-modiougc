/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#include "ModioUGCEditorStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "ModioUGCEditor.h"
#include "Slate/SlateGameResources.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FModioUGCEditorStyle::StyleInstance = nullptr;

void FModioUGCEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FModioUGCEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FModioUGCEditorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ModioUGCEditorStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH(RelativePath, ...) \
	FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define BORDER_BRUSH(RelativePath, ...) \
	FSlateBorderBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
#define TTF_FONT(RelativePath, ...) FSlateFontInfo(Style->RootToContentDir(RelativePath, TEXT(".ttf")), __VA_ARGS__)
#define OTF_FONT(RelativePath, ...) FSlateFontInfo(Style->RootToContentDir(RelativePath, TEXT(".otf")), __VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FModioUGCEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("ModioUGCEditorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ModioUGC")->GetBaseDir() / TEXT("Resources") /
						  TEXT("ModioUGCEditor"));

	Style->Set("ModioUGCEditor.PackageUGCAction", new IMAGE_BRUSH(TEXT("PackageUGC_64x"), Icon40x40));
	Style->Set("ModioUGCEditor.CreateUGCAction", new IMAGE_BRUSH(TEXT("CreateUGC_64x"), Icon40x40));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FModioUGCEditorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FModioUGCEditorStyle::Get()
{
	return *StyleInstance;
}
