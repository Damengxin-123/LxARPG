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

	/** 技能单元是否成功执行。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="是否成功")
	bool bSuccess = false;

	/** 技能单元结果类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="结果类型")
	ELxSkillUnitResultType ResultType = ELxSkillUnitResultType::Completed;

	/** 命中的目标列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中目标列表")
	TArray<TObjectPtr<AActor>> HitTargets;

	/** 命中位置列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中位置列表")
	TArray<FVector> HitLocations;

	/** 命中法线列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="命中法线列表")
	TArray<FVector> HitNormals;

	/** 技能单元结束位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="结束位置")
	FVector EndLocation = FVector::ZeroVector;

	/** 技能单元结束朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="结束朝向")
	FRotator EndRotation = FRotator::ZeroRotator;

	/** 技能单元生成的实体列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="生成实体列表")
	TArray<TObjectPtr<AActor>> SpawnedActors;

	/** 已触发次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="触发次数")
	int32 TriggeredCount = 0;

	/** 输出该结果的技能单元对象，执行层接入后可填充。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="来源技能单元")
	TObjectPtr<UObject> SourceUnit = nullptr;

	/** 自定义结果数据，给蓝图临时传递少量扩展信息。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|结果", DisplayName="自定义数据")
	TMap<FName, FString> CustomData;
};
