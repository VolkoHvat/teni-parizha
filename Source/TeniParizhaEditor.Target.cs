using UnrealBuildTool;
using System.Collections.Generic;

public class TeniParizhaEditorTarget : TargetRules
{
	public TeniParizhaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("TeniParizha");
	}
}
