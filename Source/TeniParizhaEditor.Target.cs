using UnrealBuildTool;
using System.Collections.Generic;

public class TeniParizhaEditorTarget : TargetRules
{
	public TeniParizhaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TeniParizha");
	}
}
