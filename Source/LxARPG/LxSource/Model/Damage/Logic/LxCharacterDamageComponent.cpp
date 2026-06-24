#include "LxCharacterDamageComponent.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxDamageCalculationFlow.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Lifecycle/Logic/LxCharacterLifecycleComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

ULxCharacterDamageComponent::ULxCharacterDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterDamageComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
}

bool ULxCharacterDamageComponent::BuildOutgoingDamagePackage(AActor* TargetActor, FLxEffectPackage& OutDamagePackage)
{
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
	if (DamageCalculationFlow == nullptr || DataTransferComponent == nullptr)
	{
		return false;
	}

	FLxDamageCalculationContext DamageContext;
	DamageContext.SourceActor = GetOwner();
	DamageContext.TargetActor = TargetActor;
	DamageContext.SourceDataTransferComponent = DataTransferComponent;
	if (const ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(TargetActor))
	{
		DamageContext.TargetDataTransferComponent = TargetCharacter->GetCharacterDataTransferComponent();
	}

	DamageContext.OutputEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Skill;
	DamageContext.OutputEffectPackage.SourceContext.SourceActor = GetOwner();
	DamageContext.OutputEffectPackage.SourceContext.SourceObject = this;
	DamageContext.OutputEffectPackage.TargetActor = TargetActor;
	DamageContext.OutputEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;

	DamageContext = DamageCalculationFlow->CalculateOutgoingDamage(DamageContext);
	DamageCalculationFlow->OnDamageCalculationFinished.Broadcast(DamageContext);
	OutDamagePackage = DamageContext.OutputEffectPackage;
	return !OutDamagePackage.DamageEffects.IsEmpty();
}

bool ULxCharacterDamageComponent::ReceiveIncomingDamagePackage(const FLxEffectPackage& InDamagePackage, FLxEffectPackage& OutAppliedPackage, bool bApplyResult)
{
	CacheOwnerComponents();
	EnsureDamageCalculationFlow();
	if (DamageCalculationFlow == nullptr || DataTransferComponent == nullptr)
	{
		return false;
	}

	FLxDamageCalculationContext DamageContext;
	DamageContext.SourceActor = InDamagePackage.SourceContext.SourceActor;
	DamageContext.TargetActor = GetOwner();
	DamageContext.SourceDataTransferComponent = nullptr;
	if (const ALxBaseCharacter* SourceCharacter = Cast<ALxBaseCharacter>(InDamagePackage.SourceContext.SourceActor))
	{
		DamageContext.SourceDataTransferComponent = SourceCharacter->GetCharacterDataTransferComponent();
	}
	DamageContext.TargetDataTransferComponent = DataTransferComponent;
	DamageContext.InputEffectPackage = InDamagePackage;
	DamageContext.OutputEffectPackage.SourceContext = InDamagePackage.SourceContext;
	DamageContext.OutputEffectPackage.TargetActor = GetOwner();
	DamageContext.OutputEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	DamageContext.DamageEffects = InDamagePackage.DamageEffects;
	DamageContext.bApplyResultToTarget = bApplyResult;

	DamageContext = DamageCalculationFlow->CalculateIncomingDamage(DamageContext);
	DamageCalculationFlow->OnDamageCalculationFinished.Broadcast(DamageContext);
	OutAppliedPackage = DamageContext.OutputEffectPackage;
	if (!OutAppliedPackage.IsEmpty())
	{
		OnCharacterDamageReceived.Broadcast(OutAppliedPackage, DamageContext.SourceActor);
	}

	if (bApplyResult && !OutAppliedPackage.IsEmpty())
	{
		RefreshLifecycleAfterDamage();
	}

	return !OutAppliedPackage.IsEmpty();
}

void ULxCharacterDamageComponent::CacheOwnerComponents()
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	DataTransferComponent = OwnerCharacter->GetCharacterDataTransferComponent();
	LifecycleComponent = OwnerCharacter->GetCharacterLifecycleComponent();
}

void ULxCharacterDamageComponent::EnsureDamageCalculationFlow()
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

		DamageCalculationFlow = NewObject<ULxDamageCalculationFlow>(this, FlowClass, TEXT("RuntimeDamageCalculationFlow"));
	}

}

void ULxCharacterDamageComponent::RefreshLifecycleAfterDamage()
{
	if (LifecycleComponent == nullptr || !LifecycleComponent->IsCharacterAlive())
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
