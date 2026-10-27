using UnrealBuildTool;
public class TeniParizha : ModuleRules
{
	public TeniParizha(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "AIModule", "NavigationSystem", "ProceduralMeshComponent", "RenderCore", "RHI" });
	}
}
