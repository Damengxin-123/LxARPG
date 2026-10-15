#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxSkillArtAssetLibrary.generated.h"

/** 通过官方 MCP 制作技能美术并替换现有流程引用的编辑器工具。 */
UCLASS(meta=(DisplayName="技能美术制作工具"))
class LXARPGEDITOR_API ULxSkillArtAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** 处理制作、资产验证、离屏渲染及技能生命周期验收阶段。 */
	UFUNCTION(BlueprintCallable, Category="技能美术|制作", meta=(DisplayName="处理技能美术"))
	static FString ProcessSkillArt(const FString& Stage);
};
