#include "LxCharacterCombatModuleBase.h"

#include "LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxCharacterCombatModuleBase::InitializeModule(ULxCharacterCombatComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void ULxCharacterCombatModuleBase::ShutdownModule()
{
	OwnerComponent = nullptr;
}

ALxBaseCharacter* ULxCharacterCombatModuleBase::GetCharacterOwner() const
{
	return OwnerComponent != nullptr ? Cast<ALxBaseCharacter>(OwnerComponent->GetOwner()) : nullptr;
}

AActor* ULxCharacterCombatModuleBase::GetOwner() const
{
	return OwnerComponent != nullptr ? OwnerComponent->GetOwner() : nullptr;
}

UWorld* ULxCharacterCombatModuleBase::GetWorld() const
{
	return OwnerComponent != nullptr ? OwnerComponent->GetWorld() : nullptr;
}

void ULxCharacterCombatModuleBase::BroadcastModuleDataChanged() const
{
	if (OwnerComponent != nullptr) OwnerComponent->NotifyCombatModuleDataChanged();
}
