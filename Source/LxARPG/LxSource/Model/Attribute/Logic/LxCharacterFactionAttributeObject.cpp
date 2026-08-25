#include "LxCharacterFactionAttributeObject.h"

#include "Net/UnrealNetwork.h"

void ULxCharacterFactionAttributeObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterFactionAttributeObject, CharacterFaction);
}

ELxCharacterFactionRelation ULxCharacterFactionAttributeObject::GetFactionRelation(const FGameplayTagContainer& InTargetFactionTags) const
{
	if (CharacterFaction.FriendlyTags.HasAny(InTargetFactionTags)) return ELxCharacterFactionRelation::Friendly;
	if (CharacterFaction.HostileTags.HasAny(InTargetFactionTags)) return ELxCharacterFactionRelation::Hostile;
	return ELxCharacterFactionRelation::Neutral;
}
