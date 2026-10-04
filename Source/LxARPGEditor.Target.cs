// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class LxARPGEditorTarget : TargetRules
{
	public LxARPGEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7; // 使用 UE 5.8 的默认编译规则。
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8; // 与 UE 5.8 的头文件包含顺序保持一致。
		ExtraModuleNames.AddRange(new string[] { "LxARPG", "LxARPGEditor" });
	}
}
