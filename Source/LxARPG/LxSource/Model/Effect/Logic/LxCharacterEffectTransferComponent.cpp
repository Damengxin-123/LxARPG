#include "LxCharacterEffectTransferComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEffectTransferComponent::ULxCharacterEffectTransferComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterEffectTransferComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
}

bool ULxCharacterEffectTransferComponent::SendEffectPackageToTarget(const FLxEffectPackage& InEffectPackage, AActor* TargetActor)
{
	if (TargetActor == nullptr)
	{
		return false;
	}

	FLxEffectPackage OutgoingEffectPackage = InEffectPackage;
	if (OutgoingEffectPackage.SourceContext.SourceActor == nullptr)
	{
		OutgoingEffectPackage.SourceContext.SourceActor = GetOwner();
	}

	if (OutgoingEffectPackage.SourceContext.SourceObject == nullptr)
	{
		OutgoingEffectPackage.SourceContext.SourceObject = this;
	}

	OutgoingEffectPackage.TargetActor = TargetActor;
	if (ULxCharacterEffectTransferComponent* TargetEffectTransferComponent = TargetActor->FindComponentByClass<ULxCharacterEffectTransferComponent>())
	{
		return TargetEffectTransferComponent->ReceiveEffectPackage(OutgoingEffectPackage);
	}

	return false;
}

void ULxCharacterEffectTransferComponent::SendEffectPackageToTargets(const FLxEffectPackage& InEffectPackage, const TArray<AActor*>& TargetActors)
{
	for (AActor* TargetActor : TargetActors)
	{
		if (TargetActor == nullptr)
		{
			continue;
		}

		SendEffectPackageToTarget(InEffectPackage, TargetActor);
	}
}

bool ULxCharacterEffectTransferComponent::ReceiveEffectPackage(const FLxEffectPackage& InEffectPackage)
{
	FLxEffectPackage IncomingEffectPackage = InEffectPackage;
	if (IncomingEffectPackage.TargetActor == nullptr)
	{
		IncomingEffectPackage.TargetActor = GetOwner();
	}

	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return false;
	}

	DataTransferComponent = OwnerCharacter->GetCharacterDataTransferComponent();
	EffectProcessComponent = OwnerCharacter->GetCharacterEffectProcessComponent();
	if (DataTransferComponent == nullptr || EffectProcessComponent == nullptr)
	{
		return false;
	}

	if (!IncomingEffectPackage.DamageEffects.IsEmpty())
	{
		FLxDamageReceiveResult DamageReceiveResult;
		EffectProcessComponent->ReceiveIncomingEffectPackage(IncomingEffectPackage, DamageReceiveResult);
		IncomingEffectPackage.DamageEffects.Reset();
	}

	if (!IncomingEffectPackage.IsEmpty()
		|| IncomingEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		DataTransferComponent->ApplyEffectPackage(IncomingEffectPackage);
	}

	return true;
}

void ULxCharacterEffectTransferComponent::CacheOwnerComponents()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	DataTransferComponent = OwnerCharacter->GetCharacterDataTransferComponent();
	EffectProcessComponent = OwnerCharacter->GetCharacterEffectProcessComponent();
}
