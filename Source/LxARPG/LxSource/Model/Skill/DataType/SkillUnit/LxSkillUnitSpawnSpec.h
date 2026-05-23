#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillUnitSpawnSpec.generated.h"

/** 技能单元创建规则，描述技能单元从哪里生成、朝向哪里以及是否依附对象。 */
USTRUCT(BlueprintType, DisplayName="技能单元创建规则")
struct FLxSkillUnitSpawnSpec
{
	GENERATED_BODY()

	/** 创建位置来源，例如释放者位置、目标位置、命中位置或上一个单元输出。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="创建位置类型")
	ELxSkillUnitSpawnLocationType SpawnLocationType = ELxSkillUnitSpawnLocationType::CasterLocation;

	/** 创建朝向来源，例如释放者朝向、瞄准方向或朝向目标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="创建朝向类型")
	ELxSkillUnitSpawnRotationType SpawnRotationType = ELxSkillUnitSpawnRotationType::CasterForward;

	/** 创建位置偏移，用于在来源位置基础上微调生成点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="位置偏移")
	FVector LocationOffset = FVector::ZeroVector;

	/** 创建朝向偏移，用于在来源朝向基础上微调旋转。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="朝向偏移")
	FRotator RotationOffset = FRotator::ZeroRotator;

	/** 当创建位置或朝向来源为插槽时使用的插槽名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="插槽名称")
	FName SocketName = NAME_None;

	/** 技能单元创建后是否依附到释放者、目标或生成实体上。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|创建", DisplayName="依附目标类型")
	ELxSkillUnitAttachTargetType AttachTargetType = ELxSkillUnitAttachTargetType::None;
};
