#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LxAIBehaviorTreeAssetFactory.generated.h"

class ULxAIBehaviorTreeAsset;

/** 内容浏览器中的AI控制配置资产创建入口。 */
UCLASS(meta=(DisplayName="AI控制配置资产工厂"))
class LXARPGEDITOR_API ULxAIBehaviorTreeAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	/** 设置支持类型及编辑创建标记。 */
	ULxAIBehaviorTreeAssetFactory();
	/** 创建包含感知能力与行为模板的AI控制配置资产。 */
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	/** 默认使用简短中文资产名称。 */
	virtual FString GetDefaultNewAssetName() const override { return TEXT("AI控制配置"); }
	/** 仅为空资产建立编辑图和默认模板。 */
	static void InitializeDefaultTree(ULxAIBehaviorTreeAsset* Asset);
};
