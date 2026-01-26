using UnrealBuildTool;

public class IMOPSpellSystemRuntime : ModuleRules
{
	public IMOPSpellSystemRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"NetCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Projects",
			"StructUtils",
			"PhysicsCore"
		});

		// Not strictly required, but helps when adding new folders rapidly.
		PublicIncludePaths.AddRange(new[] { "IMOPSpellSystemRuntime/Public" });
		PrivateIncludePaths.AddRange(new[] { "IMOPSpellSystemRuntime/Private" });
	}
}