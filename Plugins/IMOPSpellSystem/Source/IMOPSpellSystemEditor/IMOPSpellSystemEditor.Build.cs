using UnrealBuildTool;

public class IMOPSpellSystemEditor : ModuleRules
{
	public IMOPSpellSystemEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"IMOPSpellSystemRuntime",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"Projects"
		});
	}
}