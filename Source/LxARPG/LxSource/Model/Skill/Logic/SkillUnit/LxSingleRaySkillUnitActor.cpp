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

	FLxSkillUnitResult UnitResult = MakeSkillUnitResult(ResultType,
		ResultType == ELxSkillUnitResultType::Hit || ResultType == ELxSkillUnitResultType::Blocked);
	if (ResultType == ELxSkillUnitResultType::Blocked)
	{
		// 射线命中障碍物时，将障碍表面的碰撞点作为统一命中位置上报。
		UnitResult.HitLocations.Add(DetectionResult.HitLocation);
		UnitResult.HitNormals.Add(DetectionResult.HitNormal);
	}
	for (AActor* HitTarget : DetectionResult.CandidateTargets)
	{
		if (!IsValid(HitTarget))
		{
			continue;
		}
		const FVector TargetLocation = HitTarget->GetActorLocation();
		UnitResult.HitTargets.Add(HitTarget);
		UnitResult.HitTargetLocations.Add(TargetLocation);
		UnitResult.HitLocations.Add(TargetLocation);
		UnitResult.HitNormals.Add((GetActorLocation() - TargetLocation).GetSafeNormal());
		UnitResult.SourceToTargetDirections.Add((TargetLocation - GetActorLocation()).GetSafeNormal());
	}
	// PerformRayDetection 已通过射线基类发布命中，这里只保留结果用于结束单次射线。
	FinishSkillUnit(UnitResult);

	// 单次射线的命中判定立即完成，但必须保留一小段时间，确保蓝图表现能够渲染并复制到客户端。
	SetLifeSpan(FMath::Max(SingleRaySpec.VisualRetentionDuration, 0.01f));
}
