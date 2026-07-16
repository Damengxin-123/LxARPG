#include "LxCharacterLifecycleAttributeObject.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterLifecycleAttributeObject::ULxCharacterLifecycleAttributeObject()
{
	AliveStateTag = LxTag_CharacterState_Lifecycle_Alive;
	DeadStateTag = LxTag_CharacterState_Lifecycle_Dead;
}

void ULxCharacterLifecycleAttributeObject::ApplyMovementControl(const bool bInAlive) const
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (!bDisableMovementWhenDead || OwnerCharacter == nullptr)
	{
		return;
	}

	if (AController* Controller = OwnerCharacter->GetController())
	{
		Controller->SetIgnoreMoveInput(!bInAlive);
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		if (bInAlive && MovementComponent->MovementMode == MOVE_None)
		{
			MovementComponent->SetMovementMode(MOVE_Walking);
		}
		else if (!bInAlive)
		{
			MovementComponent->StopMovementImmediately();
			MovementComponent->DisableMovement();
		}
	}
}
