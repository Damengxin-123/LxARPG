#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillSpawnEntitySpec.generated.h"

/** 召唤实体、召唤生物、屏障和标记共用的运行时参数。实体类型和默认行为由技能单元Actor类型自身决定。 */
USTRUCT(BlueprintType, DisplayName="技能生成实体参数")
struct FLxSkillSpawnEntitySpec
{
	GENERATED_BODY()

	/** 生成实体所属阵营。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|生成实体", DisplayName="实体阵营")
	ELxSkillEntityFactionType EntityFaction = ELxSkillEntityFactionType::Ally;

	/** 一次释放生成的实体数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|生成实体", DisplayName="实体数量")
	int32 EntityCount = 1;

	/** 多个实体生成时的阵型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|生成实体", DisplayName="生成阵型")
	ELxSkillSpawnFormationType SpawnFormationType = ELxSkillSpawnFormationType::Single;

	/** 实体维持期间每秒对释放者造成的资源消耗，具体资源类型由执行层解释。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|形态|生成实体", DisplayName="每秒维持消耗")
	float MaintainCostPerSecond = 0.0f;
};
