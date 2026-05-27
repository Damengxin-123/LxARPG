#pragma once

#include "CoreMinimal.h"
#include "LxSkillCastContext.generated.h"

class AActor;
class AController;

/** 技能释放上下文。用于描述一次技能释放时的世界、释放者、目标和瞄准信息。 */
USTRUCT(BlueprintType, DisplayName="技能释放上下文")
struct FLxSkillCastContext
{
	GENERATED_BODY()

	/** 可提供 World 的对象，通常传入角色、控制器或技能释放组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="世界上下文对象")
	TObjectPtr<UObject> WorldContextObject = nullptr;

	/** 实际释放技能的角色或场景对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="释放者")
	TObjectPtr<AActor> CasterActor = nullptr;

	/** 控制此次释放行为的控制器，玩家和 AI 都可以通过它区分来源。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="释放控制器")
	TObjectPtr<AController> InstigatorController = nullptr;

	/** 此次技能释放锁定或指定的目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="目标")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** 触发此次技能释放的来源对象，例如技能物品、快捷栏或 AI 行为。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="释放来源对象")
	TObjectPtr<UObject> SourceObject = nullptr;

	/** 技能单元默认生成变换，技能蓝图可以直接用它作为创建技能单元的 SpawnTransform。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="默认生成变换")
	FTransform SpawnTransform = FTransform::Identity;

	/** 瞄准位置。只有“具有瞄准位置”为 true 时才表示本次释放明确指定了位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="瞄准位置")
	FVector AimLocation = FVector::ZeroVector;

	/** 是否具有有效瞄准位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="具有瞄准位置")
	bool bHasAimLocation = false;

	/** 瞄准方向。只有“具有瞄准方向”为 true 时才表示本次释放明确指定了方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="瞄准方向")
	FVector AimDirection = FVector::ForwardVector;

	/** 是否具有有效瞄准方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放上下文", DisplayName="具有瞄准方向")
	bool bHasAimDirection = false;
};
