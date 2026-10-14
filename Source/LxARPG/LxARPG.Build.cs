// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LxARPG : ModuleRules
{
	public LxARPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			
			"Core", 
			"CoreUObject", 
			"Engine",
			"Niagara", // 元素异常依附单元的粒子表现。
			"AnimGraphRuntime",
			"InputCore",
			"EnhancedInput",
			"UMG", 
			"HeadMountedDisplay",
			"Slate", 
			"SlateCore", 
			"Json", 
			"JsonUtilities",
			"GameplayTags", 
			"DeveloperSettings",
			"ModularGameplay",
			"AIModule",
			"NavigationSystem",
			"ProceduralMeshComponent",
			"Landscape", // 海洋自动读取地形外接矩形。
			"RHI", // 上传碰撞接触浪花的动态遮罩。
			"RenderCore"
		});

		// 整墙静态网格在编辑器和运行时共用网格描述构建路径。
		PrivateDependencyModuleNames.AddRange(new string[] { "MeshDescription", "StaticMeshDescription", "GeometryCore" }); // 圆形建筑的矩形洞口与楼板开孔使用实体布尔切割。
		if (Target.bBuildEditor)
		{
			// 仅保存独立静态网格资产时通知编辑器资产注册表。
			PrivateDependencyModuleNames.Add("AssetRegistry");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
