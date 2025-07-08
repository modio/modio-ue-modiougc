/*
 *  Copyright (C) 2025 mod.io Pty Ltd. <https://mod.io>
 *
 *  This file is part of the mod.io ModioUGC Plugin.
 *
 *  Distributed under the MIT License. (See accompanying file LICENSE or
 *   view online at <https://github.com/modio/modio-ue-modiougc/blob/main/LICENSE>)
 *
 */

using System.IO;
using UnrealBuildTool;

public class ModioUGC : ModuleRules
{
    public ModioUGC(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "Modio"
            }
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Modio",
                "Projects",
                "AssetRegistry",
                "PakFile",
                "RenderCore",
                "DeveloperSettings",
                "Projects", // Accessing FEngineVersion
                "Json"
            }
        );

        PublicDefinitions.AddRange(new string[]
        {
            "UGC_SUPPORTED_PLATFORM=1",
            "MODIO_UGC_SUPPORTED_PLATFORM=1"
        });

        PublicIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Public")
        });

        PrivateIncludePaths.AddRange(new string[]
        {
            Path.Combine(ModuleDirectory, "Private")
        });
    }
}