#include "LxSkillUnitCreateAsyncAction.h"

#include "LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateStraightProjectileUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxStraightProjectileSkillUnitActor> InSkillUnitClass,
	const FLxProjectileSkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::StraightProjectile);
	if (Action)
	{
		Action->StraightProjectileClass = InSkillUnitClass;
		Action->ProjectileCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateScalingAreaEffectsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxScalingAreaSkillUnitActor> InSkillUnitClass,
	const FLxScalingAreaEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::ScalingAreaEffect);
	if (Action)
	{
		Action->ScalingAreaEffectClass = InSkillUnitClass;
		Action->ScalingAreaCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

void ULxSkillUnitCreateAsyncAction::Activate()
{
	if (!IsValid(Skill))
	{
		FinishAsCreateFailed();
		return;
	}

	switch (CreateType)
	{
	case EAsyncCreateType::StraightProjectile:
		SkillUnitGroup = Skill->CreateStraightProjectileUnits(SourceResult, StraightProjectileClass,
			ProjectileCreateParams, TargetFilterSpec, HitLimitSpec, SpawnLocationType, false);
		break;
	case EAsyncCreateType::ScalingAreaEffect:
		SkillUnitGroup = Skill->CreateScalingAreaEffects(SourceResult, ScalingAreaEffectClass,
			ScalingAreaCreateParams, TargetFilterSpec, HitLimitSpec, SpawnLocationType, false);
		break;
	default:
		break;
	}

	if (!IsValid(SkillUnitGroup))
	{
		FinishAsCreateFailed();
		return;
	}

	SkillUnitGroup->OnSkillUnitGroupHit.AddUniqueDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupHit);
	SkillUnitGroup->OnSkillUnitGroupFinished.AddUniqueDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupFinished);

	// 必须在异步输出委托和技能单元组委托全部绑定完成后再激活，避免丢失立即命中事件。
	SkillUnitGroup->ActivateSkillUnits();
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateAction(ULxSkill* InSkill,
	const FLxSkillUnitResult& InSourceResult, const FLxSkillTargetFilterSpec& InTargetFilterSpec,
	const FLxSkillHitLimitSpec& InHitLimitSpec, EAsyncCreateType InCreateType)
{
	ULxSkillUnitCreateAsyncAction* Action = NewObject<ULxSkillUnitCreateAsyncAction>();
	if (!Action)
	{
		return nullptr;
	}

	Action->Skill = InSkill;
	Action->SourceResult = InSourceResult;
	Action->TargetFilterSpec = InTargetFilterSpec;
	Action->HitLimitSpec = InHitLimitSpec;
	Action->CreateType = InCreateType;
	if (IsValid(InSkill))
	{
		Action->RegisterWithGameInstance(InSkill);
	}
	return Action;
}

void ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupHit(ULxSkillUnitGroup* InSkillUnitGroup,
	const FLxSkillUnitResult& InSkillUnitResult)
{
	if (InSkillUnitGroup != SkillUnitGroup)
	{
		return;
	}

	FLxSkillUnitResult BlueprintResult;
	BlueprintResult.bSuccess = !InSkillUnitResult.HitTargets.IsEmpty() || !InSkillUnitResult.HitLocations.IsEmpty();
	BlueprintResult.ResultType = ELxSkillUnitResultType::Hit;
	BlueprintResult.SourceUnit = InSkillUnitResult.SourceUnit;
	BlueprintResult.HitTargets = InSkillUnitResult.HitTargets;
	BlueprintResult.HitTargetLocations = InSkillUnitResult.HitTargetLocations;
	BlueprintResult.HitLocations = InSkillUnitResult.HitLocations;
	OnHitTarget.Broadcast(BlueprintResult);
}

void ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupFinished(ULxSkillUnitGroup* InSkillUnitGroup,
	const FLxSkillUnitResult& InSkillUnitResult)
{
	if (InSkillUnitGroup != SkillUnitGroup)
	{
		return;
	}
	UnbindSkillUnitGroupEvents();
	const bool bGroupEverHit = !InSkillUnitResult.HitTargets.IsEmpty()
		|| !InSkillUnitResult.HitLocations.IsEmpty();
	if (!bGroupEverHit)
	{
		OnSkillInvalidated.Broadcast(InSkillUnitResult);
	}
	SetReadyToDestroy();
}

void ULxSkillUnitCreateAsyncAction::UnbindSkillUnitGroupEvents()
{
	if (!IsValid(SkillUnitGroup))
	{
		SkillUnitGroup = nullptr;
		return;
	}

	SkillUnitGroup->OnSkillUnitGroupHit.RemoveDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupHit);
	SkillUnitGroup->OnSkillUnitGroupFinished.RemoveDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupFinished);
	SkillUnitGroup = nullptr;
}

void ULxSkillUnitCreateAsyncAction::FinishAsCreateFailed()
{
	const FLxSkillUnitResult EmptyResult;
	OnSkillInvalidated.Broadcast(EmptyResult);
	SetReadyToDestroy();
}
