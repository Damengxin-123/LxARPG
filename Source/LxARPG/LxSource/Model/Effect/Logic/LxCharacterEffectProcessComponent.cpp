#include "LxCharacterEffectProcessComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxTypedAttributeData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectCacheComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterSpecialAttributeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "Perception/AISense_Damage.h"

namespace
{
	/** 根据效果包来源生成持续效果缓存句柄。 */
	FName MakePersistentEffectCacheHandle(const FLxEffectPackage& EffectPackage)
	{
		return ULxCharacterEffectCacheComponent::MakeEffectCacheHandle(EffectPackage.SourceContext);
	}

	/** 构建持续效果中的即时结算部分，属性修饰交给效果缓存组件统一刷新。 */
	FLxEffectPackage MakePersistentEffectImmediatePackage(const FLxEffectPackage& EffectPackage)
	{
		FLxEffectPackage ImmediatePackage = EffectPackage;
		ImmediatePackage.AttributeModifierEffects.Reset();
		return ImmediatePackage;
	}
}

ULxCharacterEffectProcessComponent::ULxCharacterEffectProcessComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterEffectProcessComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
}

void ULxCharacterEffectProcessComponent::ProcessSkillHitEffects(ULxSkill* SourceSkill,
	const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets,
	bool bPersistentEffect, ALxSkillUnitActor* PersistentSourceSkillUnit)
{
	CacheOwnerComponents();
	if (SourceSkill == nullptr || DataTransferComponent == nullptr || HitTargets.IsEmpty())
	{
		return;
	}

	TArray<FLxEffectPackage> SourceEffectPackages;
	BuildEffectPackagesFromSkillEntries(SourceSkill, InSkillEntryPackages, SourceEffectPackages, bPersistentEffect,
		PersistentSourceSkillUnit);
	if (SourceEffectPackages.IsEmpty())
	{
		return;
	}

	for (AActor* HitTarget : HitTargets)
	{
		if (!IsValid(HitTarget))
		{
			continue;
		}

		for (const FLxEffectPackage& SourceEffectPackage : SourceEffectPackages)
		{
			FLxEffectPackage OutgoingEffectPackage;
			if (BuildOutgoingEffectPackage(SourceEffectPackage, HitTarget, OutgoingEffectPackage))
			{
				if (bPersistentEffect)
				{
					if (ULxCharacterEffectCacheComponent* TargetEffectCacheComponent =
						HitTarget->FindComponentByClass<ULxCharacterEffectCacheComponent>())
					{
						const FName EffectCacheHandle = MakePersistentEffectCacheHandle(OutgoingEffectPackage);
						TargetEffectCacheComponent->ApplyOrUpdateCachedEffectPackage(EffectCacheHandle, OutgoingEffectPackage);

						const FLxEffectPackage ImmediatePackage = MakePersistentEffectImmediatePackage(OutgoingEffectPackage);
						if (!ImmediatePackage.IsEmpty())
						{
							DataTransferComponent->SendEffectPackageToTarget(ImmediatePackage, HitTarget);
						}
						continue;
					}
				}

				DataTransferComponent->SendEffectPackageToTarget(OutgoingEffectPackage, HitTarget);
			}
		}
	}
}

void ULxCharacterEffectProcessComponent::RemovePersistentSkillEffects(ALxSkillUnitActor* SourceSkillUnit,
	const TArray<AActor*>& EffectTargets)
{
	CacheOwnerComponents();
	if (!IsValid(SourceSkillUnit) || !DataTransferComponent)
	{
		return;
	}
	for (AActor* EffectTarget : EffectTargets)
	{
		if (!IsValid(EffectTarget))
		{
			continue;
		}
		FLxEffectPackage RemovalPackage;
		RemovalPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
		RemovalPackage.SourceContext.SourceActor = GetOwner();
		RemovalPackage.SourceContext.SourceObject = SourceSkillUnit;
		RemovalPackage.TargetActor = EffectTarget;
		RemovalPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::ReplaceSameSource;
		if (ULxCharacterEffectCacheComponent* TargetEffectCacheComponent =
			EffectTarget->FindComponentByClass<ULxCharacterEffectCacheComponent>())
		{
			TargetEffectCacheComponent->RemoveCachedEffectPackage(MakePersistentEffectCacheHandle(RemovalPackage));
		}

		DataTransferComponent->SendEffectPackageToTarget(RemovalPackage, EffectTarget);
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

		// 将自定义效果伤害同步到 AI 伤害感知，保证受击角色能立即识别攻击来源。
		if (IsValid(DamageContext.SourceActor) && DamageContext.SourceActor != GetOwner())
		{
			UAISense_Damage::ReportDamageEvent(this, GetOwner(), DamageContext.SourceActor,
				OutDamageReceiveResult.GetTotalDamageValue(), DamageContext.SourceActor->GetActorLocation(),
				GetOwner()->GetActorLocation());
		}
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
	SpecialAttributeComponent = OwnerCharacter->GetCharacterSpecialAttributeComponent();
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

void ULxCharacterEffectProcessComponent::BuildEffectPackagesFromSkillEntries(ULxSkill* SourceSkill,
	const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, TArray<FLxEffectPackage>& OutEffectPackages,
	bool bPersistentEffect, ALxSkillUnitActor* PersistentSourceSkillUnit) const
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
		EntryEffectPackage.SourceContext.SourceIDTag = SourceSkill->GetSkillIDTag();
		EntryEffectPackage.SourceContext.SourceName = SourceSkill->GetFName();
		if (bPersistentEffect && IsValid(PersistentSourceSkillUnit))
		{
			// 持续效果以技能单元实例作为来源，避免同一技能的多个持续单元互相覆盖或误撤回。
			EntryEffectPackage.SourceContext.SourceObject = PersistentSourceSkillUnit;
			EntryEffectPackage.SourceContext.SourceIDTag = FGameplayTag();
			EntryEffectPackage.SourceContext.SourceName = NAME_None;
		}
		EntryEffectPackage.ApplyPolicy = bPersistentEffect
			? ELxEffectPackageApplyPolicy::ReplaceSameSource
			: ELxEffectPackageApplyPolicy::Instant;

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

	if (bPersistentEffect && OutEffectPackages.Num() > 1)
	{
		FLxEffectPackage CombinedPackage = OutEffectPackages[0];
		for (int32 PackageIndex = 1; PackageIndex < OutEffectPackages.Num(); ++PackageIndex)
		{
			const FLxEffectPackage& Package = OutEffectPackages[PackageIndex];
			CombinedPackage.AttributeModifierEffects.Append(Package.AttributeModifierEffects);
			CombinedPackage.AttributeRecoveryEffects.Append(Package.AttributeRecoveryEffects);
			CombinedPackage.DamageEffects.Append(Package.DamageEffects);
			CombinedPackage.StateChangeEffects.Append(Package.StateChangeEffects);
			CombinedPackage.BuffGrantEffects.Append(Package.BuffGrantEffects);
			CombinedPackage.SkillGrantEffects.Append(Package.SkillGrantEffects);
		}
		OutEffectPackages.Reset();
		OutEffectPackages.Add(MoveTemp(CombinedPackage));
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
	if (SpecialAttributeComponent == nullptr || DataTransferComponent == nullptr || !SpecialAttributeComponent->IsCharacterAlive())
	{
		return;
	}

	FLxResourceAttributeData HealthAttributeData;
	if (!DataTransferComponent->QueryResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttributeData))
	{
		return;
	}

	if (HealthAttributeData.Value <= 0.f)
	{
		SpecialAttributeComponent->SetCharacterDead();
	}
}
