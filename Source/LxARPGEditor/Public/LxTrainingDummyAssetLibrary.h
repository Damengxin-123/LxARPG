#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxTrainingDummyAssetLibrary.generated.h"

/** 木桩资产制作工具，通过虚幻官方 MCP 保存可编辑的中文蓝图、材质和测试关卡。 */
UCLASS(meta=(DisplayName="木桩资产工具"))
class LXARPGEDITOR_API ULxTrainingDummyAssetLibrary : public UBlueprintFunctionLibrary
{

	GENERATED_BODY()

public:
	/** 按阶段制作、摆放、验证或预览木桩；已有资产和已摆放实例保留手工调整。 */
	UFUNCTION(BlueprintCallable, Category="木桩|制作", meta=(DisplayName="处理木桩资产"))
	static FString ProcessTrainingDummy(const FString& Stage);
};
