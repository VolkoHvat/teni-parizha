using UnrealBuildTool;
using System.Collections.Generic;

public class TeniParizhaTarget : TargetRules
{
	public TeniParizhaTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TeniParizha");
	}
}
