#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LxInteractionTreeAssetFactory.generated.h"

/** 内容浏览器中的交互树资产创建入口。 */
UCLASS(meta=(DisplayName="交互树资产工厂"))
class LXARPGEDITOR_API ULxInteractionTreeAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	/** 设置支持类型及编辑创建标记。 */
	ULxInteractionTreeAssetFactory();
	/** 创建带默认开始节点的交互树资产。 */
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	/** 默认使用简短中文资产名称。 */
	virtual FString GetDefaultNewAssetName() const override { return TEXT("交互树"); }
};

