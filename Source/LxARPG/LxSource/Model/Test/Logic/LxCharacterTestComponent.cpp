#include "LxCharacterTestComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageTags.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterTestComponent::ULxCharacterTestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterTestComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();

	if (ULxCharacterEffectProcessComponent* EffectProcessComponent = GetEffectProcessComponent())
	{
		EffectProcessComponent->OnCharacterDamageReceived.RemoveDynamic(this, &ULxCharacterTestComponent::HandleCharacterDamageReceived);
		EffectProcessComponent->OnCharacterDamageReceived.AddDynamic(this, &ULxCharacterTestComponent::HandleCharacterDamageReceived);
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

bool ULxCharacterTestComponent::AddTestItemList(const TArray<FLxItemQuote>& InItemList)
{
	if (InItemList.IsEmpty())
	{
		return false;
	}

	ULxCharacterDataTransferComponent* DataTransferComponent = GetDataTransferComponent();
	if (DataTransferComponent == nullptr)
	{
		return false;
	}

	TArray<FLxItemQuote> BackpackItemList;
	TArray<FGameplayTag> SkillItemIDTags;
	for (const FLxItemQuote& ItemQuote : InItemList)
	{
		if (!ItemQuote.ItemIDTag.IsValid() || ItemQuote.ItemCount <= 0)
		{
			return false;
		}

		const FLxItemInformationBase* ItemConfig = LxItemConfig::GetItemData(ItemQuote.ItemIDTag);
		if (ItemConfig == nullptr)
		{
			return false;
		}

		if (ItemConfig->ItemType == ELxItemType::Skill)
		{
			SkillItemIDTags.AddUnique(ItemQuote.ItemIDTag);
			continue;
		}

		BackpackItemList.Add(ItemQuote);
	}

	if (!BackpackItemList.IsEmpty() && !DataTransferComponent->CanAddItemListToBackpack(BackpackItemList))
	{
		return false;
	}

	for (const FGameplayTag SkillItemIDTag : SkillItemIDTags)
	{
		if (!DataTransferComponent->AddSkillItemToSkillBackpack(SkillItemIDTag))
		{
			return false;
		}
	}

	return BackpackItemList.IsEmpty() || DataTransferComponent->AddItemListToBackpack(BackpackItemList);
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

	ULxCharacterEffectProcessComponent* AttackerEffectProcessComponent = AttackerCharacter->GetCharacterEffectProcessComponent();
	ULxCharacterEffectProcessComponent* TargetEffectProcessComponent = GetEffectProcessComponent();
	if (AttackerEffectProcessComponent == nullptr || TargetEffectProcessComponent == nullptr)
	{
		return false;
	}

	FLxDamageValue DamageValue;
	DamageValue.DamageTypeTag = LxTag_CommonEffect_DamageEffect_Normal;
	DamageValue.SourceAttributeIDTag = LxTag_Attribute_Numeric_AttackPower;
	DamageValue.SourceAttributeRatio = 1.f;

	FLxDamageEffect DamageEffect;
	DamageEffect.TargetAttributeIDTag = LxTag_Attribute_Resource_Health;
	DamageEffect.DamageValues.Add(DamageValue);

	FLxEffectPackage SourceEffectPackage;
	SourceEffectPackage.SourceContext.SourceType = ELxEffectPackageSource::Other;
	SourceEffectPackage.SourceContext.SourceActor = InAttackerActor;
	SourceEffectPackage.SourceContext.SourceObject = this;
	SourceEffectPackage.TargetActor = GetOwner();
	SourceEffectPackage.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
	SourceEffectPackage.DamageEffects.Add(DamageEffect);

	FLxEffectPackage OutgoingDamagePackage;
	if (!AttackerEffectProcessComponent->BuildOutgoingEffectPackage(SourceEffectPackage, GetOwner(), OutgoingDamagePackage))
	{
		return false;
	}

	FLxDamageReceiveResult DamageReceiveResult;
	if (!TargetEffectProcessComponent->ReceiveIncomingEffectPackage(OutgoingDamagePackage, DamageReceiveResult, bApplyResult))
	{
		return false;
	}

	OutFinalDamageValue = CalculateFinalDamageValueFromReceiveResult(DamageReceiveResult);
	OutAttackerActor = InAttackerActor;
	return true;
}

ULxCharacterDataTransferComponent* ULxCharacterTestComponent::GetDataTransferComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
}

ULxCharacterEffectProcessComponent* ULxCharacterTestComponent::GetEffectProcessComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterEffectProcessComponent() : nullptr;
}

float ULxCharacterTestComponent::CalculateFinalDamageValueFromReceiveResult(const FLxDamageReceiveResult& InDamageReceiveResult)
{
	return InDamageReceiveResult.GetTotalDamageValue();
}
void ULxCharacterTestComponent::HandleCharacterDamageReceived(const FLxDamageReceiveResult& InDamageReceiveResult, AActor* InAttackerActor)
{
	const float FinalDamageValue = CalculateFinalDamageValueFromReceiveResult(InDamageReceiveResult);
	OnTestReceivedDamageValueOutput.Broadcast(FinalDamageValue);
	OnTestReceivedDamageAttackerOutput.Broadcast(InAttackerActor);
}
