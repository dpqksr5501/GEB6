// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class KHU_GEB : ModuleRules
{
	public KHU_GEB(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"KHU_GEB",
			"KHU_GEB/Variant_Platforming",
			"KHU_GEB/Variant_Platforming/Animation",
			"KHU_GEB/Variant_Combat",
			"KHU_GEB/Variant_Combat/AI",
			"KHU_GEB/Variant_Combat/Animation",
			"KHU_GEB/Variant_Combat/Gameplay",
			"KHU_GEB/Variant_Combat/Interfaces",
			"KHU_GEB/Variant_Combat/UI",
			"KHU_GEB/Variant_SideScrolling",
			"KHU_GEB/Variant_SideScrolling/AI",
			"KHU_GEB/Variant_SideScrolling/Gameplay",
			"KHU_GEB/Variant_SideScrolling/Interfaces",
			"KHU_GEB/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
