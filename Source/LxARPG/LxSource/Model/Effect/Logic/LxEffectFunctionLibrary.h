#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxEffectFunctionLibrary.generated.h"

/** 效果工具函数库，提供通用效果汇总和效果包构造辅助。 */
UCLASS(DisplayName="效果工具函数库")
class LXARPG_API ULxEffectFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 汇总属性增益减益效果，相同属性、目标、方式和条件的效果会合并为一条。 */
	UFUNCTION(BlueprintCallable, Category="效果|汇总", DisplayName="汇总属性增益减益效果")
	static void AggregateAttributeModifierEffects(const TArray<FLxAttributeModifierEffect>& InEffects, TArray<FLxAttributeModifierEffect>& OutEffects);

	/** 将一条属性增益减益效果添加到列表中，并按汇总键自动合并。 */
	UFUNCTION(BlueprintCallable, Category="效果|汇总", DisplayName="添加并汇总属性增益减益效果")
	static void AddAggregatedAttributeModifierEffect(UPARAM(ref) TArray<FLxAttributeModifierEffect>& InOutEffects, const FLxAttributeModifierEffect& InEffect);

	/** 创建一个空的模块效果包。 */
	UFUNCTION(BlueprintPure, Category="效果包", DisplayName="创建空效果包")
	static FLxEffectPackage MakeEmptyEffectPackage(FLxEffectSourceContext SourceContext, AActor* TargetActor, ELxEffectPackageApplyPolicy ApplyPolicy);
};
