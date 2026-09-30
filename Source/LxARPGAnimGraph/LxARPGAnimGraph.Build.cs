using UnrealBuildTool;

/// <summary>动作图节点在编辑和烘焙阶段可用，打包运行时仅保留运行时播放器。</summary>
public class LxARPGAnimGraph : ModuleRules
{
	/// <summary>配置原生动画图编译与动作节点依赖。</summary>
	public LxARPGAnimGraph(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "LxARPG", "AnimGraph", "AnimGraphRuntime", "BlueprintGraph" });
		PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "KismetCompiler", "GameplayTags", "ToolMenus", "Slate", "SlateCore" });
	}
}
