#include "LxCharacterEffectProcessComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Lifecycle/Logic/LxCharacterLifecycleComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

ULxCharacterEffectProcessComponent::ULxCharacterEffectProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterEffectProcessComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
}

void ULxCharacterEffectProcessComponent::ProcessSkillHitEffects(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets)
{
	CacheOwnerComponents();
	if (SourceSkill == nullptr || DataTransferComponent == nullptr || HitTargets.IsEmpty())
	{
		return;
	}

	TArray<FLxEffectPackage> SourceEffectPackages;
	BuildEffectPackagesFromSkillEntries(SourceSkill, InSkillEntryPackages, SourceEffectPackages);
	if (SourceEffectPackages.IsEmpty())
	{
		return;
	}

	for (AActor* HitTarget : HitTargets)
	{
		if (HitTarget == nullptr)
		{
			continue;
		}

		for (const FLxEffectPackage& SourceEffectPackage : SourceEffectPackages)
		{
			FLxEffectPackage OutgoingEffectPackage;
			if (BuildOutgoingEffectPackage(SourceEffectPackage, HitTarget, OutgoingEffectPackage))
			{
				DataTransferComponent->SendEffectPackageToTarget(OutgoingEffectPackage, HitTarget);
			}
		}
	}
}

bool ULxCharacterEffectProcessComponent::BuildOutgoingEffectPackage(const FLxEffectPackage& InSourceEffectPackage, AActor* TargetActor, FLxEffectPackage& OutEffectPackage)
{
	OutEffectPackage = FLxEffectPackage();
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
	if (DataTransferComponent == nullptr || TargetActor == nullptr)
	{
		return false;
	}

	FLxEffectPackage RuntimeSourceEffectPackage = InSourceEffectPackage;
	if (RuntimeSourceEffectPackage.SourceContext.SourceType == ELxEffectPackageSource::None)
	{
		RuntimeSourceEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
	}

	if (RuntimeSourceEffectPackage.SourceContext.SourceActor == nullptr)
	{
		RuntimeSourceEffectPackage.SourceContext.SourceActor = GetOwner();
	}

	if (RuntimeSourceEffectPackage.SourceContext.SourceObject == nullptr)
	{
		RuntimeSourceEffectPackage.SourceContext.SourceObject = this;
	}

	RuntimeSourceEffectPackage.TargetActor = TargetActor;
	RuntimeSourceEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	OutEffectPackage = RuntimeSourceEffectPackage;

	if (RuntimeSourceEffectPackage.DamageEffects.IsEmpty())
	{
		return !OutEffectPackage.IsEmpty() || OutEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource;
	}

	if (DamageCalculationFlow == nullptr)
	{
		OutEffectPackage.DamageEffects.Reset();
		return !OutEffectPackage.IsEmpty() || OutEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource;
	}

	FLxDamageCalculationContext DamageContext;
	DamageContext.SourceActor = RuntimeSourceEffectPackage.SourceContext.SourceActor;
	DamageContext.TargetActor = TargetActor;
	DamageContext.SourceDataTransferComponent = DataTransferComponent;
	if (const ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(TargetActor))
	{
		DamageContext.TargetDataTransferComponent = TargetCharacter->GetCharacterDataTransferComponent();
	}

	DamageContext.InputEffectPackage = RuntimeSourceEffectPackage;
	DamageContext.OutputEffectPackage = RuntimeSourceEffectPackage;
	DamageContext.DamageEffects = RuntimeSourceEffectPackage.DamageEffects;

	DamageContext = DamageCalculationFlow->CalculateOutgoingDamage(DamageContext);
	DamageCalculationFlow->OnDamageCalculationFinished.Broadcast(DamageContext);
	OutEffectPackage = DamageContext.OutputEffectPackage;
	OutEffectPackage.DamageEffects = DamageContext.DamageEffects;
	return !OutEffectPackage.IsEmpty() || OutEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource;
}

bool ULxCharacterEffectProcessComponent::ReceiveIncomingEffectPackage(const FLxEffectPackage& InEffectPackage, FLxDamageReceiveResult& OutDamageReceiveResult, bool bApplyResult)
{
	OutDamageReceiveResult = FLxDamageReceiveResult();
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
	if (DamageCalculationFlow == nullptr || DataTransferComponent == nullptr)
	{
		return false;
	}

	FLxDamageCalculationContext DamageContext;
	DamageContext.SourceActor = InEffectPackage.SourceContext.SourceActor;
	DamageContext.TargetActor = GetOwner();
	DamageContext.SourceDataTransferComponent = nullptr;
	if (const ALxBaseCharacter* SourceCharacter = Cast<ALxBaseCharacter>(InEffectPackage.SourceContext.SourceActor))
	{
		DamageContext.SourceDataTransferComponent = SourceCharacter->GetCharacterDataTransferComponent();
	}
	DamageContext.TargetDataTransferComponent = DataTransferComponent;
	DamageContext.InputEffectPackage = InEffectPackage;
	DamageContext.DamageEffects = InEffectPackage.DamageEffects;

	DamageContext = DamageCalculationFlow->CalculateIncomingDamage(DamageContext);
	DamageCalculationFlow->OnDamageCalculationFinished.Broadcast(DamageContext);

	OutDamageReceiveResult.ShieldDamageValue = DamageContext.ShieldDamageValue;
	OutDamageReceiveResult.HealthDamageValue = DamageContext.HealthDamageValue;
	OutDamageReceiveResult.bCriticalHit = DamageContext.bCriticalHit;
	OutDamageReceiveResult.bIgnoredDamage = DamageContext.bIgnoredDamage;
	if (OutDamageReceiveResult.IsEmpty())
	{
		return false;
	}

	if (bApplyResult)
	{
		ApplyDamageReceiveResultToTarget(OutDamageReceiveResult);
		RefreshLifecycleAfterDamage();
	}

	OnCharacterDamageReceived.Broadcast(OutDamageReceiveResult, DamageContext.SourceActor);
	return true;
}

void ULxCharacterEffectProcessComponent::CacheOwnerComponents()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	DataTransferComponent = OwnerCharacter->GetCharacterDataTransferComponent();
	LifecycleComponent = OwnerCharacter->GetCharacterLifecycleComponent();
}

void ULxCharacterEffectProcessComponent::EnsureDamageCalculationFlow()
{
	if (DamageCalculationFlow == nullptr)
	{
		TSubclassOf<ULxDamageCalculationFlow> FlowClass = ULxDamageCalculationFlow::StaticClass();
		if (const ULxGameSettings* GameSettings = GetDefault<ULxGameSettings>())
		{
			if (GameSettings->DamageCalculationFlowClass != nullptr)
			{
				FlowClass = GameSettings->DamageCalculationFlowClass;
			}
		}

		DamageCalculationFlow = NewObject<ULxDamageCalculationFlow>(this, FlowClass, TEXT("RuntimeEffectCalculationFlow"));
	}
}

void ULxCharacterEffectProcessComponent::BuildEffectPackagesFromSkillEntries(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, TArray<FLxEffectPackage>& OutEffectPackages) const
{
	OutEffectPackages.Reset();
	if (SourceSkill == nullptr)
	{
		return;
	}

	const TArray<FLxSkillEntryPackage>& SourceEntryPackages = InSkillEntryPackages.IsEmpty()
		? SourceSkill->GetSkillEntryPackages()
		: InSkillEntryPackages;

	for (const FLxSkillEntryPackage& SkillEntryPackage : SourceEntryPackages)
	{
		if (SkillEntryPackage.IsEmpty())
		{
			continue;
		}

		FLxEffectPackage EntryEffectPackage;
		EntryEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
		EntryEffectPackage.SourceContext.SourceActor = GetOwner();
		EntryEffectPackage.SourceContext.SourceObject = SourceSkill;
		EntryEffectPackage.SourceContext.SourceName = SourceSkill->GetFName();
		EntryEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;

		for (const FLxEntryQuote& EntryQuote : SkillEntryPackage.EntryQuotes)
		{
			ULxEntryObjectBase* EntryObject = ULxEntryObjectBase::CreateEnterObject(SourceSkill, EntryQuote);
			if (EntryObject == nullptr || EntryObject->GetEntryBase() == nullptr)
			{
				continue;
			}

			EntryObject->AppendEffectsToPackage(EntryEffectPackage);
		}

		if (!EntryEffectPackage.IsEmpty())
		{
			OutEffectPackages.Add(EntryEffectPackage);
		}
	}
}

void ULxCharacterEffectProcessComponent::ApplyDamageReceiveResultToTarget(const FLxDamageReceiveResult& InDamageReceiveResult)
{
	if (DataTransferComponent == nullptr || InDamageReceiveResult.IsEmpty())
	{
		return;
	}

	FLxEffectPackage DamageRecoveryPackage;
	DamageRecoveryPackage.SourceContext.SourceType = ELxEffectPackageSource::Other;
	DamageRecoveryPackage.SourceContext.SourceActor = GetOwner();
	DamageRecoveryPackage.SourceContext.SourceObject = this;
	DamageRecoveryPackage.TargetActor = GetOwner();
	DamageRecoveryPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;

	if (InDamageReceiveResult.ShieldDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect ShieldDamageEffect;
		ShieldDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Shield;
		ShieldDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		ShieldDamageEffect.RecoveryValue = -InDamageReceiveResult.ShieldDamageValue;
		DamageRecoveryPackage.AttributeRecoveryEffects.Add(ShieldDamageEffect);
	}

	if (InDamageReceiveResult.HealthDamageValue > 0.f)
	{
		FLxAttributeRecoveryEffect HealthDamageEffect;
		HealthDamageEffect.AttributeIDTag = LxTag_Attribute_Resource_Health;
		HealthDamageEffect.RecoveryOperation = ELxAttributeModifierOperation::AddValue;
		HealthDamageEffect.RecoveryValue = -InDamageReceiveResult.HealthDamageValue;
		DamageRecoveryPackage.AttributeRecoveryEffects.Add(HealthDamageEffect);
	}

	if (!DamageRecoveryPackage.AttributeRecoveryEffects.IsEmpty())
	{
		DataTransferComponent->ApplyEffectPackage(DamageRecoveryPackage);
	}
}

void ULxCharacterEffectProcessComponent::RefreshLifecycleAfterDamage()
{
	if (LifecycleComponent == nullptr || DataTransferComponent == nullptr || !LifecycleComponent->IsCharacterAlive())
	{
		return;
	}

	FLxAttributeData HealthAttributeData;
	if (!DataTransferComponent->QueryCharacterAttributeByIDTag(LxTag_Attribute_Resource_Health, HealthAttributeData))
	{
		return;
	}

	if (HealthAttributeData.CalculatedAttributeValue.Value <= 0.f)
	{
		LifecycleComponent->SetCharacterDead();
	}
}
