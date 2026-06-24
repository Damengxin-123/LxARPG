#include "LxCharacterLifecycleComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/State/Logic/LxCharacterStateComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterLifecycleComponent::ULxCharacterLifecycleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	AliveStateTag = LxTag_CharacterState_Lifecycle_Alive;
	DeadStateTag = LxTag_CharacterState_Lifecycle_Dead;
}

void ULxCharacterLifecycleComponent::BaseComponentInitialize()
{
	CacheOwnerComponents();
	if (StateComponent != nullptr && StateComponent->HasStateTag(DeadStateTag))
	{
		bIsAlive = false;
	}

	SyncLifecycleStateToStateComponent();
	ApplyMovementControlByLifecycle();
}

void ULxCharacterLifecycleComponent::SetCharacterAlive()
{
	SetCharacterAliveState(true);
}

void ULxCharacterLifecycleComponent::SetCharacterDead()
{
	SetCharacterAliveState(false);
}

void ULxCharacterLifecycleComponent::SetCharacterAliveState(bool bInAlive)
{
	if (bIsAlive == bInAlive)
	{
		SyncLifecycleStateToStateComponent();
		ApplyMovementControlByLifecycle();
		return;
	}

	bIsAlive = bInAlive;
	SyncLifecycleStateToStateComponent();
	ApplyMovementControlByLifecycle();
	OnLifecycleStateChanged.Broadcast(bIsAlive, GetCurrentLifecycleStateTag());
}

FGameplayTag ULxCharacterLifecycleComponent::GetCurrentLifecycleStateTag() const
{
	return bIsAlive ? AliveStateTag : DeadStateTag;
}

void ULxCharacterLifecycleComponent::CacheOwnerComponents()
{
	OwnerCharacter = GetCharacterOwner();
	StateComponent = OwnerCharacter != nullptr ? OwnerCharacter->GetCharacterStateComponent() : nullptr;
}

void ULxCharacterLifecycleComponent::SyncLifecycleStateToStateComponent()
{
	if (StateComponent == nullptr)
	{
		return;
	}

	const FGameplayTag CurrentStateTag = GetCurrentLifecycleStateTag();
	if (AliveStateTag.IsValid())
	{
		StateComponent->RemoveStateTag(LxTag_CharacterState_Lifecycle, AliveStateTag);
	}

	if (DeadStateTag.IsValid())
	{
		StateComponent->RemoveStateTag(LxTag_CharacterState_Lifecycle, DeadStateTag);
	}

	if (CurrentStateTag.IsValid())
	{
		StateComponent->AddStateTag(LxTag_CharacterState_Lifecycle, CurrentStateTag);
	}
}

void ULxCharacterLifecycleComponent::ApplyMovementControlByLifecycle()
{
	if (!bDisableMovementWhenDead || OwnerCharacter == nullptr)
	{
		return;
	}

	if (AController* Controller = OwnerCharacter->GetController())
	{
		Controller->SetIgnoreMoveInput(!bIsAlive);
	}

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		if (bIsAlive)
		{
			if (MovementComponent->MovementMode == MOVE_None)
			{
				MovementComponent->SetMovementMode(MOVE_Walking);
			}
		}
		else
		{
			MovementComponent->StopMovementImmediately();
			MovementComponent->DisableMovement();
		}
	}
}
