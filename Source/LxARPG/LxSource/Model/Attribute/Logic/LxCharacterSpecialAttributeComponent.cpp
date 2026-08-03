#include "LxCharacterSpecialAttributeComponent.h"

#include "LxCharacterLifecycleAttributeObject.h"
#include "LxCharacterSpecialAttributeObject.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "Net/UnrealNetwork.h"

ULxCharacterSpecialAttributeComponent::ULxCharacterSpecialAttributeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	SpecialAttributeObjectClasses.Add(ULxCharacterLifecycleAttributeObject::StaticClass());
}

void ULxCharacterSpecialAttributeComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	CreateRuntimeSpecialAttributeObjects();
	ApplyLifecycleState();
}

void ULxCharacterSpecialAttributeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (ULxCharacterSpecialAttributeObject* RuntimeObject : RuntimeSpecialAttributeObjects)
	{
		if (RuntimeObject != nullptr)
		{
			RuntimeObject->DeinitializeSpecialAttributeObject();
		}
	}
	RuntimeSpecialAttributeObjects.Reset();
	Super::EndPlay(EndPlayReason);
}

void ULxCharacterSpecialAttributeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, ElementAbnormalStateTags);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, LifecycleStateTags);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, MovementStateTags);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, CombatStateTags);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, CharacterFaction);
	DOREPLIFETIME(ULxCharacterSpecialAttributeComponent, bIsAlive);
}

bool ULxCharacterSpecialAttributeComponent::GetStateTagsByCategory(const FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const
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

bool ULxCharacterSpecialAttributeComponent::SetStateTagsByCategory(const FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr)
	{
		return false;
	}

	for (const FGameplayTag& StateTag : InStateTags)
	{
		if (!StateTag.IsValid() || !StateTag.MatchesTag(InStateCategoryTag))
		{
			return false;
		}
	}

	*StateContainer = InStateTags;
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterSpecialAttributeComponent::AddStateTag(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !InStateTag.IsValid() || !InStateTag.MatchesTag(InStateCategoryTag) || StateContainer->HasTagExact(InStateTag))
	{
		return false;
	}
	StateContainer->AddTag(InStateTag);
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterSpecialAttributeComponent::RemoveStateTag(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag)
{
	FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	if (StateContainer == nullptr || !StateContainer->RemoveTag(InStateTag))
	{
		return false;
	}
	BroadcastStateTagsChanged(InStateCategoryTag, *StateContainer);
	return true;
}

bool ULxCharacterSpecialAttributeComponent::HasStateTag(const FGameplayTag InStateTag) const
{
	return InStateTag.IsValid() && (ElementAbnormalStateTags.HasTag(InStateTag)
		|| LifecycleStateTags.HasTag(InStateTag)
		|| MovementStateTags.HasTag(InStateTag)
		|| CombatStateTags.HasTag(InStateTag));
}

bool ULxCharacterSpecialAttributeComponent::HasStateTagInCategory(const FGameplayTag InStateCategoryTag, const FGameplayTag InStateTag) const
{
	const FGameplayTagContainer* StateContainer = FindStateContainer(InStateCategoryTag);
	return StateContainer != nullptr && InStateTag.IsValid() && StateContainer->HasTag(InStateTag);
}

void ULxCharacterSpecialAttributeComponent::GetAllStateTags(FGameplayTagContainer& OutStateTags) const
{
	OutStateTags.Reset();
	OutStateTags.AppendTags(ElementAbnormalStateTags);
	OutStateTags.AppendTags(LifecycleStateTags);
	OutStateTags.AppendTags(MovementStateTags);
	OutStateTags.AppendTags(CombatStateTags);
}

bool ULxCharacterSpecialAttributeComponent::ClearStateTagsByCategory(const FGameplayTag InStateCategoryTag)
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

void ULxCharacterSpecialAttributeComponent::SetCharacterAlive()
{
	SetCharacterAliveState(true);
}

void ULxCharacterSpecialAttributeComponent::SetCharacterDead()
{
	SetCharacterAliveState(false);
}

void ULxCharacterSpecialAttributeComponent::SetCharacterAliveState(const bool bInAlive)
{
	const bool bStateChanged = bIsAlive != bInAlive;
	bIsAlive = bInAlive;
	ApplyLifecycleState();
	if (bStateChanged)
	{
		OnLifecycleStateChanged.Broadcast(bIsAlive, GetCurrentLifecycleStateTag());
		OnDataChange.Broadcast();
	}
}

FGameplayTag ULxCharacterSpecialAttributeComponent::GetCurrentLifecycleStateTag() const
{
	const ULxCharacterLifecycleAttributeObject* LifecycleObject = GetLifecycleAttributeObject();
	if (LifecycleObject == nullptr)
	{
		return FGameplayTag();
	}
	return bIsAlive ? LifecycleObject->GetAliveStateTag() : LifecycleObject->GetDeadStateTag();
}

ELxCharacterCampType ULxCharacterSpecialAttributeComponent::GetFactionRelation(const FGameplayTag InFactionTag) const
{
	if (InFactionTag.MatchesAny(CharacterFaction.FriendlyTags))
	{
		return ELxCharacterCampType::Friendly;
	}
	if (InFactionTag.MatchesAny(CharacterFaction.HostileTags))
	{
		return ELxCharacterCampType::Hostile;
	}
	return ELxCharacterCampType::Neutral;
}

ULxCharacterSpecialAttributeObject* ULxCharacterSpecialAttributeComponent::FindSpecialAttributeObject(const TSubclassOf<ULxCharacterSpecialAttributeObject> InObjectClass) const
{
	if (!InObjectClass)
	{
		return nullptr;
	}

	for (ULxCharacterSpecialAttributeObject* RuntimeObject : RuntimeSpecialAttributeObjects)
	{
		if (RuntimeObject != nullptr && RuntimeObject->IsA(InObjectClass))
		{
			return RuntimeObject;
		}
	}
	return nullptr;
}

void ULxCharacterSpecialAttributeComponent::CreateRuntimeSpecialAttributeObjects()
{
	for (ULxCharacterSpecialAttributeObject* RuntimeObject : RuntimeSpecialAttributeObjects)
	{
		if (RuntimeObject != nullptr)
		{
			RuntimeObject->DeinitializeSpecialAttributeObject();
		}
	}
	RuntimeSpecialAttributeObjects.Reset();

	for (const TSubclassOf<ULxCharacterSpecialAttributeObject> ObjectClass : SpecialAttributeObjectClasses)
	{
		if (!ObjectClass || ObjectClass->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}

		ULxCharacterSpecialAttributeObject* RuntimeObject = NewObject<ULxCharacterSpecialAttributeObject>(this, ObjectClass);
		RuntimeObject->InitializeSpecialAttributeObject(this);
		RuntimeSpecialAttributeObjects.Add(RuntimeObject);
	}
}

ULxCharacterLifecycleAttributeObject* ULxCharacterSpecialAttributeComponent::GetLifecycleAttributeObject() const
{
	return Cast<ULxCharacterLifecycleAttributeObject>(FindSpecialAttributeObject(ULxCharacterLifecycleAttributeObject::StaticClass()));
}

FGameplayTagContainer* ULxCharacterSpecialAttributeComponent::FindStateContainer(const FGameplayTag InStateCategoryTag)
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal) return &ElementAbnormalStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle) return &LifecycleStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Movement) return &MovementStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Combat) return &CombatStateTags;
	return nullptr;
}

const FGameplayTagContainer* ULxCharacterSpecialAttributeComponent::FindStateContainer(const FGameplayTag InStateCategoryTag) const
{
	if (InStateCategoryTag == LxTag_CharacterState_ElementAbnormal) return &ElementAbnormalStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Lifecycle) return &LifecycleStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Movement) return &MovementStateTags;
	if (InStateCategoryTag == LxTag_CharacterState_Combat) return &CombatStateTags;
	return nullptr;
}

void ULxCharacterSpecialAttributeComponent::BroadcastStateTagsChanged(const FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags)
{
	OnStateTagsChanged.Broadcast(InStateCategoryTag, InStateTags);
	OnDataChange.Broadcast();
}

void ULxCharacterSpecialAttributeComponent::ApplyLifecycleState()
{
	if (ULxCharacterLifecycleAttributeObject* LifecycleObject = GetLifecycleAttributeObject())
	{
		LifecycleStateTags.RemoveTag(LifecycleObject->GetAliveStateTag());
		LifecycleStateTags.RemoveTag(LifecycleObject->GetDeadStateTag());
		const FGameplayTag CurrentStateTag = GetCurrentLifecycleStateTag();
		if (CurrentStateTag.IsValid())
		{
			LifecycleStateTags.AddTag(CurrentStateTag);
		}
		LifecycleObject->ApplyMovementControl(bIsAlive);
		BroadcastStateTagsChanged(LxTag_CharacterState_Lifecycle, LifecycleStateTags);
	}
}

void ULxCharacterSpecialAttributeComponent::OnRep_ElementAbnormalStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_ElementAbnormal, ElementAbnormalStateTags);
}

void ULxCharacterSpecialAttributeComponent::OnRep_LifecycleStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Lifecycle, LifecycleStateTags);
}

void ULxCharacterSpecialAttributeComponent::OnRep_MovementStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Movement, MovementStateTags);
}

void ULxCharacterSpecialAttributeComponent::OnRep_CombatStateTags()
{
	BroadcastStateTagsChanged(LxTag_CharacterState_Combat, CombatStateTags);
}

void ULxCharacterSpecialAttributeComponent::OnRep_IsAlive()
{
	ApplyLifecycleState();
	OnLifecycleStateChanged.Broadcast(bIsAlive, GetCurrentLifecycleStateTag());
}
