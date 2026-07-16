#include "LxCharacterSpecialAttributeObject.h"

#include "LxCharacterSpecialAttributeComponent.h"

void ULxCharacterSpecialAttributeObject::InitializeSpecialAttributeObject(ULxCharacterSpecialAttributeComponent* InOwnerComponent)
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
