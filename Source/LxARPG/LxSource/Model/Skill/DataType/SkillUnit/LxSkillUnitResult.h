#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillUnitResult.generated.h"

class AActor;

/** 技能单元运行结果，供蓝图串接后续技能单元时读取。 */
USTRUCT(BlueprintType, DisplayName="技能单元运行结果")
struct FLxSkillUnitResult
{
	GENERATED_BODY()

	/** 技能单元是否成功执行，仅供技能系统内部判断。 */
	UPROPERTY(Transient)
	bool bSuccess = false;

	/** 技能单元结果类型，仅供技能系统内部判断。 */
	UPROPERTY(Transient)
	ELxSkillUnitResultType ResultType = ELxSkillUnitResultType::Completed;

	/** 命中的目标列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中目标列表")
	TArray<TObjectPtr<AActor>> HitTargets;

	/** 命中瞬间目标所在的世界位置，与命中目标列表下标一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中目标位置列表")
	TArray<FVector> HitTargetLocations;

	/** 命中瞬间技能子单元所在的世界位置，与命中目标列表下标一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中位置列表")
	TArray<FVector> HitLocations;

	/** 技能单元未命中目标或障碍物而结束时的世界位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="单元失效位置列表")
	TArray<FVector> InvalidLocations;

	/** 命中法线列表，仅供技能系统内部扩展。 */
	UPROPERTY(Transient)
	TArray<FVector> HitNormals;

	/** 从技能单元所在位置朝向对应命中目标位置的规范化方向，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	TArray<FVector> SourceToTargetDirections;

	/** 技能单元结束位置，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	FVector EndLocation = FVector::ZeroVector;

	/** 技能单元结束朝向，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	FRotator EndRotation = FRotator::ZeroRotator;

	/** 技能单元生成的实体列表，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** 已触发次数，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	int32 TriggeredCount = 0;

	/** 输出该结果的技能单元对象，仅供技能系统内部使用。 */
	UPROPERTY(Transient)
	TObjectPtr<UObject> SourceUnit = nullptr;

	/** 自定义结果数据，仅供技能系统内部扩展。 */
	UPROPERTY(Transient)
	TMap<FName, FString> CustomData;
};
