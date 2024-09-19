// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PrettyPostProcess : ModuleRules
{
	public PrettyPostProcess(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// Needed to include the engine Lens Flare post-process header
				EngineDirectory + "/Source/Runtime/Renderer/Private"
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				// Needed for RenderGraph, PostProcess, Shaders
				"Core",
				"RHI",
				"Renderer",
				"RenderCore",
				"Projects"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"DeveloperSettings",
				"Slate",
				"SlateCore"
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
