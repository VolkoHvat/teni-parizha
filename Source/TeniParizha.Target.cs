using UnrealBuildTool;
using System.Collections.Generic;

public class TeniParizhaTarget : TargetRules
{
	public TeniParizhaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("TeniParizha");
	}
}
