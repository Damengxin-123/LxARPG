#pragma once

#include "Modules/ModuleManager.h"

class IAssetTypeActions;

/** LxARPG编辑器模块，负责注册项目专用资产编辑能力。 */
class FLxARPGEditorModule : public IModuleInterface
{
public:
	/** 注册任务系列等项目专用资产编辑器。 */
	virtual void StartupModule() override;

	/** 注销当前模块注册的编辑器扩展。 */
	virtual void ShutdownModule() override;

private:
 /** 运行前准备所有自动生成网格的委托。 */
 FDelegateHandle GeneratedMeshPrePIEHandle;
 /** 编辑器引擎就绪后注册台地手柄的委托。 */
 FDelegateHandle TerraceVisualizerInitHandle;
 /** 保存世界前提交最终预览参数的委托。 */
 FDelegateHandle GeneratedMeshPreSaveHandle;
 /** 保存结束记录总耗时与结果的委托。 */
 FDelegateHandle GeneratedMeshPostSaveHandle;
 /** 自动类型移动或旋转时安排轻量预览。 */
 FDelegateHandle GeneratedActorMovingHandle;
 /** 样条、形态手柄和普通属性编辑时安排轻量预览。 */
 FDelegateHandle GeneratedPropertyChangedHandle;
	/** 职业关系树资产的内容浏览器入口。 */
	TSharedPtr<IAssetTypeActions> ProfessionTreeAssetTypeActions;

	/** 技能流程资产及其图形编辑器入口。 */
	TSharedPtr<IAssetTypeActions> SkillFlowAssetTypeActions;

	/** 交互树资产的内容浏览器入口。 */
	TSharedPtr<IAssetTypeActions> InteractionTreeAssetTypeActions;

	/** 任务系列资产在内容浏览器中的类型操作。 */
	TSharedPtr<IAssetTypeActions> QuestSeriesAssetTypeActions;

	/** AI行为树资产的内容浏览器入口。 */
	TSharedPtr<IAssetTypeActions> AIBehaviorTreeAssetTypeActions;
};
