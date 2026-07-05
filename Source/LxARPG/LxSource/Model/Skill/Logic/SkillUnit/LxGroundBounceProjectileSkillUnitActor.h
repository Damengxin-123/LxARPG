#pragma once

#include "CoreMinimal.h"
#include "LxProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillGroundBounceProjectileSpec.h"
#include "LxGroundBounceProjectileSkillUnitActor.generated.h"

/** 地面弹跳投射物技能单元，使用重力飞行并在有效地面碰撞时消耗次数弹起。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="地面弹跳投射物技能单元")
class LXARPG_API ALxGroundBounceProjectileSkillUnitActor : public ALxProjectileSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxGroundBounceProjectileSkillUnitActor();

	/** 初始化地面弹跳投射物专用参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|投射物|地面弹跳", DisplayName="初始化地面弹跳投射物参数")
	void InitializeGroundBounceParameters(const FLxSkillGroundBounceProjectileSpec& InGroundBounceSpec);

protected:
	virtual void ResetProjectileRuntimeState() override;
	virtual void HandleProjectileWorldHit(const FLxSkillDetectionResult& DetectionResult) override;

	/** 地面弹跳发生时在投射物内部调用，仅供该类型的蓝图播放动画、特效和声音。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能单元|投射物|地面弹跳|内部事件", DisplayName="地面弹跳时")
	void ReceiveGroundBounce(const FVector& HitLocation, const FVector& HitNormal, int32 InRemainingGroundBounceCount);

	/** 地面弹跳投射物专用参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|地面弹跳", DisplayName="地面弹跳参数")
	FLxSkillGroundBounceProjectileSpec GroundBounceSpec;

	/** 当前运行期剩余的地面弹跳次数。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="技能单元|投射物|地面弹跳", DisplayName="剩余地面弹跳次数")
	int32 RemainingGroundBounceCount = 0;

private:
	/** 判断本次场景碰撞是否属于能够触发弹跳的地面。 */
	bool IsBounceableGroundHit(const FLxSkillDetectionResult& DetectionResult) const;
};
