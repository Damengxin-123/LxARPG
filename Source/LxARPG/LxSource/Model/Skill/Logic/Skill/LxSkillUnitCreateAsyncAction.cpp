#include "LxSkillUnitCreateAsyncAction.h"

#include "LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxGroundBounceProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxLobProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDirectHitAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMeleeSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSingleRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSpawnEntitySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxTriggerSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

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
		Action->SkillUnitClass = InSkillUnitClass;
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
		Action->SkillUnitClass = InSkillUnitClass;
		Action->ScalingAreaCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateGroundBounceProjectileUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxGroundBounceProjectileSkillUnitActor> InSkillUnitClass,
	const FLxGroundBounceProjectileSkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::GroundBounceProjectile);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->GroundBounceCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateLobProjectileUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxLobProjectileSkillUnitActor> InSkillUnitClass,
	const FLxLobProjectileSkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::LobProjectile);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->LobCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateDirectHitAreaEffectsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxDirectHitAreaSkillUnitActor> InSkillUnitClass,
	const FLxDirectHitAreaEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::DirectHitAreaEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->DirectHitAreaCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateDurationAreaEffectsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxDurationAreaSkillUnitActor> InSkillUnitClass,
	const FLxDurationAreaEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::DurationAreaEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->DurationAreaCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateMeleeEffectAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxMeleeSkillUnitActor> InSkillUnitClass, const FLxMeleeSkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::MeleeEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->MeleeCreateParams = InCreateParams;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateSingleRayEffectUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxSingleRaySkillUnitActor> InSkillUnitClass, const FLxSingleRayEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::SingleRayEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->SingleRayCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxContinuousRaySkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateContinuousRayEffectUnitAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxContinuousRaySkillUnitActor> InSkillUnitClass,
	const FLxContinuousRayEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxContinuousRaySkillUnitCreateAsyncAction* Action = Cast<ULxContinuousRaySkillUnitCreateAsyncAction>(
		CreateAction(InSkill, InSourceResult, InTargetFilterSpec, InHitLimitSpec,
			EAsyncCreateType::ContinuousRayEffect, ULxContinuousRaySkillUnitCreateAsyncAction::StaticClass()));
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->ContinuousRayCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateContinuousAttachEffectsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxContinuousAttachEffectSkillUnitActor> InSkillUnitClass,
	const FLxContinuousAttachEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::ContinuousAttachEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->ContinuousAttachCreateParams = InCreateParams;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreatePeriodicAttachEffectsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxPeriodicAttachEffectSkillUnitActor> InSkillUnitClass,
	const FLxPeriodicAttachEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::PeriodicAttachEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->PeriodicAttachCreateParams = InCreateParams;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateContinuousAuraEffectUnitAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxContinuousAuraEffectSkillUnitActor> InSkillUnitClass,
	const FLxContinuousAuraEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	float InAuraRange)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::ContinuousAuraEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->ContinuousAuraCreateParams = InCreateParams;
		Action->AuraRange = InAuraRange;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreatePeriodicAuraEffectUnitAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxPeriodicAuraEffectSkillUnitActor> InSkillUnitClass,
	const FLxPeriodicAuraEffectCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	float InAuraRange)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::PeriodicAuraEffect);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->PeriodicAuraCreateParams = InCreateParams;
		Action->AuraRange = InAuraRange;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateSpawnEntityUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxSpawnEntitySkillUnitActor> InSkillUnitClass,
	const FLxSpawnEntitySkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::SpawnEntity);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->SpawnEntityCreateParams = InCreateParams;
		Action->SpawnLocationType = InSpawnLocationType;
	}
	return Action;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateTriggerUnitsAsync(
	ULxSkill* InSkill, const FLxSkillUnitResult& InSourceResult,
	TSubclassOf<ALxTriggerSkillUnitActor> InSkillUnitClass, const FLxTriggerSkillUnitCreateParams& InCreateParams,
	const FLxSkillTargetFilterSpec& InTargetFilterSpec, const FLxSkillHitLimitSpec& InHitLimitSpec,
	ELxSkillUnitResultSpawnLocationType InSpawnLocationType)
{
	ULxSkillUnitCreateAsyncAction* Action = CreateAction(InSkill, InSourceResult, InTargetFilterSpec,
		InHitLimitSpec, EAsyncCreateType::Trigger);
	if (Action)
	{
		Action->SkillUnitClass = InSkillUnitClass;
		Action->TriggerCreateParams = InCreateParams;
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
		SkillUnitGroup = Skill->CreateStraightProjectileUnits(SourceResult,
			TSubclassOf<ALxStraightProjectileSkillUnitActor>(SkillUnitClass.Get()),
			ProjectileCreateParams, TargetFilterSpec, HitLimitSpec, SpawnLocationType, false);
		break;
	case EAsyncCreateType::GroundBounceProjectile:
		SkillUnitGroup = Skill->CreateGroundBounceProjectileUnits(SourceResult,
			TSubclassOf<ALxGroundBounceProjectileSkillUnitActor>(SkillUnitClass.Get()),
			GroundBounceCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::LobProjectile:
		SkillUnitGroup = Skill->CreateLobProjectileUnits(SourceResult,
			TSubclassOf<ALxLobProjectileSkillUnitActor>(SkillUnitClass.Get()),
			LobCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::DirectHitAreaEffect:
		SkillUnitGroup = Skill->CreateDirectHitAreaEffects(SourceResult,
			TSubclassOf<ALxDirectHitAreaSkillUnitActor>(SkillUnitClass.Get()),
			DirectHitAreaCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::DurationAreaEffect:
		SkillUnitGroup = Skill->CreateDurationAreaEffects(SourceResult,
			TSubclassOf<ALxDurationAreaSkillUnitActor>(SkillUnitClass.Get()),
			DurationAreaCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::ScalingAreaEffect:
		SkillUnitGroup = Skill->CreateScalingAreaEffects(SourceResult,
			TSubclassOf<ALxScalingAreaSkillUnitActor>(SkillUnitClass.Get()),
			ScalingAreaCreateParams, TargetFilterSpec, HitLimitSpec, SpawnLocationType, false);
		break;
	case EAsyncCreateType::MeleeEffect:
		SkillUnitGroup = Skill->CreateMeleeEffect(
			TSubclassOf<ALxMeleeSkillUnitActor>(SkillUnitClass.Get()), MeleeCreateParams, true);
		break;
	case EAsyncCreateType::SingleRayEffect:
		SkillUnitGroup = Skill->CreateSingleRayEffectUnits(SourceResult,
			TSubclassOf<ALxSingleRaySkillUnitActor>(SkillUnitClass.Get()),
			SingleRayCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::ContinuousRayEffect:
		SkillUnitGroup = Skill->CreateContinuousRayEffectUnit(SourceResult,
			TSubclassOf<ALxContinuousRaySkillUnitActor>(SkillUnitClass.Get()),
			ContinuousRayCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::ContinuousAttachEffect:
		SkillUnitGroup = Skill->CreateContinuousAttachEffects(SourceResult,
			TSubclassOf<ALxContinuousAttachEffectSkillUnitActor>(SkillUnitClass.Get()),
			ContinuousAttachCreateParams, false);
		break;
	case EAsyncCreateType::PeriodicAttachEffect:
		SkillUnitGroup = Skill->CreatePeriodicAttachEffects(SourceResult,
			TSubclassOf<ALxPeriodicAttachEffectSkillUnitActor>(SkillUnitClass.Get()),
			PeriodicAttachCreateParams, false);
		break;
	case EAsyncCreateType::ContinuousAuraEffect:
		SkillUnitGroup = Skill->CreateContinuousAuraEffectUnit(
			TSubclassOf<ALxContinuousAuraEffectSkillUnitActor>(SkillUnitClass.Get()),
			ContinuousAuraCreateParams, AuraRange, false);
		break;
	case EAsyncCreateType::PeriodicAuraEffect:
		SkillUnitGroup = Skill->CreatePeriodicAuraEffectUnit(
			TSubclassOf<ALxPeriodicAuraEffectSkillUnitActor>(SkillUnitClass.Get()),
			PeriodicAuraCreateParams, AuraRange, false);
		break;
	case EAsyncCreateType::SpawnEntity:
		SkillUnitGroup = Skill->CreateSpawnEntityUnits(SourceResult,
			TSubclassOf<ALxSpawnEntitySkillUnitActor>(SkillUnitClass.Get()),
			SpawnEntityCreateParams, SpawnLocationType, false);
		break;
	case EAsyncCreateType::Trigger:
		SkillUnitGroup = Skill->CreateTriggerUnits(SourceResult,
			TSubclassOf<ALxTriggerSkillUnitActor>(SkillUnitClass.Get()),
			TriggerCreateParams, SpawnLocationType, false);
		break;
	default:
		break;
	}

	if (!IsValid(SkillUnitGroup))
	{
		FinishAsCreateFailed();
		return;
	}

	ApplyTargetRulesToSkillUnits();

	SkillUnitGroup->OnSkillUnitGroupHit.AddUniqueDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupHit);
	SkillUnitGroup->OnSkillUnitGroupFinished.AddUniqueDynamic(
		this, &ULxSkillUnitCreateAsyncAction::HandleSkillUnitGroupFinished);

	// 必须在异步输出委托和技能单元组委托全部绑定完成后再激活，避免丢失立即命中事件。
	SkillUnitGroup->ActivateSkillUnits();
}

bool ULxSkillUnitCreateAsyncAction::StopContinuousSkillRelease()
{
	bool bStopped = false;
	if (IsValid(Skill) && Skill->IsSustainedReleaseActive())
	{
		if (const ALxBaseCharacter* CasterCharacter = Cast<ALxBaseCharacter>(Skill->GetSkillCasterActor()))
		{
			if (ULxSkillCastModule* SkillCastModule = CasterCharacter->GetSkillCastComponent())
			{
				bStopped = SkillCastModule->StopSustainedRelease(Skill);
			}
		}

		if (Skill->IsSustainedReleaseActive())
		{
			bStopped = Skill->TryStopSustainedRelease() || bStopped;
		}
	}

	// 停止临时持续射线后会触发单元组完成并销毁子单元 Actor，下次释放重新创建。
	if (IsValid(SkillUnitGroup) && SkillUnitGroup->HasActiveSkillUnits())
	{
		SkillUnitGroup->StopSkillUnits();
		bStopped = true;
	}
	return bStopped;
}

bool ULxSkillUnitCreateAsyncAction::UpdateContinuousSkillLocationAndRotation(
	FVector InWorldLocation, FRotator InWorldRotation)
{
	if (!IsValid(SkillUnitGroup))
	{
		return false;
	}

	const FTransform UpdatedTransform(InWorldRotation, InWorldLocation);
	SkillUnitGroup->UpdateSkillUnitsTransform(UpdatedTransform);
	return true;
}

ULxSkillUnitCreateAsyncAction* ULxSkillUnitCreateAsyncAction::CreateAction(ULxSkill* InSkill,
	const FLxSkillUnitResult& InSourceResult, const FLxSkillTargetFilterSpec& InTargetFilterSpec,
	const FLxSkillHitLimitSpec& InHitLimitSpec, EAsyncCreateType InCreateType, UClass* InActionClass)
{
	UClass* ActionClass = InActionClass && InActionClass->IsChildOf(StaticClass())
		? InActionClass : StaticClass();
	ULxSkillUnitCreateAsyncAction* Action = NewObject<ULxSkillUnitCreateAsyncAction>(
		GetTransientPackage(), ActionClass);
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

void ULxSkillUnitCreateAsyncAction::ApplyTargetRulesToSkillUnits() const
{
	if (!IsValid(SkillUnitGroup))
	{
		return;
	}

	for (ALxSkillUnitActor* SkillUnit : SkillUnitGroup->GetSkillUnits())
	{
		if (IsValid(SkillUnit))
		{
			SkillUnit->SetTargetRules(TargetFilterSpec, HitLimitSpec);
		}
	}
}
