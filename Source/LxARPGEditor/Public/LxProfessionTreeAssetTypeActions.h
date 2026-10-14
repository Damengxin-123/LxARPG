#pragma once
#include "AssetTypeActions_Base.h"

/** 注册职业关系树资产类型及其专用编辑器。 */
class FLxProfessionTreeAssetTypeActions : public FAssetTypeActions_Base
{
public:
	/** 返回中文类型名称。 */
	virtual FText GetName() const override { return FText::FromString(TEXT("职业关系树")); }
	/** 返回类型识别颜色。 */
	virtual FColor GetTypeColor() const override { return FColor(40, 165, 150); }
	/** 返回支持的静态资产类型。 */
	virtual UClass* GetSupportedClass() const override;
	/** 放入玩法资产分类。 */
	virtual uint32 GetCategories() override { return EAssetTypeCategories::Gameplay; }
	/** 打开包含列表、画布和细节配置的专用编辑器。 */
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> Host = nullptr) override;
};
