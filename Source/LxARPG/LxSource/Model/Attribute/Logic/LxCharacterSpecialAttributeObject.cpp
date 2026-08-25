#include "LxCharacterSpecialAttributeObject.h"

#include "LxCharacterAttributeComponent.h"

void ULxCharacterSpecialAttributeObject::InitializeSpecialAttributeObject(ULxCharacterAttributeComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void ULxCharacterSpecialAttributeObject::DeinitializeSpecialAttributeObject()
{
	OwnerComponent = nullptr;
}

ALxBaseCharacter* ULxCharacterSpecialAttributeObject::GetCharacterOwner() const
{
	return OwnerComponent != nullptr ? OwnerComponent->GetCharacterOwner() : nullptr;
}
