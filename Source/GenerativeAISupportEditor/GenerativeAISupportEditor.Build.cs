// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.


using UnrealBuildTool;

public class GenerativeAISupportEditor : ModuleRules
{
	public GenerativeAISupportEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GenerativeAISupport",  // Depend on the runtime module
				"Json",                 // Add JSON support for serialization
				"JsonUtilities"  
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"WorkspaceMenuStructure",
				"Projects",
				"EditorScriptingUtilities",
				"Blutility",
				"MaterialEditor",
				"MaterialUtilities",
				"BlueprintGraph",
				"UMGEditor",
				"UMG",
				"Settings",
				"FunctionalTesting",     // For AutomationBlueprintFunctionLibrary 
				"SourceControl"    // For Source Control integration
			}
		);
	}
}