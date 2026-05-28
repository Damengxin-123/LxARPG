#include "LxCharacterStateComponent.h"

#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "Net/UnrealNetwork.h"

ULxCharacterStateComponent::ULxCharacterStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULxCharacterStateComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
}

void ULxCharacterStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxCharacterStateComponent, ElementAbnormalStateTags);
	DOREPLIFETIME(ULxCharacterStateComponent, LifecycleStateTags);
	DOREPLIFETIME(ULxCharacterStateComponent, MovementStateTags);
	DOREPLIFETIME(ULxCharacterStateComponent, CombatStateTags);
}

bool ULxCharacterStateComponent::GetStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();

	if (InStateCategoryTag == LxTag_CharacterState)
	{
		GetAllStateTags(OutStateTags);
		return true;
	}

	const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr)
	{
		return false;
	}

	OutStateTags = *StateContainer;
	return true;
}

bool ULxCharacterStateComponent::SetStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr)
	{
		return false;
	}

	for (const FGameplayTag& StateTag : InStateTags)
	{
		if (!IsStateTagAllowedForCategory(InStateCategoryTag, StateTag))
		{
			return false;
		}
	}

	*StateContainer = InStateTags;
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterStateComponent::AddStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !IsStateTagAllowedForCategory(InStateCategoryTag, InStateTag))
	{
		return false;
	}

	if (StateContainer->HasTagExact(InStateTag))
	{
		return false;
	}

	StateContainer->AddTag(InStateTag);
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterStateComponent::RemoveStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !InStateTag.IsValid())
	{
		return false;
	}

	if (!StateContainer->RemoveTag(InStateTag))
	{
		return false;
	}

	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterStateComponent::HasStateTag(FGameplayTag InStateTag) const
{
	if (!InStateTag.IsValid())
	{
		return false;
	}

	return ElementAbnormalStateTags.HasTag(InStateTag)
		|| LifecycleStateTags.HasTag(InStateTag)
		|| MovementStateTags.HasTag(InStateTag)
		|| CombatStateTags.HasTag(InStateTag);
}

bool ULxCharacterStateComponent::HasStateTagInCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const
{
	const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	return StateContainer != nullptr && InStateTag.IsValid() && StateContainer->HasTag(InStateTag);
}

void ULxCharacterStateComponent::GetAllStateTags(FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	OutStateTags.AppendTags(ElementAbnormalStateTags);
	OutStateTags.AppendTags(LifecycleStateTags);
	OutStateTags.AppendTags(MovementStateTags);
	OutStateTags.AppendTags(CombatStateTags);
}

bool ULxCharacterStateComponent::ClearStateTagsByCategory(FGameplayTag InStateCategoryTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || StateContainer->IsEmpty())
	{
		return false;
	}

	StateContainer->Reset();
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

FGameplayTagContainer* ULxCharacterStateComponent::FindStateContainer(FGameplayTag InStateCategoryTag)
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal)
	{
		return &ElementAbnormalStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle)
	{
		return &LifecycleStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Movement)
	{
		return &MovementStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Combat)
	{
		return &CombatStateTags;
	}

	return nullptr;
}

const FGameplayTagContainer* ULxCharacterStateComponent::FindStateContainer(FGameplayTag InStateCategoryTag) const
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal)
	{
		return &ElementAbnormalStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle)
	{
		return &LifecycleStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Movement)
	{
		return &MovementStateTags;
	}

	if (InStateCategoryTag == LxTag_CharacterState_Combat)
	{
		return &CombatStateTags;
	}

	return nullptr;
}

bool ULxCharacterStateComponent::IsStateTagAllowedForCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const
{
	return InStateCategoryTag.IsValid()
		&& InStateTag.IsValid()
		&& FindStateContainer(InStateCategoryTag) != nullptr
		&& InStateTag.MatchesTag(InStateCategoryTag);
}

void ULxCharacterStateComponent::BroadcastStateTagsChanged(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	OnStateTagsChanged.Broadcast(InStateCategoryTag, InStateTags);
	OnDataChange.Broadcast();
}

void ULxCharacterStateComponent::OnRep_ElementAbnormalStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_ElementAbnormal, ElementAbnormalStateTags);
}

void ULxCharacterStateComponent::OnRep_LifecycleStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Lifecycle, LifecycleStateTags);
}

void ULxCharacterStateComponent::OnRep_MovementStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Movement, MovementStateTags);
}

void ULxCharacterStateComponent::OnRep_CombatStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Combat, CombatStateTags);
}
