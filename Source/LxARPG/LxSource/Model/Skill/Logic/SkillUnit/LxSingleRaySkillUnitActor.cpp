#include "LxSingleRaySkillUnitActor.h"

void ALxSingleRaySkillUnitActor::InitializeSingleRayParameters(const FLxSingleRayEffectSpec& InSingleRaySpec)
{
	SingleRaySpec = InSingleRaySpec;
}

void ALxSingleRaySkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		return;
	}

	Super::ActivateSkillUnit_Implementation();
	if (!HasAuthority() || !IsSkillUnitActive())
	{
		return;
	}

	const FLxSkillDetectionResult DetectionResult = PerformRayDetection();

	ELxSkillUnitResultType ResultType = ELxSkillUnitResultType::Miss;
	if (!DetectionResult.CandidateTargets.IsEmpty())
	{
		ResultType = ELxSkillUnitResultType::Hit;
	}
	else if (DetectionResult.bHitWorld)
	{
		ResultType = ELxSkillUnitResultType::Blocked;
	}

	FLxSkillUnitResult UnitResult = MakeSkillUnitResult(ResultType, ResultType == ELxSkillUnitResultType::Hit);
	for (AActor* HitTarget : DetectionResult.CandidateTargets)
	{
		if (!IsValid(HitTarget))
		{
			continue;
		}
		const FVector TargetLocation = HitTarget->GetActorLocation();
		UnitResult.HitTargets.Add(HitTarget);
		UnitResult.HitLocations.Add(TargetLocation);
		UnitResult.HitNormals.Add((GetActorLocation() - TargetLocation).GetSafeNormal());
		UnitResult.SourceToTargetDirections.Add((TargetLocation - GetActorLocation()).GetSafeNormal());
	}
	if (UnitResult.HitTargets.IsEmpty())
	{
		UnitResult.HitLocations.Add(DetectionResult.HitLocation);
		UnitResult.HitNormals.Add(DetectionResult.HitNormal);
	}
	PublishSkillUnitHitResult(UnitResult);
	FinishSkillUnit(UnitResult);

	// 单次射线的命中判定立即完成，但必须保留一小段时间，确保蓝图表现能够渲染并复制到客户端。
	SetLifeSpan(FMath::Max(SingleRaySpec.VisualRetentionDuration, 0.01f));
}
