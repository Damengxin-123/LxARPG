#include "LxContinuousAuraEffectSkillUnitActor.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

TArray<AActor*> ALxContinuousAuraEffectSkillUnitActor::GetActiveAuraTargets() const
{
	TArray<AActor*> Result;
	for (AActor* TargetActor : ActiveAuraTargets)
	{
		if (IsValid(TargetActor))
		{
			Result.Add(TargetActor);
		}
	}
	return Result;
}

void ALxContinuousAuraEffectSkillUnitActor::HandleAuraEffectActivated()
{
	ScanCurrentAuraTargets();
}

void ALxContinuousAuraEffectSkillUnitActor::HandleAuraEffectDeactivated(
	ELxAuraTargetEffectRemoveReason RemoveReason)
{
	const TArray<TObjectPtr<AActor>> TargetsToRemove = ActiveAuraTargets;
	for (AActor* TargetActor : TargetsToRemove)
	{
		RemoveActiveAuraTarget(TargetActor, RemoveReason);
	}
	ActiveAuraTargets.Reset();
}

void ALxContinuousAuraEffectSkillUnitActor::HandleAuraDetectionResult(
	const FLxSkillDetectionResult& DetectionResult)
{
	if (!IsSkillUnitActive())
	{
		return;
	}

	if (DetectionResult.EventType == ELxSkillDetectionEventType::OverlapEnd)
	{
		RemoveActiveAuraTarget(DetectionResult.HitActor, ELxAuraTargetEffectRemoveReason::LeftRange);
		return;
	}

	if (DetectionResult.EventType != ELxSkillDetectionEventType::OverlapBegin
		&& DetectionResult.EventType != ELxSkillDetectionEventType::ManualScan)
	{
		return;
	}

	for (AActor* CandidateTarget : DetectionResult.CandidateTargets)
	{
		AddActiveAuraTarget(CandidateTarget);
	}
}

void ALxContinuousAuraEffectSkillUnitActor::HandleActiveAuraTargetDestroyed(AActor* DestroyedActor)
{
	RemoveActiveAuraTarget(DestroyedActor, ELxAuraTargetEffectRemoveReason::TargetInvalid);
}

void ALxContinuousAuraEffectSkillUnitActor::AddActiveAuraTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || ActiveAuraTargets.Contains(TargetActor))
	{
		return;
	}

	ActiveAuraTargets.Add(TargetActor);
	TargetActor->OnDestroyed.AddUniqueDynamic(this,
		&ALxContinuousAuraEffectSkillUnitActor::HandleActiveAuraTargetDestroyed);
	TriggerAuraTargetHit(TargetActor);
}

void ALxContinuousAuraEffectSkillUnitActor::RemoveActiveAuraTarget(AActor* TargetActor,
	ELxAuraTargetEffectRemoveReason RemoveReason)
{
	if (!TargetActor || !ActiveAuraTargets.Contains(TargetActor))
	{
		return;
	}

	ActiveAuraTargets.Remove(TargetActor);
	TargetActor->OnDestroyed.RemoveDynamic(this,
		&ALxContinuousAuraEffectSkillUnitActor::HandleActiveAuraTargetDestroyed);
	if (TriggerComponent)
	{
		TriggerComponent->ResetTargetTriggerRecord(TargetActor);
	}

	FLxAuraTargetEffectRemoveResult RemoveResult;
	RemoveResult.EffectTarget = TargetActor;
	RemoveResult.RemoveReason = RemoveReason;
	OnAuraTargetEffectRemoved.Broadcast(this, RemoveResult);
}
