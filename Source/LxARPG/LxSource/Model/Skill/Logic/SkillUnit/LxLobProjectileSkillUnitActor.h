#pragma once

#include "CoreMinimal.h"
#include "LxProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillLobProjectileSpec.h"
#include "LxLobProjectileSkillUnitActor.generated.h"

/** 抛射投射物技能单元，激活时获得向前和向上的初始速度，随后受重力影响沿抛物线飞行。 */
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
	virtual void ActivateSkillUnit_Implementation() override;

	/** 抛射投射物专用参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|投射物|抛射", DisplayName="抛射参数")
	FLxSkillLobProjectileSpec LobSpec;
};
