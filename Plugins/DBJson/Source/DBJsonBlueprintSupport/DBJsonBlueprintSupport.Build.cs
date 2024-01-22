// Copyright 2021-2022, DearBing. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class DBJsonBlueprintSupport : ModuleRules
	{
		public DBJsonBlueprintSupport(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{ 
					"Core", 
					"CoreUObject", 
					"Engine",
					"UnrealEd",
					"Slate",
					"SlateCore",
					"KismetWidgets",
					"KismetCompiler",
					"BlueprintGraph",
					"Json",
					"DBJson"
				}
			);
		}
	}
}
