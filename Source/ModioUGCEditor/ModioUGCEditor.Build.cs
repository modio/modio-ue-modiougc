/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

using UnrealBuildTool;

public class ModioUGCEditor : ModuleRules
{
    public ModioUGCEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.Add("ModioUGC");

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "PluginBrowser"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Projects",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                "EditorStyle",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Json",
                "DeveloperToolSettings",
                "PakFile",
                "EditorWidgets",
                "UATHelper"
            }
        );
    }
}