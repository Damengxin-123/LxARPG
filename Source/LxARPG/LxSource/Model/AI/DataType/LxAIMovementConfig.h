#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxAIMovementConfig.generated.h"

/** 行为树共用运动能力；倍率在角色基础移动速度及属性加成之后计算。 */
USTRUCT(BlueprintType, meta=(DisplayName="运动能力"))
struct LXARPG_API FLxAIMovementConfig
{
	GENERATED_BODY()

	/** 低速相对属性加成后速度的倍率，0.5表示50%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动能力", meta=(DisplayName="低速移动倍率", ClampMin="0.0"))
	float LowSpeedMultiplier = 0.5f;

	/** 中速相对属性加成后速度的倍率，1.0表示100%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动能力", meta=(DisplayName="中速移动倍率", ClampMin="0.0"))
	float MediumSpeedMultiplier = 1.0f;

	/** 高速相对属性加成后速度的倍率，1.5表示150%。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动能力", meta=(DisplayName="高速移动倍率", ClampMin="0.0"))
	float HighSpeedMultiplier = 1.5f;

	/** 非移动动作接近目标或调整距离时使用中速倍率，不根据攻击类型禁止寻路。 */
	float GetSpeedMultiplier(ELxCharacterMotionType MotionType) const
	{
		const float Value = MotionType == ELxCharacterMotionType::Move ? LowSpeedMultiplier
			: MotionType == ELxCharacterMotionType::Run ? HighSpeedMultiplier : MediumSpeedMultiplier;
		return FMath::IsFinite(Value) ? FMath::Max(0.0f, Value) : 1.0f;
	}

	/** 检查三档倍率为非负有限值且按低、中、高递增，允许相等。 */
	bool ValidateConfiguration(FText& OutError) const
	{
		OutError = FText();
		if (!FMath::IsFinite(LowSpeedMultiplier) || !FMath::IsFinite(MediumSpeedMultiplier)
			|| !FMath::IsFinite(HighSpeedMultiplier) || LowSpeedMultiplier < 0.0f
			|| MediumSpeedMultiplier < LowSpeedMultiplier || HighSpeedMultiplier < MediumSpeedMultiplier)
		{
			OutError = NSLOCTEXT("AI运动能力", "倍率无效", "运动倍率必须为非负有限值，且低速 ≤ 中速 ≤ 高速。");
			return false;
		}
		return true;
	}
};
