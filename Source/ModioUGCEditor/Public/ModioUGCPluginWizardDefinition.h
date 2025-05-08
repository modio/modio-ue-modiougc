/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

#pragma once

// Depends on code from the plugin browser to work correctly
#include "Features/IPluginsEditorFeature.h"
#include "IPluginWizardDefinition.h"

class FModioUGCPluginWizardDefinition : public IPluginWizardDefinition
{
public:
	FModioUGCPluginWizardDefinition(bool bContentOnlyProject = false);

	// Begin IPluginWizardDefinition interface
	virtual const TArray<TSharedRef<FPluginTemplateDescription>>& GetTemplatesSource() const override;
	virtual void OnTemplateSelectionChanged(TSharedPtr<FPluginTemplateDescription> InSelectedItem,
											ESelectInfo::Type SelectInfo) override;
	virtual TSharedPtr<FPluginTemplateDescription> GetSelectedTemplate() const override;
	virtual void ClearTemplateSelection() override;
	virtual bool HasValidTemplateSelection() const override;

	virtual bool CanShowOnStartup() const override
	{
		return false;
	}
	virtual bool HasModules() const override;
	virtual bool IsMod() const override
	{
		return true;
	}
	virtual void OnShowOnStartupCheckboxChanged(ECheckBoxState CheckBoxState) override {}
	virtual ECheckBoxState GetShowOnStartupCheckBoxState() const override
	{
		return ECheckBoxState();
	}
	virtual TSharedPtr<class SWidget> GetCustomHeaderWidget() override
	{
		return nullptr;
	}
	virtual FText GetInstructions() const override;

	virtual bool GetPluginIconPath(FString& OutIconPath) const override;
	virtual EHostType::Type GetPluginModuleDescriptor() const override;
	virtual ELoadingPhase::Type GetPluginLoadingPhase() const override;
	virtual bool GetTemplateIconPath(TSharedRef<FPluginTemplateDescription> InTemplate,
									 FString& OutIconPath) const override;
	virtual FString GetPluginFolderPath() const override;
	virtual TArray<FString> GetFoldersForSelection() const override;
	virtual void PluginCreated(const FString& PluginName, bool bWasSuccessful) const override;
	// End IPluginWizardDefinition interface

private:
	/** Creates the templates that can be used by the plugin manager to generate the plugin */
	void PopulateTemplatesSource();

	/** Gets the folder for the specified template. */
	FString GetFolderForTemplate(TSharedRef<FPluginTemplateDescription> InTemplate) const;

	/** The available templates for the mod. They should function as mixins to the backing template */
	TArray<TSharedRef<FPluginTemplateDescription>> TemplateDefinitions;

	/** The content that will be used when creating the mod */
	TSharedPtr<FPluginTemplateDescription> CurrentTemplateDefinition;

	/** The base directory of this plugin. Used for accessing the templates used to create UGC */
	FString PluginBaseDir;

	/** The backing template definition for the mod. This should never be directly selectable */
	TSharedPtr<FPluginTemplateDescription> BackingTemplate;

	/**
	 * The path to the template that ultimately serves as the template that the mod will be based on. It's not intended
	 * to be selected directly, but rather other templates will act as mixins to define what content will exist in the
	 * plugin.
	 */
	FString BackingTemplatePath;

	/** If true, this definition is for a project that can only contain content */
	bool bIsContentOnlyProject;

	/** Custom header widget */
	TSharedPtr<class SWidget> CustomHeaderWidget;

	/** Brush used for drawing the custom header widget */
	TSharedPtr<struct FSlateDynamicImageBrush> IconBrush;
};

struct FModioUGCPluginTemplateDescription : public FPluginTemplateDescription
{
	FModioUGCPluginTemplateDescription(FText InName, FText InDescription, FString InOnDiskPath,
									   bool InCanContainContent, EHostType::Type InModuleDescriptorType,
									   ELoadingPhase::Type InLoadingPhase = ELoadingPhase::Default)
		: FPluginTemplateDescription(InName, InDescription, InOnDiskPath, InCanContainContent, InModuleDescriptorType,
									 InLoadingPhase)
	{}

	virtual void OnPluginCreated(TSharedPtr<IPlugin> NewPlugin);
};