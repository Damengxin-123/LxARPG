#pragma once

#include "CoreMinimal.h"
#include "LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAuraSpec.h"
#include "LxAuraSkillUnitActor.generated.h"

/** 光环技能单元类型，是依附在拥有者上的特殊持续范围技能单元。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="光环技能单元")
class LXARPG_API ALxAuraSkillUnitActor : public ALxDurationAreaSkillUnitActor
{
	GENERATED_BODY()

public:
	/** 初始化光环参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|光环", DisplayName="初始化光环参数")
	void InitializeAuraParameters(const FLxSkillAuraSpec& InAuraSpec);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|光环", DisplayName="光环参数")
	FLxSkillAuraSpec AuraSpec;
};
