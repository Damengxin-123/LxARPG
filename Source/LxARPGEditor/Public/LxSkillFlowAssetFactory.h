#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LxSkillFlowAssetFactory.generated.h"

/** 内容浏览器中的技能流程资产创建入口。 */
UCLASS(meta=(DisplayName="技能流程资产工厂"))
class LXARPGEDITOR_API ULxSkillFlowAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	/** 设置支持类型及编辑创建标记。 */
	ULxSkillFlowAssetFactory();
	/** 创建带默认开始节点的技能流程资产。 */
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* Parent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	/** 默认使用简短中文资产名称。 */
	virtual FString GetDefaultNewAssetName() const override { return TEXT("技能流程"); }
};
