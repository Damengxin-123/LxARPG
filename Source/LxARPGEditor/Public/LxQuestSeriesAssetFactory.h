#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "LxQuestSeriesAssetFactory.generated.h"

/** 在内容浏览器中创建任务系列资产的工厂。 */
UCLASS(meta=(DisplayName="任务系列资产工厂"))
class LXARPGEDITOR_API ULxQuestSeriesAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	/** 配置任务系列资产的创建方式和支持类型。 */
	ULxQuestSeriesAssetFactory();

	/** 创建一个新的任务系列资产。 */
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
		UObject* Context, FFeedbackContext* Warn) override;
};
