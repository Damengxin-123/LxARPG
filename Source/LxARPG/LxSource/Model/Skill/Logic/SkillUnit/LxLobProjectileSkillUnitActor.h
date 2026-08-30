#pragma once

#include "CoreMinimal.h"
#include "LxProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillLobProjectileSpec.h"
#include "LxLobProjectileSkillUnitActor.generated.h"

/** 抛射投射物技能单元，激活时同时叠加完整技能发射方向速度和向上抛射速度。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="抛射投射物技能单元")
class LXARPG_API ALxLobProjectileSkillUnitActor : public ALxProjectileSkillUnitActor
{
	GENERATED_BODY()

public:
	ALxLobProjectileSkillUnitActor();

	/** 初始化抛射投射物专用参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|投射物|抛射", DisplayName="初始化抛射投射物参数")
	void InitializeLobParameters(const FLxSkillLobProjectileSpec& InLobSpec);

protected:
	/** 将蓝图选出的主要碰撞体设为扫掠根组件并配置场景阻挡。 */
	virtual void ConfigureProjectilePrimaryCollision() override;

	virtual void ActivateSkillUnit_Implementation() override;

	/** 抛射投射物专用参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|抛射", DisplayName="抛射参数")
	FLxSkillLobProjectileSpec LobSpec;
};
