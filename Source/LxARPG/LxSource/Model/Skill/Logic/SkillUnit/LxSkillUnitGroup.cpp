#include "LxSkillUnitGroup.h"

#include "LxSkillUnitActor.h"
#include "LxMeleeSkillUnitActor.h"
#include "LxAttachEffectSkillUnitActor.h"
#include "LxContinuousAuraEffectSkillUnitActor.h"

void ULxSkillUnitGroup::InitializeSkillUnitGroup(const TArray<ALxSkillUnitActor*>& InSkillUnits)
{
	ClearSkillUnits();
	ClearAccumulatedHitResult();
	AddSkillUnits(InSkillUnits);
}

bool ULxSkillUnitGroup::AddSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!IsValid(InSkillUnit) || ContainsSkillUnit(InSkillUnit))
	{
		return false;
	}

	ManagedSkillUnits.Add(InSkillUnit);
	BindSkillUnitEvents(InSkillUnit);
	bFinishedBroadcasted = false;
	return true;
}

int32 ULxSkillUnitGroup::AddSkillUnits(const TArray<ALxSkillUnitActor*>& InSkillUnits)
{
	int32 AddedCount = 0;
	for (ALxSkillUnitActor* SkillUnit : InSkillUnits)
	{
		if (AddSkillUnit(SkillUnit))
		{
			++AddedCount;
		}
	}

	return AddedCount;
}

void ULxSkillUnitGroup::ActivateSkillUnits()
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		SkillUnit->ActivateSkillUnit();
	}
}

void ULxSkillUnitGroup::StopSkillUnits()
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		SkillUnit->StopSkillUnit();
	}
}

void ULxSkillUnitGroup::CancelSkillUnits()
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		SkillUnit->CancelSkillUnit();
	}
}

void ULxSkillUnitGroup::UpdateSkillUnitsTransform(const FTransform& InTransform)
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		SkillUnit->UpdateSkillUnitTransform(InTransform);
	}
}

bool ULxSkillUnitGroup::HasActiveSkillUnits() const
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		if (SkillUnit->IsSkillUnitActive())
		{
			return true;
		}
	}
	return false;
}

bool ULxSkillUnitGroup::ReceiveMeleeWeaponHit(const FLxMeleeHitContext& InHitContext)
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		if (ALxMeleeSkillUnitActor* MeleeSkillUnit = Cast<ALxMeleeSkillUnitActor>(SkillUnit))
		{
			if (MeleeSkillUnit->ReceiveWeaponHit(InHitContext))
			{
				return true;
			}
		}
	}
	return false;
}

bool ULxSkillUnitGroup::GetMeleeSpec(FLxSkillMeleeSpec& OutMeleeSpec) const
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		if (const ALxMeleeSkillUnitActor* MeleeSkillUnit = Cast<ALxMeleeSkillUnitActor>(SkillUnit))
		{
			OutMeleeSpec = MeleeSkillUnit->GetMeleeSpec();
			return true;
		}
	}
	OutMeleeSpec = FLxSkillMeleeSpec();
	return false;
}

void ULxSkillUnitGroup::DestroySkillUnits()
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		SkillUnit->Destroy();
	}
}

void ULxSkillUnitGroup::ClearSkillUnits()
{
	for (ALxSkillUnitActor* SkillUnit : GetSkillUnits())
	{
		UnbindSkillUnitEvents(SkillUnit);
	}

	ManagedSkillUnits.Reset();
	DestroyedSkillUnitLocations.Reset();
	bFinishedBroadcasted = false;
}

TArray<ALxSkillUnitActor*> ULxSkillUnitGroup::GetSkillUnits() const
{
	TArray<ALxSkillUnitActor*> ValidSkillUnits;
	for (ALxSkillUnitActor* SkillUnit : ManagedSkillUnits)
	{
		if (IsValid(SkillUnit))
		{
			ValidSkillUnits.Add(SkillUnit);
		}
	}

	return ValidSkillUnits;
}

int32 ULxSkillUnitGroup::GetValidSkillUnitCount() const
{
	return GetSkillUnits().Num();
}

bool ULxSkillUnitGroup::IsSkillUnitGroupEmpty() const
{
	return GetValidSkillUnitCount() <= 0;
}

void ULxSkillUnitGroup::ClearAccumulatedHitResult()
{
	AccumulatedHitResult = FLxSkillUnitResult();
}

void ULxSkillUnitGroup::BeginDestroy()
{
	ClearSkillUnits();
	Super::BeginDestroy();
}

void ULxSkillUnitGroup::MergeSkillUnitHitResult(const FLxSkillUnitResult& InSkillUnitResult)
{
	if (!InSkillUnitResult.bSuccess || InSkillUnitResult.ResultType != ELxSkillUnitResultType::Hit)
	{
		return;
	}

	AccumulatedHitResult.bSuccess = true;
	AccumulatedHitResult.ResultType = ELxSkillUnitResultType::Hit;
	AccumulatedHitResult.SourceUnit = this;
	AccumulatedHitResult.EndLocation = InSkillUnitResult.EndLocation;
	AccumulatedHitResult.EndRotation = InSkillUnitResult.EndRotation;
	AccumulatedHitResult.HitTargets.Append(InSkillUnitResult.HitTargets);
	AccumulatedHitResult.HitLocations.Append(InSkillUnitResult.HitLocations);
	AccumulatedHitResult.HitNormals.Append(InSkillUnitResult.HitNormals);
	AccumulatedHitResult.SourceToTargetDirections.Append(InSkillUnitResult.SourceToTargetDirections);
	AccumulatedHitResult.SpawnedActors.Append(InSkillUnitResult.SpawnedActors);
	AccumulatedHitResult.CustomData.Append(InSkillUnitResult.CustomData);
	AccumulatedHitResult.TriggeredCount = FMath::Max(AccumulatedHitResult.HitTargets.Num(), AccumulatedHitResult.HitLocations.Num());
}

void ULxSkillUnitGroup::BindSkillUnitEvents(ALxSkillUnitActor* InSkillUnit)
{
	if (!IsValid(InSkillUnit))
	{
		return;
	}

	InSkillUnit->OnSkillUnitHit.AddUniqueDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitHit);
	InSkillUnit->OnSkillUnitFinished.AddUniqueDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitFinished);
	InSkillUnit->OnDestroyed.AddUniqueDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitDestroyed);
	if (ALxAttachEffectSkillUnitActor* AttachEffect = Cast<ALxAttachEffectSkillUnitActor>(InSkillUnit))
	{
		AttachEffect->OnAttachEffectEnded.AddUniqueDynamic(this, &ULxSkillUnitGroup::HandleAttachEffectEnded);
	}
	if (ALxContinuousAuraEffectSkillUnitActor* AuraEffect = Cast<ALxContinuousAuraEffectSkillUnitActor>(InSkillUnit))
	{
		AuraEffect->OnAuraTargetEffectRemoved.AddUniqueDynamic(this, &ULxSkillUnitGroup::HandleAuraTargetEffectRemoved);
	}
}

void ULxSkillUnitGroup::UnbindSkillUnitEvents(ALxSkillUnitActor* InSkillUnit)
{
	if (!IsValid(InSkillUnit))
	{
		return;
	}

	InSkillUnit->OnSkillUnitHit.RemoveDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitHit);
	InSkillUnit->OnSkillUnitFinished.RemoveDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitFinished);
	InSkillUnit->OnDestroyed.RemoveDynamic(this, &ULxSkillUnitGroup::HandleManagedSkillUnitDestroyed);
	if (ALxAttachEffectSkillUnitActor* AttachEffect = Cast<ALxAttachEffectSkillUnitActor>(InSkillUnit))
	{
		AttachEffect->OnAttachEffectEnded.RemoveDynamic(this, &ULxSkillUnitGroup::HandleAttachEffectEnded);
	}
	if (ALxContinuousAuraEffectSkillUnitActor* AuraEffect = Cast<ALxContinuousAuraEffectSkillUnitActor>(InSkillUnit))
	{
		AuraEffect->OnAuraTargetEffectRemoved.RemoveDynamic(this, &ULxSkillUnitGroup::HandleAuraTargetEffectRemoved);
	}
}

void ULxSkillUnitGroup::RemoveManagedSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!InSkillUnit)
	{
		return;
	}

	UnbindSkillUnitEvents(InSkillUnit);
	ManagedSkillUnits.RemoveAll([InSkillUnit](const TObjectPtr<ALxSkillUnitActor>& ManagedSkillUnit)
	{
		return ManagedSkillUnit.Get() == InSkillUnit;
	});
	BroadcastFinishedIfEmpty();
}

void ULxSkillUnitGroup::BroadcastFinishedIfEmpty()
{
	ManagedSkillUnits.RemoveAll([](const TObjectPtr<ALxSkillUnitActor>& ManagedSkillUnit)
	{
		return !IsValid(ManagedSkillUnit.Get());
	});

	if (!bFinishedBroadcasted && ManagedSkillUnits.IsEmpty())
	{
		bFinishedBroadcasted = true;
		// 使用快照避免技能对象在首个回调中释放本组并清空成员，导致后续蓝图监听者收到空数组。
		const TArray<FVector> DestroyedLocationsSnapshot = DestroyedSkillUnitLocations;
		OnSkillUnitGroupFinished.Broadcast(this, DestroyedLocationsSnapshot);
	}
}

bool ULxSkillUnitGroup::ContainsSkillUnit(ALxSkillUnitActor* InSkillUnit) const
{
	if (!InSkillUnit)
	{
		return false;
	}

	for (ALxSkillUnitActor* SkillUnit : ManagedSkillUnits)
	{
		if (SkillUnit == InSkillUnit)
		{
			return true;
		}
	}

	return false;
}

void ULxSkillUnitGroup::HandleManagedSkillUnitHit(ALxSkillUnitActor* InSkillUnit, const FLxSkillUnitResult& InSkillUnitResult)
{
	if (!ContainsSkillUnit(InSkillUnit))
	{
		return;
	}

	MergeSkillUnitHitResult(InSkillUnitResult);
	OnSkillUnitGroupHit.Broadcast(this, InSkillUnitResult);
}

void ULxSkillUnitGroup::HandleManagedSkillUnitFinished(ALxSkillUnitActor* InSkillUnit, const FLxSkillUnitResult& InSkillUnitResult)
{
	if (!ContainsSkillUnit(InSkillUnit) || !IsValid(InSkillUnit) || InSkillUnit->IsActorBeingDestroyed()
		|| !bDestroyUnitsWhenFinished)
	{
		return;
	}

	if (!InSkillUnit->Destroy())
	{
		RemoveManagedSkillUnit(InSkillUnit);
	}
}

void ULxSkillUnitGroup::HandleManagedSkillUnitDestroyed(AActor* DestroyedActor)
{
	if (ContainsSkillUnit(Cast<ALxSkillUnitActor>(DestroyedActor)))
	{
		DestroyedSkillUnitLocations.Add(DestroyedActor->GetActorLocation());
	}
	RemoveManagedSkillUnit(Cast<ALxSkillUnitActor>(DestroyedActor));
}

void ULxSkillUnitGroup::HandleAttachEffectEnded(ALxAttachEffectSkillUnitActor* SkillUnit,
	const FLxAttachEffectEndResult& EndResult)
{
	if (!ContainsSkillUnit(SkillUnit) || !EndResult.bEffectApplied || !IsValid(EndResult.AttachTarget))
	{
		return;
	}
	TArray<AActor*> Targets;
	Targets.Add(EndResult.AttachTarget);
	OnSkillUnitGroupEffectsRemoved.Broadcast(this, SkillUnit, Targets);
}

void ULxSkillUnitGroup::HandleAuraTargetEffectRemoved(ALxContinuousAuraEffectSkillUnitActor* SkillUnit,
	const FLxAuraTargetEffectRemoveResult& RemoveResult)
{
	if (!ContainsSkillUnit(SkillUnit) || !IsValid(RemoveResult.EffectTarget))
	{
		return;
	}
	TArray<AActor*> Targets;
	Targets.Add(RemoveResult.EffectTarget);
	OnSkillUnitGroupEffectsRemoved.Broadcast(this, SkillUnit, Targets);
}
