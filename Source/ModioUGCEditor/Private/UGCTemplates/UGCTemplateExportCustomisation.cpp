// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/UGCTemplateExportCustomisation.h"
#include "Editor/PropertyEditor/Public/DetailLayoutBuilder.h"
#include "Editor/PropertyEditor/Public/DetailCategoryBuilder.h"
#include "Editor/PropertyEditor/Public/DetailWidgetRow.h"
#include "Developer/SharedSettingsWidgets/Public/SExternalImageReference.h"
#include "UGCTemplates/UGCTemplateSubsystem.h"
#include "Interfaces/IPluginManager.h"
#include "UGCTemplates/UGCTemplateExportDetails.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#ifdef WITH_EDITOR
#include "Editor.h"
#endif

#define LOCTEXT_NAMESPACE "FUGCTemplateExportCustomisation"

TSharedRef<IDetailCustomization> FUGCTemplateExportCustomisation::MakeInstance()
{
	return MakeShareable(new FUGCTemplateExportCustomisation);
}

void FUGCTemplateExportCustomisation::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UUGCTemplateExportDetails>> ObjectsBeingCustomized = DetailBuilder.GetObjectsOfTypeBeingCustomized<UUGCTemplateExportDetails>();
	DetailsBeingCustomised = ObjectsBeingCustomized[0].Get();

	UpdatePluginSource();
	UpdateCategorySource();

	IDetailCategoryBuilder& ModCategory = DetailBuilder.EditCategory(TEXT("Mod"));

	ModCategory.AddCustomRow(LOCTEXT("UGCTemplateExportCusomtisationModSelectionRowLabel", "Mod to Export"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("UGCTemplateExportCusomtisationModSelectionLabel", "Mod To Export"))
		.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
	]
	.ValueContent()
	[
		SAssignNew(ModSelectionBox, SComboBox<TSharedPtr<FUGCPluginInfo>>)
		.OptionsSource(&UGCPluginSource)
		.OnSelectionChanged(this, &FUGCTemplateExportCustomisation::OnModSelectionChanged)
		.OnGenerateWidget(this, &FUGCTemplateExportCustomisation::GeneratePluginComboBoxWidget)
		[
			SNew(STextBlock).Text_Lambda([this]() 
				{
					auto SelectedItem = ModSelectionBox->GetSelectedItem();
					if (SelectedItem == nullptr)
					{
						return LOCTEXT("UGCTemplateExportCusomtisationModSelectionDefaultLabel", "Please select plugin to export.");
					}
					return FText::FromString(SelectedItem->Name);
				}
			)
		]
	];

	FString AutomaticPath = "";
	FString TargetPath = "";

	IDetailCategoryBuilder& ModTemplateDetailsCatgory = DetailBuilder.EditCategory(TEXT("Mod Template Details"));

	ModTemplateDetailsCatgory.AddCustomRow(LOCTEXT("UGCTemplateExportCusomtisationThumbnailRowLabel", "Thumbnail"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("UGCTemplateExportCusomtisationThumbnailLabel", "Thumbnail"))
		.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
	]
	.ValueContent()
	[
		SAssignNew(IconSelectorBox, SBox)
		.WidthOverride(256.0f)
		.HeightOverride(256.0f)
		[
			GenerateIconSelectorWidget(AutomaticPath, TargetPath)
		]
	];

	ModTemplateDetailsCatgory.AddCustomRow(LOCTEXT("UGCTemplateExportCusomtisationCategoryRowLabel", "Category"))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("UGCTemplateExportCusomtisationCategoryLabel", "Category"))
		.Font(FAppStyle::GetFontStyle("PropertyWindow.NormalFont"))
	]
	.ValueContent()
	[
		GenerateCategoryWidgetContent()
	];

	if (DetailsBeingCustomised->Context != nullptr)
	{
		ModSelectionBox->SetSelectedItem(GetPluginInfoFrom(DetailsBeingCustomised->Context));
	}
}

void FUGCTemplateExportCustomisation::UpdatePluginSource() 
{
	TArray<FUGCPluginInfo> UGCPlugins;

	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	TemplateSubsystem->DiscoverUGC(UGCPlugins);

	UGCPluginSource.Empty();
	for (auto Plugin : UGCPlugins)
	{
		UGCPluginSource.Add(MakeShareable(new FUGCPluginInfo(Plugin)));
	}
}

void FUGCTemplateExportCustomisation::UpdateCategorySource()
{
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	if (TemplateSubsystem == nullptr)
	{
		return;
	}

	TArray<FUGCTemplateCategoryView> CategoryInfo;
	TemplateSubsystem->GetCategories(CategoryInfo);

	for (auto Category : CategoryInfo)
	{
		CategorySource.Add(MakeShareable(new FUGCTemplateCategoryView(Category)));
	}

}

TSharedRef<SWidget> FUGCTemplateExportCustomisation::GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item->Name); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedRef<SWidget> FUGCTemplateExportCustomisation::GenerateCatgoryComboBoxWidget(TSharedPtr<FUGCTemplateCategoryView> Item)
{
	return SNew(STextBlock)
		.Text(FText::FromString(Item->Category))
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedRef<SWidget> FUGCTemplateExportCustomisation::GenerateIconSelectorWidget(FString BasePath, FString TargetPath)
{
	return SNew(SExternalImageReference, BasePath, TargetPath);
}

TSharedRef<SWidget> FUGCTemplateExportCustomisation::GenerateCategoryWidgetContent() 
{
	if (CategorySource.Num() == 0)
	{
		return SNew(STextBlock)
			.Text(LOCTEXT("UGCTemplateExportCusomtisationNoCategoriesLabel",
				  "No categories defined in Project Settings > mod.io UGC Templates"));
	}

	return	SAssignNew(CategorySelectionBox, SComboBox<TSharedPtr<FUGCTemplateCategoryView>>)
			.OptionsSource(&CategorySource)
			.OnSelectionChanged(this, &FUGCTemplateExportCustomisation::OnCategorySelectionChanged)
			.OnGenerateWidget(this, &FUGCTemplateExportCustomisation::GenerateCatgoryComboBoxWidget)
			[
				SNew(STextBlock).Text_Lambda([this]()
					{
						auto SelectedItem = CategorySelectionBox->GetSelectedItem();
						if (SelectedItem == nullptr)
						{
							return LOCTEXT("UGCTemplateExportCusomtisationCategorySelectionDefaultLabel",
										   "Please select template category.");
						}
						return SelectedItem->DisplayName;
					}
				)
			];
}

void FUGCTemplateExportCustomisation::OnModSelectionChanged(TSharedPtr<FUGCPluginInfo> Item, ESelectInfo::Type SelectType)
{
	if (DetailsBeingCustomised == nullptr)
	{
		//error
		return;
	}

	DetailsBeingCustomised->ModToExport = *Item.Get();
	
	FString AutomaticPath = "";
	FString TargetPath = "";

	if (Item != nullptr)
	{
		AutomaticPath = FPaths::Combine(Item->Plugin->GetBaseDir(), "Resources/Icon128.png");
		TargetPath = FPaths::Combine(Item->Plugin->GetBaseDir(), "Resources/Thumbnail.png");
	}

	IconSelectorBox->SetContent(GenerateIconSelectorWidget(AutomaticPath, TargetPath));
}

void FUGCTemplateExportCustomisation::OnCategorySelectionChanged(TSharedPtr<FUGCTemplateCategoryView> Item, ESelectInfo::Type SelectType)
{
	if (DetailsBeingCustomised == nullptr)
	{
		// error
		return;
	}

	DetailsBeingCustomised->Category = Item->Category;
}

TSharedPtr<FUGCPluginInfo> FUGCTemplateExportCustomisation::GetPluginInfoFrom(TSharedPtr<IPlugin> Context)
{
	for (auto PluginInfo : UGCPluginSource)
	{
		if (PluginInfo->Plugin == Context)
		{
			return PluginInfo;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE