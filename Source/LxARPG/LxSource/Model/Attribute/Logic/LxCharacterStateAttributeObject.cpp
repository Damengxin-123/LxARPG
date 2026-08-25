#include "LxCharacterStateAttributeObject.h"

#include "LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "Net/UnrealNetwork.h"

void ULxCharacterStateAttributeObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterStateAttributeObject, ElementAbnormalStateTags);
	DOREPLIFETIME(ULxCharacterStateAttributeObject, LifecycleStateTags);
	DOREPLIFETIME(ULxCharacterStateAttributeObject, MovementStateTags);
	DOREPLIFETIME(ULxCharacterStateAttributeObject, CombatStateTags);
}

bool ULxCharacterStateAttributeObject::GetStateTagsByCategory(const FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	if (InStateCategoryTag == LxTag_CharacterState)
	{
		GetAllStateTags(OutStateTags);
		return true;
	}

	const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr) return false;
	OutStateTags = *StateContainer;
	return true;
}

bool ULxCharacterStateAttributeObject::SetStateTagsByCategory(const FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr) return false;
	for (const FGameplayTag& StateTag : InStateTags)
	{
		if (!StateTag.IsValid() || !StateTag.MatchesTag(InStateCategoryTag)) return false;
	}
	*StateContainer = InStateTags;
	NotifyStateTagsChanged(InStateCategoryTag);
	return true;
}

bool ULxCharacterStateAttributeObject::AddStateTag(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !InStateTag.IsValid() || !InStateTag.MatchesTag(InStateCategoryTag) || StateContainer->HasTagExact(InStateTag)) return false;
	StateContainer->AddTag(InStateTag);
	NotifyStateTagsChanged(InStateCategoryTag);
	return true;
}

bool ULxCharacterStateAttributeObject::RemoveStateTag(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !StateContainer->RemoveTag(InStateTag)) return false;
	NotifyStateTagsChanged(InStateCategoryTag);
	return true;
}

bool ULxCharacterStateAttributeObject::HasStateTag(const FGameplayTag InStateTag) const
{
	return InStateTag.IsValid() && (ElementAbnormalStateTags.HasTag(InStateTag) || LifecycleStateTags.HasTag(InStateTag)
		|| MovementStateTags.HasTag(InStateTag) || CombatStateTags.HasTag(InStateTag));
}

bool ULxCharacterStateAttributeObject::HasStateTagInCategory(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag) const
{
	const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	return StateContainer != nullptr && InStateTag.IsValid() && StateContainer->HasTag(InStateTag);
}

void ULxCharacterStateAttributeObject::GetAllStateTags(FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	OutStateTags.AppendTags(ElementAbnormalStateTags);
	OutStateTags.AppendTags(LifecycleStateTags);
	OutStateTags.AppendTags(MovementStateTags);
	OutStateTags.AppendTags(CombatStateTags);
}

bool ULxCharacterStateAttributeObject::ClearStateTagsByCategory(const FGameplayTag InStateCategoryTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || StateContainer->IsEmpty()) return false;
	StateContainer->Reset();
	NotifyStateTagsChanged(InStateCategoryTag);
	return true;
}

FGameplayTagContainer* ULxCharacterStateAttributeObject::FindStateContainer(const FGameplayTag InStateCategoryTag)
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal) return &ElementAbnormalStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle) return &LifecycleStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Movement) return &MovementStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Combat) return &CombatStateTags;
	return nullptr;
}

const FGameplayTagContainer* ULxCharacterStateAttributeObject::FindStateContainer(const FGameplayTag InStateCategoryTag) const
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal) return &ElementAbnormalStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle) return &LifecycleStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Movement) return &MovementStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Combat) return &CombatStateTags;
	return nullptr;
}

void ULxCharacterStateAttributeObject::NotifyStateTagsChanged(const FGameplayTag InStateCategoryTag) const
{
	if (OwnerComponent != nullptr)
	{
		const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
		if (StateContainer != nullptr) OwnerComponent->HandleStateAttributeChanged(InStateCategoryTag, *StateContainer);
	}
}

void ULxCharacterStateAttributeObject::OnRep_ElementAbnormalStateTags() { NotifyStateTagsChanged(LxTag_CharacterState_ElementAbnormal); }
void ULxCharacterStateAttributeObject::OnRep_LifecycleStateTags() { NotifyStateTagsChanged(LxTag_CharacterState_Lifecycle); }
void ULxCharacterStateAttributeObject::OnRep_MovementStateTags() { NotifyStateTagsChanged(LxTag_CharacterState_Movement); }
void ULxCharacterStateAttributeObject::OnRep_CombatStateTags() { NotifyStateTagsChanged(LxTag_CharacterState_Combat); }
