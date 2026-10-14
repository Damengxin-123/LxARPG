#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LxProfessionTreeAssetFactory.generated.h"

/** 内容浏览器中的职业关系树资产创建入口。 */
UCLASS(meta=(DisplayName="职业关系树工厂"))
class LXARPGEDITOR_API ULxProfessionTreeAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	/** 设置创建类型并启用创建后编辑。 */
	ULxProfessionTreeAssetFactory();
	/** 创建具有空画布的职业关系资产。 */
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;
	/** 新建资产默认使用简短中文名称。 */
	virtual FString GetDefaultNewAssetName() const override { return TEXT("职业关系"); }
};
