// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

/// <summary>
/// 项目专用编辑器模块，提供任务系列资产的表格与关系图编辑能力。
/// </summary>
public class LxARPGEditor : ModuleRules
{
	/// <summary>
	/// 配置任务系列编辑器需要的运行时与编辑器模块依赖。
	/// </summary>
	public LxARPGEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"LxARPG",
			"AnimGraph",
			"AnimGraphRuntime"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"LxARPGAnimGraph",
			"ApplicationCore",
			"AssetTools",
			"BlueprintGraph",
			"KismetCompiler",
			"EditorFramework",
			"GraphEditor",
			"GameplayTags",
			"GameplayTagsEditor",
			"InputCore",
			"ImageCore",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
