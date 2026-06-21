#include "LxCharacterTestComponent.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterTestComponent::ULxCharacterTestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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

ULxCharacterDataTransferComponent* ULxCharacterTestComponent::GetDataTransferComponent() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	return OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr;
}
