#include "LxCharacterEffectTransferComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEffectTransferModule::ULxCharacterEffectTransferModule() = default;

void ULxCharacterEffectTransferModule::OnModuleInitialize()
{
	CacheOwnerComponents();
}

bool ULxCharacterEffectTransferModule::SendEffectPackageToTarget(const FLxEffectPackage& InEffectPackage, AActor* TargetActor)
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
	if (const ULxCharacterEffectComponent* TargetEffectComponent = TargetActor->FindComponentByClass<ULxCharacterEffectComponent>())
	{
		if (ULxCharacterEffectTransferModule* TargetTransferModule = TargetEffectComponent->GetTransferModule())
		{
			return TargetTransferModule->ReceiveEffectPackage(OutgoingEffectPackage);
		}
	}

	return false;
}

void ULxCharacterEffectTransferModule::SendEffectPackageToTargets(const FLxEffectPackage& InEffectPackage, const TArray<AActor*>& TargetActors)
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

bool ULxCharacterEffectTransferModule::ReceiveEffectPackage(const FLxEffectPackage& InEffectPackage)
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
	EffectProcessModule = OwnerCharacter->GetCharacterEffectProcessComponent();
	if (DataTransferComponent == nullptr || EffectProcessModule == nullptr)
	{
		return false;
	}

	if (!IncomingEffectPackage.DamageEffects.IsEmpty())
	{
		FLxDamageReceiveResult DamageReceiveResult;
		EffectProcessModule->ReceiveIncomingEffectPackage(IncomingEffectPackage, DamageReceiveResult);
		IncomingEffectPackage.DamageEffects.Reset();
	}

	if (!IncomingEffectPackage.IsEmpty()
		|| IncomingEffectPackage.ApplyPolicy == ELxEffectPackageApplyPolicy::ReplaceSameSource)
	{
		DataTransferComponent->ApplyEffectPackage(IncomingEffectPackage);
	}

	return true;
}

void ULxCharacterEffectTransferModule::CacheOwnerComponents()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	DataTransferComponent = OwnerCharacter->GetCharacterDataTransferComponent();
	EffectProcessModule = OwnerCharacter->GetCharacterEffectProcessComponent();
}
