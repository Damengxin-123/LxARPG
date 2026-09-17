#include "LxInteractionData.h"

#include "Serialization/Archive.h"

void FLxInteractionRequirement::PostSerialize(const FArchive& Ar)
{
	if (!Ar.IsLoading())
	{
		return;
	}

	for (const FLxItemQuote& Item : RequiredItems_DEPRECATED)
	{
		FLxInteractionItemRequirement& Migrated = ItemRequirements.AddDefaulted_GetRef();
		Migrated.ItemIDTag = Item.ItemIDTag;
		Migrated.ItemCount = Item.ItemCount;
	}
	RequiredItems_DEPRECATED.Reset();

	for (const FGameplayTag& StateTag : RequiredStateTags_DEPRECATED)
	{
		StateRequirements.AddDefaulted_GetRef().StateTag = StateTag;
	}
	RequiredStateTags_DEPRECATED.Reset();
}
