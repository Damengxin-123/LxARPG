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
	/** 任务系列资产在内容浏览器中的类型操作。 */
	TSharedPtr<IAssetTypeActions> QuestSeriesAssetTypeActions;
};
