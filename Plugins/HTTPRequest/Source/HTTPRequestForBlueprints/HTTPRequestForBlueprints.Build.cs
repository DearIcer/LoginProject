// Copyright (c) 2023 King Wai Mark

using UnrealBuildTool;

public class HTTPRequestForBlueprints : ModuleRules
{
	public HTTPRequestForBlueprints(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"Json", 
				"JsonUtilities",
				"XmlParser"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"HTTP"
			}
			);
		
	}
}
