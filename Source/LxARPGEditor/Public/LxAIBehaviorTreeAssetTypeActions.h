#pragma once

#include "AssetTypeActions_Base.h"

/** 注册AI控制配置资产菜单与专用编辑器。 */
class FLxAIBehaviorTreeAssetTypeActions : public FAssetTypeActions_Base
{
public:
	/** 内容浏览器资产类型名称。 */
	virtual FText GetName() const override { return FText::FromString(TEXT("AI控制配置")); }
	/** 资产图标颜色。 */
	virtual FColor GetTypeColor() const override { return FColor(45, 130, 205); }
	/** 返回受支持的运行时资产类型。 */
	virtual UClass* GetSupportedClass() const override;
	/** 放入玩法资产分类。 */
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Gameplay; }
	/** 双击资产打开专用编辑器。 */
	virtual void OpenAssetEditor(const TArray<UObject*>& Objects, TSharedPtr<IToolkitHost> Host) override;
};
