#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxCloseCombatTypes.generated.h"

class AActor;
class UAnimMontage;
class ULxSkillUnitGroup;
class UPrimitiveComponent;

/** 角色近身战斗组件当前行为状态。 */
UENUM(BlueprintType, DisplayName="近身战斗状态")
enum class ELxCloseCombatState : uint8
{
	Idle UMETA(DisplayName="空闲"),
	Attacking UMETA(DisplayName="攻击中"),
	Blocking UMETA(DisplayName="格挡中")
};

/** 开始一次近战攻击所需的来源、碰撞与命中限制参数。 */
USTRUCT(BlueprintType, DisplayName="近战攻击请求")
struct LXARPG_API FLxMeleeAttackRequest
{
	GENERATED_BODY()

	/** 发起本次攻击的技能唯一标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|来源", DisplayName="技能ID", meta=(Categories="技能"))
	FGameplayTag SkillIDTag;


	/** 发出开始攻击命令的技能单元管理中间层，可在没有单独近战单元时作为来源标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|来源", DisplayName="来源技能单元中间层")
	TObjectPtr<ULxSkillUnitGroup> SourceSkillUnitGroup = nullptr;

	/** 本次攻击期间负责产生重叠事件的武器碰撞体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|碰撞", DisplayName="武器碰撞体")
	TObjectPtr<UPrimitiveComponent> WeaponCollision = nullptr;

	/** 开始攻击时播放的角色动画；为空时只启动碰撞判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|动画", DisplayName="攻击动画")
	TObjectPtr<UAnimMontage> AttackMontage = nullptr;

	/** 本次攻击最多广播的有效命中次数，0 表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|限制", DisplayName="最大命中次数", meta=(ClampMin="0"))
	int32 MaxHitCount = 0;

	/** 是否允许本次攻击多次命中同一个目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|攻击|限制", DisplayName="允许重复命中相同目标")
	bool bAllowRepeatedHitSameTarget = false;

	/** 判断请求是否具备攻击来源和武器碰撞体。 */
	bool IsValid() const
	{
		return WeaponCollision != nullptr && SourceSkillUnitGroup != nullptr;
	}
};

/** 近战组件确认有效后向外广播的单次攻击命中结果。 */
USTRUCT(BlueprintType, DisplayName="近战攻击命中结果")
struct LXARPG_API FLxMeleeAttackHitResult
{
	GENERATED_BODY()

	/** 本次攻击的技能唯一标签。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|攻击|结果", DisplayName="技能ID")
	FGameplayTag SkillIDTag;


	/** 本次攻击的来源技能单元管理中间层。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|攻击|结果", DisplayName="来源技能单元中间层")
	TObjectPtr<ULxSkillUnitGroup> SourceSkillUnitGroup = nullptr;

	/** 通过当前命中限制检查的有效目标。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|攻击|结果", DisplayName="命中目标")
	TObjectPtr<AActor> HitTarget = nullptr;

	/** 武器与目标产生重叠时的目标碰撞体。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|攻击|结果", DisplayName="命中碰撞体")
	TObjectPtr<UPrimitiveComponent> HitComponent = nullptr;

	/** 本次命中的世界位置。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|攻击|结果", DisplayName="命中位置")
	FVector HitLocation = FVector::ZeroVector;
};

/** 近战攻击结束上下文，预留给后续补充正常完成、打断等结束原因。 */
USTRUCT(BlueprintType, DisplayName="近战攻击结束上下文")
struct LXARPG_API FLxMeleeAttackEndContext
{
	GENERATED_BODY()
};

/** 开始格挡所需的盾牌碰撞与动画参数。 */
USTRUCT(BlueprintType, DisplayName="格挡请求")
struct LXARPG_API FLxBlockRequest
{
	GENERATED_BODY()

	/** 格挡期间负责接收武器重叠事件的盾牌碰撞体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|格挡|碰撞", DisplayName="盾牌碰撞体")
	TObjectPtr<UPrimitiveComponent> ShieldCollision = nullptr;

	/** 开始格挡时播放的角色动画；为空时只启动碰撞判定。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="近身战斗|格挡|动画", DisplayName="格挡动画")
	TObjectPtr<UAnimMontage> BlockMontage = nullptr;

	/** 判断请求是否具备盾牌碰撞体。 */
	bool IsValid() const { return ShieldCollision != nullptr; }
};

/** 盾牌碰撞体接收到武器攻击后产生的格挡结果。 */
USTRUCT(BlueprintType, DisplayName="格挡命中结果")
struct LXARPG_API FLxBlockHitResult
{
	GENERATED_BODY()

	/** 本次格挡是否成功阻止了攻击。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|格挡|结果", DisplayName="格挡成功")
	bool bBlockSucceeded = false;

	/** 被格挡攻击的技能唯一标签。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|格挡|结果", DisplayName="攻击技能ID")
	FGameplayTag AttackSkillIDTag;

	/** 发起攻击的角色或单位。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|格挡|结果", DisplayName="攻击者")
	TObjectPtr<AActor> AttackerActor = nullptr;

	/** 与盾牌发生重叠的武器碰撞体。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|格挡|结果", DisplayName="武器碰撞体")
	TObjectPtr<UPrimitiveComponent> WeaponCollision = nullptr;

	/** 武器与盾牌发生重叠的世界位置。 */
	UPROPERTY(BlueprintReadOnly, Category="近身战斗|格挡|结果", DisplayName="格挡位置")
	FVector HitLocation = FVector::ZeroVector;
};

/** 格挡结束上下文，预留给后续补充主动结束、破防等结束原因。 */
USTRUCT(BlueprintType, DisplayName="格挡结束上下文")
struct LXARPG_API FLxBlockEndContext
{
	GENERATED_BODY()
};
