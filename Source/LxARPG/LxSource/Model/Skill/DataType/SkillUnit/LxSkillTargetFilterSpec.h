#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillTargetFilterSpec.generated.h"

/** 技能目标筛选参数，描述哪些对象可以成为技能单元目标。 */
USTRUCT(BlueprintType, DisplayName="技能目标筛选参数")
struct FLxSkillTargetFilterSpec
{
	GENERATED_BODY()

	/** 目标阵营类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="目标阵营")
	ELxSkillTargetFactionType TargetFactionType = ELxSkillTargetFactionType::Enemy;

	/** 目标必须拥有的标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="必须拥有标签")
	FGameplayTagContainer RequiredTags;

	/** 目标不能拥有的标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="禁止拥有标签")
	FGameplayTagContainer BlockedTags;

	/** 是否包含死亡单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="包含死亡单位")
	bool bIncludeDead = false;

	/** 是否包含无敌单位。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="包含无敌单位")
	bool bIncludeInvincible = false;

	/** 是否要求释放点到目标之间视线可达。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="需要视线可达")
	bool bRequireLineOfSight = false;

	/** 最大目标数量，低于等于 0 表示不限制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="最大目标数量")
	int32 MaxTargetCount = 0;

	/** 目标排序方式，用于选择最大目标数量内的优先目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|目标筛选", DisplayName="目标排序方式")
	ELxSkillTargetSortType SortType = ELxSkillTargetSortType::None;
};
