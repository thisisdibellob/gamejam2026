// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class gamejam2026 : ModuleRules
{
	public gamejam2026(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });
	}
}
