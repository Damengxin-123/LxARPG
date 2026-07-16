#pragma once

#include "CoreMinimal.h"
#include "LxCharacterAnimationTypes.h"
#include "LxCharacterAnimationAssetConfig.generated.h"

class UAnimationAsset;

/** 角色动画类型与动画资产的配置项。 */
USTRUCT(BlueprintType, meta=(DisplayName="角色动画资产配置"))
struct LXARPG_API FLxCharacterAnimationAssetConfig
{
	GENERATED_BODY()

	/** 需要匹配的角色动画类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色动画|动画配置", DisplayName="动画类型")
	ELxCharacterMotionType AnimationType = ELxCharacterMotionType::None;

	/** 动画类型对应的动画资产，可配置动画序列或融合空间。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色动画|动画配置", DisplayName="动画资产")
	TObjectPtr<UAnimationAsset> AnimationAsset = nullptr;
};
