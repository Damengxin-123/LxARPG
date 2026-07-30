#include "LxAIGroupSubsystem.h"

#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

void ULxAIGroupSubsystem::RegisterMember(const FName InGroupId, ALxAICharacter* InCharacter)
{
	if (InGroupId.IsNone() || !IsValid(InCharacter))
	{
		return;
	}

	GroupRuntimeData.FindOrAdd(InGroupId).Members.Add(InCharacter);
}

void ULxAIGroupSubsystem::UnregisterMember(const FName InGroupId, ALxAICharacter* InCharacter)
{
	FLxAIGroupRuntimeData* GroupData = GroupRuntimeData.Find(InGroupId);
	if (!GroupData || !IsValid(InCharacter))
	{
		return;
	}

	GroupData->Members.Remove(InCharacter);
	GroupData->MemberActions.Remove(InCharacter);
	if (GroupData->Members.IsEmpty())
	{
		GroupRuntimeData.Remove(InGroupId);
	}
}

void ULxAIGroupSubsystem::ReportSensedTarget(const FName InGroupId, ALxAICharacter* InSourceCharacter,
	AActor* InTargetActor, const bool bInHostileBehavior)
{
	if (InGroupId.IsNone() || !IsValid(InSourceCharacter) || !IsValid(InTargetActor) || InSourceCharacter == InTargetActor)
	{
		return;
	}

	FLxAIGroupRuntimeData& GroupData = GroupRuntimeData.FindOrAdd(InGroupId);
	FLxAISharedPerceptionRecord& Record = GroupData.SharedTargets.FindOrAdd(InTargetActor);
	Record.TargetActor = InTargetActor;
	Record.SourceCharacter = InSourceCharacter;
	Record.LastSensedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	Record.bHostileBehaviorConfirmed |= bInHostileBehavior;
}

void ULxAIGroupSubsystem::GetSharedTargets(const FName InGroupId, const float InMaxAge, TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();
	const FLxAIGroupRuntimeData* GroupData = GroupRuntimeData.Find(InGroupId);
	if (!GroupData)
	{
		return;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	for (const TPair<TWeakObjectPtr<AActor>, FLxAISharedPerceptionRecord>& Pair : GroupData->SharedTargets)
	{
		AActor* TargetActor = Pair.Key.Get();
		if (IsValid(TargetActor) && CurrentTime - Pair.Value.LastSensedTime <= FMath::Max(0.0f, InMaxAge))
		{
			OutTargets.AddUnique(TargetActor);
		}
	}
}

bool ULxAIGroupSubsystem::IsTargetMarkedHostile(const FName InGroupId, const AActor* InTargetActor) const
{
	const FLxAIGroupRuntimeData* GroupData = GroupRuntimeData.Find(InGroupId);
	if (!GroupData || !IsValid(InTargetActor))
	{
		return false;
	}
	const FLxAISharedPerceptionRecord* Record = GroupData->SharedTargets.Find(InTargetActor);
	return Record && Record->bHostileBehaviorConfirmed;
}

void ULxAIGroupSubsystem::GetGroupMembers(const FName InGroupId, TArray<ALxAICharacter*>& OutMembers) const
{
	OutMembers.Reset();
	const FLxAIGroupRuntimeData* GroupData = GroupRuntimeData.Find(InGroupId);
	if (!GroupData)
	{
		return;
	}

	for (const TWeakObjectPtr<ALxAICharacter>& Member : GroupData->Members)
	{
		if (ALxAICharacter* Character = Member.Get(); IsValid(Character))
		{
			OutMembers.Add(Character);
		}
	}
}

void ULxAIGroupSubsystem::SetMemberAction(const FName InGroupId, ALxAICharacter* InCharacter, const ELxAIActionType InActionType)
{
	if (InGroupId.IsNone() || !IsValid(InCharacter))
	{
		return;
	}

	FLxAIGroupRuntimeData& GroupData = GroupRuntimeData.FindOrAdd(InGroupId);
	GroupData.Members.Add(InCharacter);
	GroupData.MemberActions.FindOrAdd(InCharacter) = InActionType;
}

int32 ULxAIGroupSubsystem::GetActionExecutorCount(const FName InGroupId, const ELxAIActionType InActionType) const
{
	const FLxAIGroupRuntimeData* GroupData = GroupRuntimeData.Find(InGroupId);
	if (!GroupData)
	{
		return 0;
	}

	int32 ExecutorCount = 0;
	for (const TPair<TWeakObjectPtr<ALxAICharacter>, ELxAIActionType>& Pair : GroupData->MemberActions)
	{
		if (Pair.Key.IsValid() && Pair.Value == InActionType)
		{
			++ExecutorCount;
		}
	}
	return ExecutorCount;
}
