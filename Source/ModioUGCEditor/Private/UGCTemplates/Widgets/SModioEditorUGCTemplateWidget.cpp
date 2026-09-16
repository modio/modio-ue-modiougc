// Fill out your copyright notice in the Description page of Project Settings.


#include "UGCTemplates/Widgets/SModioEditorUGCTemplateWidget.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "UGCTemplates/Widgets/SUGCTemplateSubsitutionWidget.h"
#include "UGCTemplates/UGCTemplateDescriptor.h"
#include "../Private/SDetailsView.h"
#include "Interfaces/IPluginManager.h"
#include "UGCTemplates/Widgets/SUGCTemplateTileWidget.h"
#include "../../Slate/Public/Widgets/Views/STileView.h"
#include "UGCTemplates/Widgets/SUGCTemplateDetailsWidget.h"
#include "../../Slate/Public/Widgets/Views/SListView.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/ContentBrowser/Public/IContentBrowserSingleton.h"
#include "ModioUGCEditor.h"
#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

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
	Mode = InArgs._Mode;

	LoadResources();

	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	OnTemplateLogHandle = TemplateSubsystem->OnTemplateLogMessage.AddRaw(this, &SModioEditorUGCTemplateWidget::AddMessageToLog);
	UpdateSources();

	IPluginManager& PluginManager = IPluginManager::Get();
	OnPluginCreatedHandle = PluginManager.OnNewPluginCreated().AddRaw(this, &SModioEditorUGCTemplateWidget::NewPluginDetected);
	OnPluginMountedHandle = PluginManager.OnNewPluginMounted().AddRaw(this, &SModioEditorUGCTemplateWidget::NewPluginDetected);

	ExportDescriptor = NewObject<UUGCTemplateDescriptor>();

	LoadCategories();

	const FModioEditorUGCTemplateWidgetCommands& Commands = FModioEditorUGCTemplateWidgetCommands::Get();

	LogCommandList->MapAction(Commands.ClearLog, FExecuteAction::CreateSP(this, &SModioEditorUGCTemplateWidget::ClearLog));

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
	ExportDescriptorDetails = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	ExportDescriptorDetails->SetObject(ExportDescriptor);

	const TSharedPtr<SScrollBar> LogHScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Horizontal);
	const TSharedPtr<SScrollBar> LogVScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Vertical);

// clang-format off

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.FillHeight(4.0f)
			[
				SNew(SBox)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(0.0f,0.0f,2.0f,4.0f)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SImage).Image(HeaderBackgroundBrush)
						]
						+SOverlay::Slot()
						[
							//Categories
							CreateCatergoriesWidget()
						]
					]
					+SHorizontalBox::Slot()
					.Padding(2.0f,0.0f,2.0f,4.0f)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Fill)
						[
							SNew(SImage).Image(HeaderBackgroundBrush)
						]
						+SOverlay::Slot()
						[
							//Templates
							CreateTemplatesWidget()
						]
					]
					+SHorizontalBox::Slot()
					.Padding(2.0f,0.0f,0.0f,4.0f)
					[
						//Details
						CreateDetailsWidget()
					]
				]
			]
			+SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				CreateOptionsWidget()
			]
		]
		//+SVerticalBox::Slot()
		//.AutoHeight()
		//[
		//	SNew(SBox)
		//	.MaxDesiredHeight(300)
		//	.MinDesiredHeight(300)
		//	[
		//		SAssignNew(LogExpandableArea, SExpandableArea)
		//		.InitiallyCollapsed(true)
		//		.AreaTitle(LOCTEXT("Log", "Log"))
		//		//.AreaTitleFont(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
		//		//.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f))
		//		.Padding(FMargin(8.f))
		//		.BodyContent()
		//		[
		//			SAssignNew(LogText, SMultiLineEditableTextBox)
		//				.IsReadOnly(true)
		//				.AllowMultiLine(true)
		//				.HScrollBar(LogHScrollBar)
		//				.VScrollBar(LogVScrollBar)
		//				.AllowContextMenu(true)
		//				.ContextMenuExtender(
		//					FMenuExtensionDelegate::CreateLambda(
		//					[this, Commands](FMenuBuilder& InBuilder)
		//					{
		//						InBuilder.PushCommandList(LogCommandList);
		//						InBuilder.AddMenuEntry(Commands.ClearLog);
		//					}))
		//		]
		//	]
		//]
	];

	SetDefaultTemplateSelection();

	if (InArgs._Context != nullptr && Mode == EUGCTemplateType::TT_Item)
	{
		ModTemplateModSelectionBox->SetSelectedItem(GetPluginInfoFrom(InArgs._Context));
	}
	// clang-format on
}

void SModioEditorUGCTemplateWidget::TearDown()
{
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	TemplateSubsystem->OnTemplateLogMessage.Remove(OnTemplateLogHandle);

	IPluginManager& PluginManager = IPluginManager::Get();
	PluginManager.OnNewPluginCreated().Remove(OnPluginCreatedHandle);
	PluginManager.OnNewPluginMounted().Remove(OnPluginMountedHandle);
}

void SModioEditorUGCTemplateWidget::LoadResources()
{
	HeaderBackgroundBrush = new FSlateBrush();
	HeaderBackgroundBrush->TintColor = FLinearColor(0.025f, 0.025f, 0.025f, 1.f);
	HeaderBackgroundBrush->SetImageSize(FVector2D(64.f, 64.f));

	PanelBackgroundBrush = new FSlateBrush();
	PanelBackgroundBrush->TintColor = FLinearColor(0.015f, 0.015f, 0.015f, 1.f);

	BoldSeperatorBrush = new FSlateBrush();
	BoldSeperatorBrush->TintColor = FLinearColor(0.05f, 0.05f, 0.05f, 1.0f);

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

void SModioEditorUGCTemplateWidget::UpdateSources()
{
	//Clear any cached data
	UGCPlugins.Empty();
	Templates.Empty();

	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	TemplateSubsystem->DiscoverUGC(UGCPlugins);
	TemplateSubsystem->DiscoverTemplates(Templates);
	UGCPluginsOptions = ConvertToOptionsSource(UGCPlugins);
	TemplatesOptions = ConvertToOptionsSource(Templates);
	ItemTemplatesOptions = TemplatesOptions.FilterByPredicate([](const TSharedPtr<FUGCTemplateInfo>& Key) -> bool {	return Key->Descriptor->Type == EUGCTemplateType::TT_Item; });
	ModTemplatesOptions = TemplatesOptions.FilterByPredicate([](const TSharedPtr<FUGCTemplateInfo>& Key) -> bool { return Key->Descriptor->Type == EUGCTemplateType::TT_Mod; });
}

void SModioEditorUGCTemplateWidget::NewPluginDetected(IPlugin& NewPlugin)
{
	UpdateSources();
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

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::CreateCatergoriesWidget() 
{
	//clang-format off
	auto CategoryWidget = 
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SBox)
			[
				SNew(SScrollBox)
				+SScrollBox::Slot()
				[
					SNew(SListView<TSharedPtr<FUGCTemplateCategoryView>>)
					//.ItemHeight(64.0f)
					.ListItemsSource(&Categories)
					.OnSelectionChanged(this, &SModioEditorUGCTemplateWidget::OnCategoryClicked)
					.OnGenerateRow_Lambda([this](TSharedPtr<FUGCTemplateCategoryView> InItem, const TSharedRef<STableViewBase>& OwnerTable)
							{
								return SNew(STableRow<TSharedPtr<FUGCTemplateInfo>>, OwnerTable)
								.Padding(0.0f)
								[
									SNew(SUGCTemplateCategoryTile)
									.CategoryInfo(InItem)
								];
							})
				]
			]
		];
	
	return CategoryWidget;
	
	//clang-format on
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::CreateTemplatesWidget()
{
	FilteredTemplateOptions = (Mode == EUGCTemplateType::TT_Mod ? ModTemplatesOptions : ItemTemplatesOptions);

	//clang-format off
	return SNew(SBox)
		.WidthOverride(256)
		.MinDesiredWidth(256)
		.MaxDesiredWidth(256)
		[
			SAssignNew(TemplateView, STileView<TSharedPtr<FUGCTemplateInfo>>)
				 .ItemAlignment(EListItemAlignment::LeftAligned)
				 .ListItemsSource(&FilteredTemplateOptions)
				 .OnSelectionChanged(this, &SModioEditorUGCTemplateWidget::OnListSelectionChanged)
				 .ItemWidth(75)
				 .ItemHeight(96)
				 .SelectionMode(ESelectionMode::Single)
				 .OnGenerateTile_Lambda(
					 [](TSharedPtr<FUGCTemplateInfo> TemplateInfoPtr, const TSharedRef<STableViewBase>& OwnerTable) {
						 return SNew(STableRow<TSharedPtr<FUGCTemplateInfo>>, OwnerTable)
							.Padding(FMargin(5.0f, 0.0f))
							[
								SNew(SUGCTemplateTileWidget)
								.TemplateInfo(*TemplateInfoPtr)
							];
					 })
		];
	//clang-format on
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::CreateDetailsWidget()
{
	return SAssignNew(TemplateDetailsWidget, SUGCTemplateDetailsWidget);
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::CreateOptionsWidget()
{
	FText NameLabel = (Mode == EUGCTemplateType::TT_Mod ? LOCTEXT("UGCTemplateOptionWidgetLabelMod",	"Mod Name: ")
														: LOCTEXT("UGCTemplateOptionWidgetLabelItem",	"Item Name: "));
	
	//clang-format off
	auto OptionsWidget =
		SNew(SOverlay)
		+SOverlay::Slot()
		//.Padding(4.0f)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SImage).Image(HeaderBackgroundBrush)
		]
		+SOverlay::Slot()
		[
			SNew(SVerticalBox) 
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBox)
				.HeightOverride(75)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.Padding(8.0f)
					[
						SNew(SBox)
						.VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox) 
							+SHorizontalBox::Slot()
		   					.AutoWidth()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.SizeParam(FAuto())
							[
								SNew(STextBlock)
								.Text(NameLabel)
							]
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.SizeParam(FAuto())
							[
								SNew(SBox)
								.WidthOverride(500)
								[
									SAssignNew(NewNameText, SEditableTextBox)
								]
							]
						]
					]
					+SHorizontalBox::Slot()
					.Padding(8.0f)
					[
						SNew(SBox)
						.Visibility_Lambda([this]()
							{
								 return Mode == EUGCTemplateType::TT_Item ? EVisibility::Visible : EVisibility::Collapsed;
							})
								  .VAlign(VAlign_Center)
								  .HAlign(HAlign_Right)
						[
							SNew(SHorizontalBox) 
							+SHorizontalBox::Slot()
							//.HAlign(HAlign_Right)
							.VAlign(VAlign_Center)
							.AutoWidth()
							[
								SNew(STextBlock)
								.Text(LOCTEXT("UGCTemplateModSelectionBoxLabel", "Mod: "))
							] 
							+SHorizontalBox::Slot()
							//.HAlign(HAlign_Right)
							.AutoWidth()
							[
								SNew(SBox)
								.WidthOverride(300)
								[
									SAssignNew(ModTemplateModSelectionBox,SComboBox<TSharedPtr<FUGCPluginInfo>>)
									.OptionsSource(&UGCPluginsOptions)
									.OnGenerateWidget(this, &SModioEditorUGCTemplateWidget::GeneratePluginComboBoxWidget)
									[
										SNew(STextBlock)
										.Text_Lambda([this]()
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
				]
			] 
			+
			SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				.Padding(8.0f)
				.AutoWidth()
				[
					SNew(SBox)
					[
						SNew(SButton)
						.IsEnabled_Lambda([this]() { return SelectedTemplate != nullptr && !NewNameText->GetText().IsEmpty(); })
						.Text(LOCTEXT("UGCTemplateOptionWidgetCreateLabel", "Create"))
						.OnClicked(this, &SModioEditorUGCTemplateWidget::OnCreateClicked)
					]
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Top)
				.Padding(8.0f)
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("UGCTemplateOptionWidgetCancelLabel", "Cancel"))
					.OnClicked(this, &SModioEditorUGCTemplateWidget::OnCancelClicked)
				]
			]
		];

	//clang-format on
	return OptionsWidget;
}

FReply SModioEditorUGCTemplateWidget::OnCreateClicked()
{
	TArray<FString> FocusPaths;
	TMap<FString, FString> Subs;
	TemplateDetailsWidget->GetSubs(Subs);
	FUGCTemplateInfo Template = *(SelectedTemplate.Get());

	bool bSuccess = true;
	FUGCTemplateResult Result;
	FString UGCName = NewNameText->GetText().ToString();

	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	if (Mode == EUGCTemplateType::TT_Mod)
	{
		bSuccess = TemplateSubsystem->CreateUGCFromTemplate(UGCName, Template, Subs, Result);
		if (bSuccess)
		{
			FocusPaths.Add(Result.Plugin->GetBaseDir());
		}
		DisplayNewModNotification(bSuccess, FocusPaths);
	}
	else	
	{
		FUGCPluginInfo Mod = *(ModTemplateModSelectionBox->GetSelectedItem().Get());
		bSuccess = TemplateSubsystem->AddUGCTemplateItemTo(Mod, Template, Subs, UGCName);
		if (bSuccess)
		{
			FocusPaths.Add(Mod.Plugin->GetBaseDir());
		}
		DisplayAddItemNotification(bSuccess, FocusPaths);
		Result.Plugin = Mod.Plugin;
	}

	if (bSuccess)
	{
		FModioUGCEditorModule& ModioUGCEditorModule = FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
		ModioUGCEditorModule.DismissUGCTemplateWindow();
	}

	return FReply::Handled();
}

FReply SModioEditorUGCTemplateWidget::OnCancelClicked()
{
	FModioUGCEditorModule& ModioUGCEditorModule = FModuleManager::LoadModuleChecked<FModioUGCEditorModule>("ModioUGCEditor");
	ModioUGCEditorModule.DismissUGCTemplateWindow();

	return FReply::Handled();
}

void SModioEditorUGCTemplateWidget::LoadCategories()
{
	auto TemplateSubsystem = GEditor->GetEditorSubsystem<UUGCTemplateSubsystem>();
	if (TemplateSubsystem == nullptr)
	{
		return;
	}

	TArray<FUGCTemplateCategoryView> CategoryInfo;
	TemplateSubsystem->GetCategories(CategoryInfo);

	FUGCTemplateCategoryView AllCategory;
	AllCategory.Category = "";
	AllCategory.DisplayName = LOCTEXT("UUGCTemplateAllCategoryLabel", "All");
	AllCategory.Image =	FSoftObjectPath::ConstructFromAssetPath(FTopLevelAssetPath("/Engine/ArtTools/RenderToTexture/Textures/127grey", "127grey"));

	CategoryInfo.Insert(AllCategory, 0);

	for (auto Category : CategoryInfo)
	{
		Categories.Add(MakeShareable(new FUGCTemplateCategoryView(Category)));
	}
}

void SModioEditorUGCTemplateWidget::OnListSelectionChanged(TSharedPtr<FUGCTemplateInfo> Item, ESelectInfo::Type SelectInfo)
{
	SelectedTemplate = Item;
	if (TemplateDetailsWidget == nullptr || Item == nullptr)
	{
		return;
	}

	TemplateDetailsWidget->Refresh(*Item);
}

void SModioEditorUGCTemplateWidget::OnCategoryClicked(TSharedPtr<FUGCTemplateCategoryView> CategoryTile, ESelectInfo::Type SelectInfo)
{
	if (CategoryTile == nullptr)
	{
		return;
	}

	auto Category = CategoryTile->Category;
	auto OptionsToFilter = (Mode == EUGCTemplateType::TT_Mod ? &ModTemplatesOptions : &ItemTemplatesOptions);
	if (Category.IsEmpty())
	{
		FilteredTemplateOptions = *OptionsToFilter;
	}
	else
	{
		FilteredTemplateOptions = OptionsToFilter->FilterByPredicate([Category](const TSharedPtr<FUGCTemplateInfo>& Key) -> bool { return Key->Descriptor->Category.Equals(Category); });
	}
	
	TemplateView->SetItemsSource(&FilteredTemplateOptions);
	TemplateView->RebuildList();

	SetDefaultTemplateSelection();
}

TSharedRef<SWidget> SModioEditorUGCTemplateWidget::GeneratePluginComboBoxWidget(TSharedPtr<FUGCPluginInfo> Item)
{
	return SNew(STextBlock)
		.Text_Lambda([Item]() { return FText::FromString(*Item->Name); })
		.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")));
}

TSharedPtr<FUGCPluginInfo> SModioEditorUGCTemplateWidget::GetPluginInfoFrom(TSharedPtr<IPlugin> Context)
{
	for (auto PluginInfo : UGCPluginsOptions)
	{
		if (PluginInfo->Plugin == Context)
		{
			return PluginInfo;
		}
	}

	return nullptr;
}

void SModioEditorUGCTemplateWidget::SetDefaultTemplateSelection()
{
	bool bHasPossibleSelections = FilteredTemplateOptions.Num() > 0;
	TemplateDetailsWidget->SetVisibility(bHasPossibleSelections ? EVisibility::Visible
																: EVisibility::Hidden);

	if (bHasPossibleSelections)
	{
		TemplateView->SetItemSelection(FilteredTemplateOptions[0], true);
	}
}

void SModioEditorUGCTemplateWidget::DisplayNewModNotification(bool bSuccess, const TArray<FString>& FocusPaths)
{
	FText NotificationText = (bSuccess ? LOCTEXT("UGCNewModSuccessNotificationText",	"Succcess")
									   : LOCTEXT("UGCNewModFailNotificationText",		"Fail"));

	SNotificationItem::ECompletionState NotificationStatus = SNotificationItem::CS_None;
	
	FNotificationInfo Info(NotificationText);
	Info.bFireAndForget = true;
	Info.bUseSuccessFailIcons = true;
	Info.ExpireDuration = 10.0f;
	
	if (bSuccess)
	{
		Info.SubText = LOCTEXT("UGCNewModSuccessNotificationText", "New mod created successfully");
		Info.HyperlinkText = FText::FromString("Show in content browser");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([FocusPaths]() {
			IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
			ContentBrowser.SyncBrowserToFolders(FocusPaths);
		});

		NotificationStatus = SNotificationItem::CS_Success;
	}
	else
	{
		Info.HyperlinkText = FText::FromString("Open Log");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([]() {
			FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog"));
		});
		Info.SubText = LOCTEXT("UGCNewModFailNotificationText", "Could not create new mod");

		NotificationStatus = SNotificationItem::CS_Fail;
	}

	FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(NotificationStatus);

}

void SModioEditorUGCTemplateWidget::DisplayAddItemNotification(bool bSuccess, const TArray<FString>& FocusPaths)
{
	FText NotificationText = (bSuccess ? LOCTEXT("UGCAddItemSuccessNotificationText", "Succcess")
									   : LOCTEXT("UGCAddItemFailNotificationText", "Fail"));

	SNotificationItem::ECompletionState NotificationStatus = SNotificationItem::CS_None;

	FNotificationInfo Info(NotificationText);
	Info.bFireAndForget = true;
	Info.bUseSuccessFailIcons = true;
	Info.ExpireDuration = 10.0f;

	if (bSuccess)
	{
		Info.SubText = LOCTEXT("UGCAddItemSuccessNotificationText", "New item added successfully");
		Info.HyperlinkText = FText::FromString("Show in content browser");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([FocusPaths]() {
			IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
			ContentBrowser.SyncBrowserToFolders(FocusPaths);
		});

		NotificationStatus = SNotificationItem::CS_Success;
	}
	else
	{
		Info.HyperlinkText = FText::FromString("Open Log");
		Info.Hyperlink = FSimpleDelegate::CreateLambda([]() { FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog")); });
		Info.SubText = LOCTEXT("UGCAddItemFailNotificationText", "Could not add new item to mod");

		NotificationStatus = SNotificationItem::CS_Fail;
	}

	FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(NotificationStatus);
}

#undef LOCTEXT_NAMESPACE

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


