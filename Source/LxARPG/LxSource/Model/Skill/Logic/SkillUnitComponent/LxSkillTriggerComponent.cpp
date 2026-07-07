#include "LxSkillTriggerComponent.h"

#include "GameFramework/Actor.h"

void ULxSkillTriggerComponent::SetTriggerSpec(const FLxSkillTriggerSpec& InTriggerSpec)
{
	TriggerSpec = InTriggerSpec;
	OnDataChange.Broadcast();
}

void ULxSkillTriggerComponent::SetTargetFilterSpec(const FLxSkillTargetFilterSpec& InTargetFilterSpec)
{
	TargetFilterSpec = InTargetFilterSpec;
	OnDataChange.Broadcast();
}

void ULxSkillTriggerComponent::SetHitLimitSpec(const FLxSkillHitLimitSpec& InHitLimitSpec)
{
	HitLimitSpec = InHitLimitSpec;
	OnDataChange.Broadcast();
}

void ULxSkillTriggerComponent::StartTrigger()
{
	TargetHitCounts.Reset();
	TargetLastTriggerTimes.Reset();
	TotalTriggerCount = 0;
	bTriggering = true;
}

void ULxSkillTriggerComponent::StopTrigger()
{
	bTriggering = false;
}

void ULxSkillTriggerComponent::HandleDetectionResult(const FLxSkillDetectionResult& DetectionResult)
{
	RequestTrigger(DetectionResult);
}

void ULxSkillTriggerComponent::HandleLifeTick(float RemainingTime)
{
	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::ManualScan;
	DetectionResult.SourceComponent = this;
	DetectionResult.SourceUnit = GetOwner();
	RequestTrigger(DetectionResult);
}

void ULxSkillTriggerComponent::RequestTrigger(const FLxSkillDetectionResult& DetectionResult)
{
	if (!bTriggering)
	{
		return;
	}

	TArray<AActor*> TriggeredTargets;
	for (AActor* CandidateTarget : DetectionResult.CandidateTargets)
	{
		if (CanTriggerTarget(CandidateTarget))
		{
			TriggeredTargets.Add(CandidateTarget);
		}
	}

	if (TriggeredTargets.Num() == 0 && DetectionResult.bHitWorld)
	{
		BroadcastTriggerResult(DetectionResult, TriggeredTargets);
		return;
	}

	if (TriggeredTargets.Num() > 0)
	{
		BroadcastTriggerResult(DetectionResult, TriggeredTargets);
	}
}

void ULxSkillTriggerComponent::ResetTargetTriggerRecord(AActor* InTarget)
{
	if (!InTarget)
	{
		return;
	}

	const TWeakObjectPtr<AActor> TargetKey(InTarget);
	TargetHitCounts.Remove(TargetKey);
	TargetLastTriggerTimes.Remove(TargetKey);
	OnDataChange.Broadcast();
}

bool ULxSkillTriggerComponent::CanTriggerTarget(AActor* InTarget) const
{
	if (!InTarget || InTarget == GetOwner())
	{
		return false;
	}

	if (HitLimitSpec.MaxTotalHitCount > 0 && TotalTriggerCount >= HitLimitSpec.MaxTotalHitCount)
	{
		return false;
	}

	const TWeakObjectPtr<AActor> TargetKey(InTarget);
	const int32 CurrentHitCount = TargetHitCounts.Contains(TargetKey) ? TargetHitCounts[TargetKey] : 0;
	if (HitLimitSpec.MaxHitCountPerTarget > 0 && CurrentHitCount >= HitLimitSpec.MaxHitCountPerTarget)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (World && HitLimitSpec.HitIntervalPerTarget > 0.0f && TargetLastTriggerTimes.Contains(TargetKey))
	{
		const float LastTime = TargetLastTriggerTimes[TargetKey];
		if (World->GetTimeSeconds() - LastTime < HitLimitSpec.HitIntervalPerTarget)
		{
			return false;
		}
	}

	return true;
}

void ULxSkillTriggerComponent::BroadcastTriggerResult(const FLxSkillDetectionResult& DetectionResult, const TArray<AActor*>& TriggeredTargets)
{
	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

	for (AActor* TriggeredTarget : TriggeredTargets)
	{
		const TWeakObjectPtr<AActor> TargetKey(TriggeredTarget);
		TargetHitCounts.FindOrAdd(TargetKey) += 1;
		TargetLastTriggerTimes.FindOrAdd(TargetKey) = CurrentTime;
	}

	TotalTriggerCount += FMath::Max(TriggeredTargets.Num(), 1);

	FLxSkillTriggerResult Result;
	Result.bTriggered = true;
	Result.DetectionResult = DetectionResult;
	Result.TriggeredCount = TotalTriggerCount;
	for (AActor* TriggeredTarget : TriggeredTargets)
	{
		Result.TriggeredTargets.Add(TriggeredTarget);
	}

	OnTriggered.Broadcast(Result);
	OnDataChange.Broadcast();
}
