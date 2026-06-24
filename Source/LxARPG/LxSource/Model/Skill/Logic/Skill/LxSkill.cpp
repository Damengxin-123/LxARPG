#include "LxSkill.h"

#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

void ULxSkill::InitializeSkill_Implementation(const FLxSkillCastContext& InCastContext)
{
	CurrentCastContext = InCastContext;
	if (!CurrentCastContext.WorldContextObject && CurrentCastContext.CasterActor)
	{
		CurrentCastContext.WorldContextObject = CurrentCastContext.CasterActor;
	}
}

void ULxSkill::StartSkillCharge_Implementation()
{
	if (!CanSkillCharge())
	{
		return;
	}

	bCharging = true;
}

void ULxSkill::EndSkillCharge_Implementation()
{
	if (!bCharging)
	{
		return;
	}

	bCharging = false;
}

void ULxSkill::ReleaseSkillDirectly_Implementation()
{
}

void ULxSkill::ReceiveSkillEffectForTarget_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, AActor* HitTarget)
{
	if (!HitTarget)
	{
		return;
	}

	ULxCharacterDataTransferComponent* TargetDataTransferComponent = HitTarget->FindComponentByClass<ULxCharacterDataTransferComponent>();
	if (!TargetDataTransferComponent)
	{
		return;
	}

	const TArray<FLxEffectPackage> RuntimeEffectPackages = BuildRuntimeSkillEffectPackages(InSkillEntryPackages);
	if (RuntimeEffectPackages.IsEmpty())
	{
		return;
	}

	for (const FLxEffectPackage& SkillEffectPackage : RuntimeEffectPackages)
	{
		FLxEffectPackage RuntimeSkillEffectPackage = SkillEffectPackage;
		RuntimeSkillEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
		RuntimeSkillEffectPackage.SourceContext.SourceActor = CurrentCastContext.CasterActor;
		RuntimeSkillEffectPackage.SourceContext.SourceObject = this;
		RuntimeSkillEffectPackage.TargetActor = HitTarget;

		if (RuntimeSkillEffectPackage.SourceContext.SourceName.IsNone())
		{
			RuntimeSkillEffectPackage.SourceContext.SourceName = GetFName();
		}

		TargetDataTransferComponent->ReceiveEffectPackage(RuntimeSkillEffectPackage);
	}
}

void ULxSkill::ReceiveSkillEffectForTargets_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	for (AActor* HitTarget : HitTargets)
	{
		ReceiveSkillEffectForTarget(InSkillEntryPackages, HitTarget);
	}
}

TArray<FLxEffectPackage> ULxSkill::BuildRuntimeSkillEffectPackages(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages) const
{
	const TArray<FLxSkillEntryPackage>& SourceEntryPackages = InSkillEntryPackages.IsEmpty() ? SkillEntryPackages : InSkillEntryPackages;
	TArray<FLxEffectPackage> RuntimeEffectPackages;

	for (const FLxSkillEntryPackage& SkillEntryPackage : SourceEntryPackages)
	{
		if (SkillEntryPackage.IsEmpty())
		{
			continue;
		}

		FLxEffectPackage EntryEffectPackage;
		EntryEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
		EntryEffectPackage.SourceContext.SourceActor = CurrentCastContext.CasterActor;
		EntryEffectPackage.SourceContext.SourceObject = const_cast<ULxSkill*>(this);
		EntryEffectPackage.SourceContext.SourceName = GetFName();
		EntryEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;

		for (const FLxEntryQuote& EntryQuote : SkillEntryPackage.EntryQuotes)
		{
			ULxEntryObjectBase* EntryObject = ULxEntryObjectBase::CreateEnterObject(const_cast<ULxSkill*>(this), EntryQuote);
			if (EntryObject == nullptr || EntryObject->GetEntryBase() == nullptr)
			{
				continue;
			}

			EntryObject->AppendEffectsToPackage(EntryEffectPackage);
		}

		if (!EntryEffectPackage.IsEmpty())
		{
			RuntimeEffectPackages.Add(EntryEffectPackage);
		}
	}

	return RuntimeEffectPackages;
}

float ULxSkill::GetEffectiveReleaseCooldown() const
{
	return FMath::Max(0.2f, ReleaseCooldown);
}

bool ULxSkill::IsReleaseCooldownReady() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	return World->GetTimeSeconds() - LastReleaseTime >= GetEffectiveReleaseCooldown();
}

void ULxSkill::MarkSkillReleased()
{
	if (const UWorld* World = GetWorld())
	{
		LastReleaseTime = World->GetTimeSeconds();
	}
}

void ULxSkill::AddSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!InSkillUnit)
	{
		return;
	}

	SkillUnitList.AddUnique(InSkillUnit);
}

void ULxSkill::RemoveSkillUnit(ALxSkillUnitActor* InSkillUnit)
{
	if (!InSkillUnit)
	{
		return;
	}

	SkillUnitList.Remove(InSkillUnit);
}

void ULxSkill::ClearSkillUnits()
{
	SkillUnitList.Reset();
}

TArray<ALxSkillUnitActor*> ULxSkill::GetSkillUnits() const
{
	TArray<ALxSkillUnitActor*> Result;
	for (ALxSkillUnitActor* SkillUnit : SkillUnitList)
	{
		if (SkillUnit)
		{
			Result.Add(SkillUnit);
		}
	}
	return Result;
}

UWorld* ULxSkill::GetWorld() const
{
	if (CurrentCastContext.WorldContextObject)
	{
		return CurrentCastContext.WorldContextObject->GetWorld();
	}

	if (CurrentCastContext.CasterActor)
	{
		return CurrentCastContext.CasterActor->GetWorld();
	}

	return nullptr;
}
