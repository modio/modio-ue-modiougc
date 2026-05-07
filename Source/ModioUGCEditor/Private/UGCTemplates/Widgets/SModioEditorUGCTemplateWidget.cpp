// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SModioEditorUGCTemplateWidget.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "UGCTemplates/Widgets/SUGCTemplateSubsitutionWidget.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "../Private/SDetailsView.h"

#define LOCTEXT_NAMESPACE "ModioEditorUGCTemplateWidget"

FModioEditorUGCTemplateWidgetCommands::FModioEditorUGCTemplateWidgetCommands()
	: TCommands<FModioEditorUGCTemplateWidgetCommands>(
		  "ModioEditorUGCTemplateWidgetCommands", // Context name for fast lookup
		  NSLOCTEXT("Contexts", "FModioEditorUGCTemplateWidget",
					"Modio UGC Templates"), // Localized context name for displaying
		  NAME_None, FAppStyle::GetAppStyleSetName())
{
}

void FModioEditorUGCTemplateWidgetCommands::RegisterCommands() 
{
	UI_COMMAND(ClearLog, "Clear", "Clear Log", EUserInterfaceActionType::Button,
			   FInputChord(EKeys::L, EModifierKey::Control));
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

SModioEditorUGCTemplateWidget::SModioEditorUGCTemplateWidget() 
	: LogCommandList(MakeShared<FUICommandList>())
{
}

SModioEditorUGCTemplateWidget::~SModioEditorUGCTemplateWidget() {}

void SModioEditorUGCTemplateWidget::Construct(const FArguments& InArgs)
{
	LoadResources();

	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	OnTemplateLogHandle = TemplateSubsystem->OnTemplateLogMessage.AddRaw(this, &SModioEditorUGCTemplateWidget::AddMessageToLog);
	TemplateSubsystem->DiscoverUGC(UGCPlugins);
	TemplateSubsystem->DiscoverTemplates(Templates);
	UGCPluginsOptions = ConvertToOptionsSource(UGCPlugins);
	TemplatesOptions = ConvertToOptionsSource(Templates);
	ItemTemplatesOptions = TemplatesOptions.FilterByPredicate([](const TSharedPtr<FUGCTemplateInfo>& Key) -> bool { return Key->Descriptor->Type == EUGCTemplateType::TT_Item; });
	ModTemplatesOptions  = TemplatesOptions.FilterByPredicate([](const TSharedPtr<FUGCTemplateInfo>& Key) -> bool { return Key->Descriptor->Type == EUGCTemplateType::TT_Mod; });

	ExportDescriptor = NewObject<UUGCTemplateDescriptor>();
	//ExportDescriptor = MakeShareable(NewObject<UUGCTemplateDescriptor>());

	const FModioEditorUGCTemplateWidgetCommands& Commands = FModioEditorUGCTemplateWidgetCommands::Get();

	LogCommandList->MapAction(Commands.ClearLog, FExecuteAction::CreateSP(this, &SModioEditorUGCTemplateWidget::ClearLog));

//	FSlateBrush* BackgroundBrush = new FSlateBrush();
//#if ENGINE_MAJOR_VERSION >= 5
//	BackgroundBrush->TintColor = FLinearColor(0.01f, 0.01f, 0.01f, 1.f);
//#else
//	BackgroundBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.f);
//#endif

	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = false;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bAllowMultipleTopLevelObjects = false;
		DetailsViewArgs.bShowScrollBar = false; // Don't need to show this, as we are putting it in a scroll box
	}
	//RigOptionsDetailsView =	FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ExportDescriptorDetails = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ExportDescriptorDetails->SetObject(ExportDescriptor);

	const TSharedPtr<SScrollBar> LogHScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Horizontal);
	const TSharedPtr<SScrollBar> LogVScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Vertical);

// clang-format off

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage).Image(HeaderBackgroundBrush)
			]
			+SOverlay::Slot()
			.VAlign(VAlign_Center)
			[
				SAssignNew(HeaderText, STextBlock)
				.Text(LOCTEXT("UGCTemplateHeader", "Use templates to create new or add items to existing UGC"))
				.Justification(ETextJustify::Center)
			]
		]
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage).Image(BackgroundBrush)
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoHeight()
				.Padding(12)
				[
					SNew(STextBlock)
					.Text(FText::FromString("This is a preview UI for testing purposes"))
					.Justification(ETextJustify::Center)
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.MinDesiredHeight(50)
					.MaxDesiredHeight(50)
					[
						SNew(SHorizontalBox) 
						+SHorizontalBox::Slot()
						[
							SNew(SButton)
							.Text(LOCTEXT("ItemTemplateLabel", "Item Templates"))
							.OnClicked(this, &SModioEditorUGCTemplateWidget::ToItemTemplates)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
						]
						+SHorizontalBox::Slot()
						[
							SNew(SButton)
							.Text(LOCTEXT("ModTemplateLabel", "Mod Templates"))
							.OnClicked(this, &SModioEditorUGCTemplateWidget::ToModTemplates)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
						]
						+SHorizontalBox::Slot()
						[
							SNew(SButton)
							.Text(LOCTEXT("ExportTemplateLabel", "Export"))
							.OnClicked(this, &SModioEditorUGCTemplateWidget::ToExportTemplate)
							.VAlign(VAlign_Center)
							.HAlign(HAlign_Center)
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(TemplateTypeSwitcher, SWidgetSwitcher)
					+SWidgetSwitcher::Slot()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						.AutoHeight()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(PanelBackgroundBrush)
							]
							+SOverlay::Slot()
							.Padding(8.f)
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								[
									SNew(SStackBox)
									.Orientation(EOrientation::Orient_Horizontal)
									+SStackBox::Slot()
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("AddTemplateItemModLabel", "Select a mod to add to:"))
									] 
									+SStackBox::Slot()
									[
										SAssignNew(ModTemplateModSelectionBox, SComboBox<TSharedPtr<FUGCPluginInfo>>)
										.OptionsSource(&UGCPluginsOptions)
										.OnGenerateWidget(this, &SModioEditorUGCTemplateWidget::GeneratePluginComboBoxWidget)
										[
											SNew(STextBlock).Text_Lambda([this]() 
												{
													auto SelectedItem = ModTemplateModSelectionBox->GetSelectedItem();
													if (SelectedItem == nullptr)
													{
														return LOCTEXT("AddTemplateItemUnselectedMod", "Please select mod to add item to.");
													}
													return FText::FromString(SelectedItem->Name);
												}
											)
										]
									]
								]
							]
						]
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						.AutoHeight()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(PanelBackgroundBrush)
							]
							+SOverlay::Slot()
							.Padding(8.f)
							[
								SNew(SStackBox)
								.Orientation(EOrientation::Orient_Horizontal)
								+SStackBox::Slot()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("AddTemplateItemTemplateLabel", "Select an item template to add:"))
								] 
								+SStackBox::Slot()
								.Padding(8.f)
								[
									SAssignNew(ItemTemplateModSelectionBox, SComboBox<TSharedPtr<FUGCTemplateInfo>>)
									.OptionsSource(&ItemTemplatesOptions)
									.OnGenerateWidget(this, &SModioEditorUGCTemplateWidget::GenerateTemplateComboBoxWidget)
									[
										SNew(STextBlock).Text_Lambda([this]() 
											{
												auto SelectedItem = ItemTemplateModSelectionBox->GetSelectedItem();
												if (SelectedItem == nullptr)
												{
													return LOCTEXT("AddTemplateItemUnselectedItem", "Please select item to add.");
												}
												return FText::FromString(SelectedItem->Name);
											}
										)
									]
								]
							]
						]
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						[
							SNew(SButton)
							.Text(LOCTEXT("Next", "Next"))
							.IsEnabled_Lambda([this]() 
								{
									if (ItemTemplateModSelectionBox->GetSelectedItem() == nullptr)
									{
										return false;
									}

									if (ModTemplateModSelectionBox->GetSelectedItem() == nullptr)
									{
										return false;
									}

									if (CurrentSubMenu != nullptr)
									{
										return false;
									}

									return true;
								})
							.OnClicked(this, &SModioEditorUGCTemplateWidget::OnClickedAddItem)
						]
					]
					+SWidgetSwitcher::Slot()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						.AutoHeight()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(PanelBackgroundBrush)
							]
							+SOverlay::Slot()
							.Padding(8.f)
							[
								SNew(SStackBox)
								.Orientation(Orient_Horizontal)
								+SStackBox::Slot()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CreateUGCTemplateModNameLabel", "New Mod name: "))
								]
								+SStackBox::Slot()
								[
									SAssignNew(NewModNameInput, SEditableTextBox)
								]
							]
						]
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						.AutoHeight()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(PanelBackgroundBrush)
							]
							+SOverlay::Slot()
							.Padding(10.f)
							[
								SNew(SStackBox)
								.Orientation(EOrientation::Orient_Horizontal)
								+SStackBox::Slot()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("CreateUGCTemplateModLabel", "Select a mod template to create:"))
								]
								+SStackBox::Slot()
								.Padding(8.f)
								.VAlign(VAlign_Center)
								[
									SAssignNew(CreateModTemplateSelectionBox, SComboBox<TSharedPtr<FUGCTemplateInfo>>)
									.OptionsSource(&ModTemplatesOptions)
									.OnGenerateWidget(this, &SModioEditorUGCTemplateWidget::GenerateTemplateComboBoxWidget)
									[
										SNew(STextBlock).Text_Lambda([this]()
											{
												auto SelectedItem = CreateModTemplateSelectionBox->GetSelectedItem();
												if (SelectedItem == nullptr)
												{
													return LOCTEXT("CreateUGCTemplateModUnselectedMod", "Please select mod template.");
												}
												return FText::FromString(SelectedItem->Name);
											}
										)
									]
								]
							]
						]
						+SVerticalBox::Slot()
						.Padding(PanelPadding)
						[
							SNew(SButton)
							.Text(LOCTEXT("Next", "Next"))
							.IsEnabled_Lambda([this]()
								{
									if (CreateModTemplateSelectionBox->GetSelectedItem() == nullptr)
									{
										return false;
									}

									if (NewModNameInput->GetText().IsEmptyOrWhitespace())
									{
										return false;
									}

									if (CurrentSubMenu != nullptr)
									{
										return false;
									}

									return true;
								}
							)
							.OnClicked(this, &SModioEditorUGCTemplateWidget::OnClickedAddMod)
						]
					]
					+SWidgetSwitcher::Slot()
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								SNew(SImage)
								.Image(PanelBackgroundBrush)
							]
							+SOverlay::Slot()
							.Padding(8.f)
							[
								SNew(SStackBox)
								.Orientation(Orient_Horizontal)
								+SStackBox::Slot()
								.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("ExportUGCTemplateLabel", "Please select plugin to export:"))
								]
								+SStackBox::Slot()
								.Padding(8.f)
								[
									SAssignNew(TemplateExportSelectionBox, SComboBox<TSharedPtr<FUGCPluginInfo>>)
									.OptionsSource(&UGCPluginsOptions)
									.OnGenerateWidget(this, &SModioEditorUGCTemplateWidget::GeneratePluginComboBoxWidget)
									[
										SNew(STextBlock).Text_Lambda([this]()
											{
												auto SelectedItem = TemplateExportSelectionBox->GetSelectedItem();
												if (SelectedItem == nullptr)
												{
													return LOCTEXT("ExportUGCTemplateUnselectedMod", "Please select mod template.");
												}
												return FText::FromString(SelectedItem->Name);
											}
										)
									]
								]
							]
						
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							ExportDescriptorDetails.ToSharedRef()
						]
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SButton)
							.Text(LOCTEXT("Export", "Export"))
							.IsEnabled_Lambda([this]()
							{
								if (TemplateExportSelectionBox->GetSelectedItem() == nullptr)
								{
									return false;
								}

								return true;
							})
							.OnClicked(this, &SModioEditorUGCTemplateWidget::OnClickedExport)
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBox)
					.MaxDesiredHeight(300)
					.MinDesiredHeight(300)
					[
						SAssignNew(LogExpandableArea, SExpandableArea)
						.InitiallyCollapsed(true)
						.AreaTitle(LOCTEXT("Log", "Log"))
						//.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
						//.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
						.Padding(FMargin(8.f))
						.BodyContent()
						[
							SAssignNew(LogText, SMultiLineEditableTextBox)
								.IsReadOnly(true)
								.AllowMultiLine(true)
								.HScrollBar(LogHScrollBar)
								.VScrollBar(LogVScrollBar)
								.AllowContextMenu(true)
								.ContextMenuExtender(
									FMenuExtensionDelegate::CreateLambda(
									[this, Commands](FMenuBuilder& InBuilder)
									{
										InBuilder.PushCommandList(LogCommandList);
										InBuilder.AddMenuEntry(Commands.ClearLog);
									}))
						]
					]
				]
			]
		]
	];
	// clang-format on
}

void SModioEditorUGCTemplateWidget::TearDown()
{
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	TemplateSubsystem->OnTemplateLogMessage.Remove(OnTemplateLogHandle);
}

void SModioEditorUGCTemplateWidget::LoadResources()
{
	HeaderBackgroundBrush = new FSlateBrush();
	HeaderBackgroundBrush->TintColor = FLinearColor(0.025f, 0.025f, 0.025f, 1.f);
	HeaderBackgroundBrush->SetImageSize(FVector2D(64.f, 64.f));

	PanelBackgroundBrush = new FSlateBrush();
	PanelBackgroundBrush->TintColor = FLinearColor(0.015f, 0.015f, 0.015f, 1.f);

	BoldSeperatorBrush = new FSlateBrush();
	BoldSeperatorBrush->TintColor = FLinearColor(0.05, 0.05, 0.05, 1.0f);

	BackgroundBrush = new FSlateBrush();
#if ENGINE_MAJOR_VERSION >= 5
	BackgroundBrush->TintColor = FLinearColor(0.01f, 0.01f, 0.01f, 1.f);
#else
	BackgroundBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.f);
#endif

	HeaderLargeTextStyle = GetTextStyle("EmbossedText", "Normal", 14);
	HeaderSmallTextStyle = GetTextStyle("EmbossedText", "Normal", 11);

	ButtonTextStyle = GetTextStyle("EmbossedText", "Normal", 10);
}

FSlateFontInfo SModioEditorUGCTemplateWidget::GetTextStyle(FName PropertyName, FName FaceName, int32 Size)
{
	FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle(PropertyName);
	FontInfo.Size = Size;
	FontInfo.TypefaceFontName = FaceName;
	return FontInfo;
}

void SModioEditorUGCTemplateWidget::AddMessageToLog(FString Message)
{
	if (LogText == nullptr)
	{
		return;
	}

	LogText->SetIsReadOnly(false);
	LogText->InsertTextAtCursor(Message);
	LogText->SetIsReadOnly(true);
}

void SModioEditorUGCTemplateWidget::ClearLog()
{
	LogText->SetText(FText::GetEmpty());
}

FReply SModioEditorUGCTemplateWidget::ToModTemplates()
{
	if (TemplateTypeSwitcher.IsValid())
	{
		TemplateTypeSwitcher->SetActiveWidgetIndex(1);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SModioEditorUGCTemplateWidget::ToItemTemplates()
{
	if (TemplateTypeSwitcher.IsValid())
	{
		TemplateTypeSwitcher->SetActiveWidgetIndex(0);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}


FReply SModioEditorUGCTemplateWidget::ToExportTemplate() 
{
	if (TemplateTypeSwitcher.IsValid())
	{
		TemplateTypeSwitcher->SetActiveWidgetIndex(2);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SModioEditorUGCTemplateWidget::OnClickedAddItem() 
{
	auto SubMenu = GetSubMenuFor(ItemTemplateModSelectionBox->GetSelectedItem());
	SubMenu->ConfirmButton->SetOnClicked(FOnClicked::CreateLambda([this, SubMenu]() 
	{
		TMap<FString, FString> Subs;
		SubMenu->GetSubs(Subs);
		FUGCPluginInfo Mod = *(ModTemplateModSelectionBox->GetSelectedItem().Get());
		FUGCTemplateInfo Template = *(ItemTemplateModSelectionBox->GetSelectedItem().Get());

		auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
		TemplateSubsystem->AddUGCTemplateItemTo(Mod, Template, Subs);

		SubMenu->ParentWindow->HideWindow();
		FSlateApplication::Get().RequestDestroyWindow(SubMenu->ParentWindow.ToSharedRef());
		return FReply::Handled();
	}
	));
	CurrentSubMenu = SubMenu;

	return FReply::Handled();
}

FReply SModioEditorUGCTemplateWidget::OnClickedAddMod()
{
	auto SubMenu = GetSubMenuFor(CreateModTemplateSelectionBox->GetSelectedItem());
	SubMenu->ConfirmButton->SetOnClicked(FOnClicked::CreateLambda([this, SubMenu]()
	{
		TMap<FString, FString> Subs;
		SubMenu->GetSubs(Subs);
		FUGCTemplateInfo Template = *(CreateModTemplateSelectionBox->GetSelectedItem().Get());

		auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
		TemplateSubsystem->CreateUGCFromTemplate(NewModNameInput->GetText().ToString(), Template, Subs);

		SubMenu->ParentWindow->HideWindow();
		FSlateApplication::Get().RequestDestroyWindow(SubMenu->ParentWindow.ToSharedRef());
		return FReply::Handled();
	}));
	CurrentSubMenu = SubMenu;

	return FReply::Handled();
}

FReply SModioEditorUGCTemplateWidget::OnClickedExport()
{
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();

	FUGCPluginInfo PluginToExport = *(TemplateExportSelectionBox->GetSelectedItem());
	TemplateSubsystem->ExportUGCTemplate(PluginToExport, ExportDescriptor, true);
	return FReply::Handled();
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item->Name); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::GenerateTemplateComboBoxWidget(TSharedPtr<FUGCTemplateInfo> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item->Name); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedPtr<class SUGCTemplateSubsitutionWidget> SModioEditorUGCTemplateWidget::GetSubMenuFor(TSharedPtr<FUGCTemplateInfo> Template)
{
	TSharedPtr<SUGCTemplateSubsitutionWidget> SubMenu = nullptr;
	TSharedPtr<SWindow> SubWindow = SNew(SWindow)
										.Title(FText::FromString("Configure"))
										.SupportsMaximize(false)
										.SupportsMinimize(false)
										.HasCloseButton(true)
										.ClientSize(FVector2D(1000.f, 720.f))
										.SizingRule(ESizingRule::FixedSize)
										.AutoCenter(EAutoCenter::PreferredWorkArea)
										.ScreenPosition(FVector2D(0, 0))
										.LayoutBorder(FMargin(3.f))[SAssignNew(SubMenu, SUGCTemplateSubsitutionWidget)];
	
	SubWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>& WindowRef) 
		{
			CurrentSubMenu = nullptr;
		}
	));
	
	SubMenu->BuildFor(Template);
	SubMenu->ParentWindow = SubWindow;
	FSlateApplication::Get().AddWindow(SubWindow.ToSharedRef(), true);
	SubWindow->BringToFront(true);

	return SubMenu;
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


