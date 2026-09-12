#pragma once

#include "AssetTypeActions_Base.h"

/** 注册任务系列资产的名称、分类和专用编辑器打开方式。 */
class FLxQuestSeriesAssetTypeActions : public FAssetTypeActions_Base
{
public:
	/** 返回内容浏览器显示的资产类型名称。 */
	virtual FText GetName() const override;

	/** 返回内容浏览器使用的资产类型颜色。 */
	virtual FColor GetTypeColor() const override;

	/** 返回任务系列资产类。 */
	virtual UClass* GetSupportedClass() const override;

	/** 把任务系列资产放入游戏玩法分类。 */
	virtual uint32 GetCategories() override;

	/** 使用任务系列专用编辑器打开选中的资产。 */
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects,
		TSharedPtr<IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
};
