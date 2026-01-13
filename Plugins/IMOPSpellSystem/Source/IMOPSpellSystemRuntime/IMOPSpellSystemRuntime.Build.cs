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
			"EnhancedInput"
		});


		PrivateDependencyModuleNames.AddRange(new string[] {
			"Projects",      // IPluginManager (falls du es noch irgendwo nutzt)
			"StructUtils",   // FInstancedStruct (habt ihr in Phase 1)
			"UMG",            // nur falls UI; sonst weg
			"PhysicsCore" // falls du Overlaps/Collision tiefer nutzt (oft nicht nötig, aber safe)
		});
	}
}