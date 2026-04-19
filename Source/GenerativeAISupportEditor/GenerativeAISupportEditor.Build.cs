// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.


using System.IO;
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

		string FabPrivatePath = Path.Combine(EngineDirectory, "Plugins", "Fab", "Source", "Fab", "Private");
		bool bHasFabPrivateHeaders = Directory.Exists(FabPrivatePath);
		PublicDefinitions.Add($"GENAI_WITH_FAB={(bHasFabPrivateHeaders ? 1 : 0)}");

		if (bHasFabPrivateHeaders)
		{
			PrivateIncludePaths.Add(FabPrivatePath);
		}

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Slate",
				"SlateCore",
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
				"SourceControl",   // For Source Control integration
				"HTTP",
				"WebBrowser"
			}
		);

		if (bHasFabPrivateHeaders)
		{
			PrivateDependencyModuleNames.Add("Fab");
		}
	}
}
