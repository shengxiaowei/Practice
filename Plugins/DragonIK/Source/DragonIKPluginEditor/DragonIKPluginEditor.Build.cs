/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

using UnrealBuildTool;
using System;
public class DragonIKPluginEditor : ModuleRules
{
	public DragonIKPluginEditor(ReadOnlyTargetRules Target): base(Target)
	{

        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        
        PublicIncludePaths.AddRange(
			new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Public")
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                System.IO.Path.Combine(ModuleDirectory,"Private")
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "DragonIKPlugin"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore", "AnimGraph", "BlueprintGraph",
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
