// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System;

public class DragonIKPlugin : ModuleRules
{
    public DragonIKPlugin(ReadOnlyTargetRules Target): base(Target)
    {


        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        Console.WriteLine("Mod Dir : " + ModuleDirectory);

        PublicIncludePaths.AddRange(
            new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public")
//                  "DragonIKPlugin/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Private"),
				// ... add other private include paths required here ...
			}
            );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore","AnimationCore",
				// ... add private dependencies that you statically link with here ...	
			}
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                { 
                    // ... add any modules that your module loads dynamically here ...
                }
                );
        
    
	}
}
