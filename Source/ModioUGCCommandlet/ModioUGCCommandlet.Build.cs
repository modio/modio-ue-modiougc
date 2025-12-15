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

public class ModioUGCCommandlet : ModuleRules
{
    public ModioUGCCommandlet(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
            new string[] 
            {
                    "Core",
                    "CoreUObject",
                    "Engine",
            });

        PublicDependencyModuleNames.AddRange(
            new string[] 
            {
                "ModioUGCEditor",
            });

    }
}