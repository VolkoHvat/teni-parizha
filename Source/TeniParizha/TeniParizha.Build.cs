using UnrealBuildTool;

public class TeniParizha : ModuleRules
{
	public TeniParizha(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"ProceduralMeshComponent", "AIModule", "NavigationSystem", "GameplayTasks",
			"UMG", "Slate", "SlateCore", "RenderCore"
		});
	}
}
