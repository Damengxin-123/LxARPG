#include "LxCharacterTestComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxCharacterDamageComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterTestComponent::ULxCharacterTestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterTestComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();

	if (ULxCharacterDamageComponent* DamageComponent = GetDamageComponent())
	{
		DamageComponent->OnCharacterDamageReceived.RemoveDynamic(this, &ULxCharacterTestComponent::HandleCharacterDamageReceived);
		DamageComponent->OnCharacterDamageReceived.AddDynamic(this, &ULxCharacterTestComponent::HandleCharacterDamageReceived);
	}
}

bool ULxCharacterTestComponent::AddTestItemToBackpack(FGameplayTag InItemIDTag, int32 InItemCount)
{
	if (!InItemIDTag.IsValid() || InItemCount <= 0)
	{
		return false;
	}

	TArray<FLxItemQuote> ItemList;
	ItemList.Add(FLxItemQuote(InItemIDTag, InItemCount));
	return AddTestItemListToBackpack(ItemList);
}

bool ULxCharacterTestComponent::AddTestItemListToBackpack(const TArray<FLxItemQuote>& InItemList)
{
	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	return DataTransferComponent != nullptr && DataTransferComponent->AddItemListToBackpack(InItemList);
}

bool ULxCharacterTestComponent::AddTestSkillItemToSkillBackpack(FGameplayTag InSkillItemIDTag)
{
	if (!InSkillItemIDTag.IsValid())
	{
		return false;
	}

	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	return DataTransferComponent != nullptr && DataTransferComponent->AddSkillItemToSkillBackpack(InSkillItemIDTag);
}

bool ULxCharacterTestComponent::CanLearnTestProfession(FGameplayTag InProfessionIDTag, FLxProfessionLearnCheckResult& OutCheckResult)
{
	OutCheckResult = FLxProfessionLearnCheckResult();
	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	return DataTransferComponent != nullptr && DataTransferComponent->CanLearnProfession(InProfessionIDTag, OutCheckResult);
}

bool ULxCharacterTestComponent::LearnTestProfession(FGameplayTag InProfessionIDTag)
{
	if (!InProfessionIDTag.IsValid())
	{
		return false;
	}

	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	return DataTransferComponent != nullptr && DataTransferComponent->LearnProfession(InProfessionIDTag);
}

void ULxCharacterTestComponent::AddTestProfessionExperienceByType(ELxProfessionType InProfessionType, float InExperience)
{
	if (InExperience <= 0.f)
	{
		return;
	}

	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	if (DataTransferComponent != nullptr)
	{
		DataTransferComponent->AddProfessionExperienceByType(InProfessionType, InExperience);
	}
}

bool ULxCharacterTestComponent::ApplyTestDamageFromAttacker(AActor* InAttackerActor, float& OutFinalDamageValue, AActor*& OutAttackerActor, bool bApplyResult)
{
	OutFinalDamageValue = 0.f;
	OutAttackerActor = nullptr;

	const ALxBaseCharacter* AttackerCharacter = Cast<ALxBaseCharacter>(InAttackerActor);
	if (AttackerCharacter == nullptr)
	{
		return false;
	}

	ULxCharacterDamageComponent* AttackerDamageComponent = AttackerCharacter->GetCharacterDamageComponent();
	ULxCharacterDamageComponent* TargetDamageComponent = GetDamageComponent();
	if (AttackerDamageComponent == nullptr || TargetDamageComponent == nullptr)
	{
		return false;
	}

	FLxEffectPackage OutgoingDamagePackage;
	if (!AttackerDamageComponent->BuildOutgoingDamagePackage(GetOwner(), OutgoingDamagePackage))
	{
		return false;
	}

	FLxEffectPackage AppliedDamagePackage;
	if (!TargetDamageComponent->ReceiveIncomingDamagePackage(OutgoingDamagePackage, AppliedDamagePackage, bApplyResult))
	{
		return false;
	}

	OutFinalDamageValue = CalculateFinalDamageValueFromAppliedPackage(AppliedDamagePackage);
	OutAttackerActor = AppliedDamagePackage.SourceContext.SourceActor != nullptr ? AppliedDamagePackage.SourceContext.SourceActor.Get() : InAttackerActor;
	return true;
}

ULxCharacterDataTransferComponent* ULxCharacterTestComponent::GetDataTransferComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
}

ULxCharacterDamageComponent* ULxCharacterTestComponent::GetDamageComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDamageComponent() : nullptr;
}

float ULxCharacterTestComponent::CalculateFinalDamageValueFromAppliedPackage(const FLxEffectPackage& InAppliedPackage)
{
	float FinalDamageValue = 0.f;
	for (const FLxAttributeRecoveryEffect& RecoveryEffect : InAppliedPackage.AttributeRecoveryEffects)
	{
		if (RecoveryEffect.RecoveryOperation == ELxAttributeModifierOperation::AddValue && RecoveryEffect.RecoveryValue < 0.f)
		{
			FinalDamageValue += -RecoveryEffect.RecoveryValue;
		}
	}

	return FinalDamageValue;
}

void ULxCharacterTestComponent::HandleCharacterDamageReceived(const FLxEffectPackage& InAppliedDamagePackage, AActor* InAttackerActor)
{
	const float FinalDamageValue = CalculateFinalDamageValueFromAppliedPackage(InAppliedDamagePackage);
	OnTestReceivedDamageValueOutput.Broadcast(FinalDamageValue);
	OnTestReceivedDamageAttackerOutput.Broadcast(InAttackerActor);
}
