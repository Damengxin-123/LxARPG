#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxCharacterAnimationTypes.generated.h"

/**
 * 角色运动行为类型。
 * 只描述角色当前发生了什么行为，不直接绑定具体动画资源。
 */
UENUM(BlueprintType, meta=(DisplayName="角色运动类型"))
enum class ELxCharacterMotionType : uint8
{
	None UMETA(DisplayName="无"),
	Idle UMETA(DisplayName="闲置"),
	Move UMETA(DisplayName="低速移动"),
	Run UMETA(DisplayName="高速移动"),
	JumpStart UMETA(DisplayName="跳跃"),
	Airborne UMETA(DisplayName="滞空"),
	JumpEnd UMETA(DisplayName="跳跃结束"),
	Attack UMETA(DisplayName="近战攻击"),
	Skill UMETA(DisplayName="技能"),
	Hurt UMETA(DisplayName="受击"),
	Dead UMETA(DisplayName="倒地"),
	// 新类型追加到末尾，保留已有蓝图与动画资产的序列化数值。
	MediumMove UMETA(DisplayName="中速移动"),
	Alert UMETA(DisplayName="警惕"),
	RangedAttack UMETA(DisplayName="远程攻击"),
	Defend UMETA(DisplayName="防御")
};

/**
 * 角色运动信号。
 * 由外部行为组件主动发送给运动分析组件，携带行为类型和可选运动参数。
 */
USTRUCT(BlueprintType, meta=(DisplayName="角色运动信号"))
struct LXARPG_API FLxCharacterMotionSignal
{
	GENERATED_BODY()

	/** 当前动作对应的技能物品ID；无效表示通用动作。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|动作", meta=(DisplayName="技能ID", Categories="物品.技能"))
	FGameplayTag SkillId;

	/** 当前角色运动行为类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|运动信号", DisplayName="运动类型")
	ELxCharacterMotionType MotionType = ELxCharacterMotionType::None;

	/** 当前运动输入方向或行为方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|运动信号", DisplayName="运动方向")
	FVector MotionDirection = FVector::ZeroVector;

	/** 当前运动速度或外部希望动画参考的速度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|运动信号", DisplayName="运动速度")
	float MotionSpeed = 0.0f;

	/** 当前运动信号是否希望保持循环表现。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|运动信号", DisplayName="是否循环")
	bool bLoop = true;
};

/**
 * 角色动画播放信号。
 * 由动画处理组件生成，动画实例只消费该信号并驱动蓝图播放。
 */
USTRUCT(BlueprintType, meta=(DisplayName="角色动画播放信号"))
struct LXARPG_API FLxCharacterAnimationSignal
{
	GENERATED_BODY()

	/** 当前动作对应的技能物品ID；无效表示通用动作。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|动作", meta=(DisplayName="技能ID", Categories="物品.技能"))
	FGameplayTag SkillId;

	/** 当前需要播放的动画类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|动画信号", DisplayName="动画类型")
	ELxCharacterMotionType AnimationType = ELxCharacterMotionType::None;

	/** 当前动画播放速率。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|动画信号", DisplayName="播放速率")
	float PlayRate = 1.0f;

	/** 当前动画是否循环播放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色动画|动画信号", DisplayName="是否循环")
	bool bLoop = true;
};
