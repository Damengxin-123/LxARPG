#include "LxCharacterLifecycleAttributeObject.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterSpecialAttributeComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterLifecycleAttributeObject::ULxCharacterLifecycleAttributeObject()
{
	AliveStateTag = LxTag_CharacterState_Lifecycle_Alive;
	DeadStateTag = LxTag_CharacterState_Lifecycle_Dead;
}

void ULxCharacterLifecycleAttributeObject::InitializeSpecialAttributeObject(ULxCharacterSpecialAttributeComponent* InOwnerComponent)
{
	Super::InitializeSpecialAttributeObject(InOwnerComponent);
	if (ALxBaseCharacter* OwnerCharacter = GetCharacterOwner())
	{
		if (ULxCharacterAttributeComponent* AttributeComponent = OwnerCharacter->GetCharacterAttributeComponent())
		{
			AttributeComponent->OnTypedAttributeSnapshotChanged.RemoveDynamic(this, &ULxCharacterLifecycleAttributeObject::HandleCharacterAttributesChanged);
			AttributeComponent->OnTypedAttributeSnapshotChanged.AddDynamic(this, &ULxCharacterLifecycleAttributeObject::HandleCharacterAttributesChanged);
		}
	}
	EvaluateDeathFromCurrentAttributes();
}

void ULxCharacterLifecycleAttributeObject::DeinitializeSpecialAttributeObject()
{
	if (ALxBaseCharacter* OwnerCharacter = GetCharacterOwner())
	{
		if (ULxCharacterAttributeComponent* AttributeComponent = OwnerCharacter->GetCharacterAttributeComponent())
		{
			AttributeComponent->OnTypedAttributeSnapshotChanged.RemoveDynamic(this, &ULxCharacterLifecycleAttributeObject::HandleCharacterAttributesChanged);
		}
	}
	Super::DeinitializeSpecialAttributeObject();
}

void ULxCharacterLifecycleAttributeObject::ApplyLifecycleState(const bool bInAlive)
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	if (bDisableMovementWhenDead)
	{
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

	if (bInAlive)
	{
		bDeathSequenceStarted = false;
		if (OwnerCharacter->HasAuthority())
		{
			OwnerCharacter->SetLifeSpan(0.0f);
		}
		return;
	}

	StartDeathSequence();
}

void ULxCharacterLifecycleAttributeObject::HandleCharacterAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	EvaluateDeathFromAttributeSnapshot(AttributeSnapshot);
}

void ULxCharacterLifecycleAttributeObject::EvaluateDeathFromAttributeSnapshot(const FLxTypedAttributeSnapshot& AttributeSnapshot) const
{
	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr || !OwnerCharacter->HasAuthority() || OwnerComponent == nullptr || !OwnerComponent->IsCharacterAlive())
	{
		return;
	}

	for (const FLxResourceAttributeData& ResourceAttribute : AttributeSnapshot.ResourceAttributes)
	{
		if (ResourceAttribute.AttributeIDTag == LxTag_Attribute_Resource_Health && ResourceAttribute.Value <= 0.0f)
		{
			OwnerComponent->SetCharacterDead();
			return;
		}
	}
}

void ULxCharacterLifecycleAttributeObject::EvaluateDeathFromCurrentAttributes() const
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	const ULxCharacterAttributeComponent* AttributeComponent = OwnerCharacter != nullptr
		? OwnerCharacter->GetCharacterAttributeComponent()
		: nullptr;
	const ULxCharacterBaseAttributeSet* AttributeSet = AttributeComponent != nullptr
		? AttributeComponent->GetRuntimeAttributeSet()
		: nullptr;
	if (AttributeSet == nullptr)
	{
		return;
	}

	FLxResourceAttributeData HealthAttribute;
	if (AttributeSet->GetResourceAttribute(LxTag_Attribute_Resource_Health, HealthAttribute))
	{
		FLxTypedAttributeSnapshot AttributeSnapshot;
		AttributeSnapshot.ResourceAttributes.Add(HealthAttribute);
		EvaluateDeathFromAttributeSnapshot(AttributeSnapshot);
	}
}

void ULxCharacterLifecycleAttributeObject::StartDeathSequence()
{
	if (bDeathSequenceStarted)
	{
		return;
	}
	bDeathSequenceStarted = true;

	ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}
	OwnerCharacter->SetCharacterState(ELxCharacterState::Dead);

	if (ULxCharacterAnimationProcessComponent* AnimationProcessComponent = OwnerCharacter->GetCharacterAnimationProcessComponent())
	{
		FLxCharacterMotionSignal DeathMotionSignal;
		DeathMotionSignal.MotionType = DeathAnimationType;
		DeathMotionSignal.MotionSpeed = 1.0f;
		DeathMotionSignal.bLoop = false;
		AnimationProcessComponent->ReceiveActionMotionSignal(DeathMotionSignal);
	}

	if (bDestroyCharacterWhenDead && OwnerCharacter->HasAuthority())
	{
		if (DeathDestroyDelay <= 0.0f)
		{
			OwnerCharacter->Destroy();
		}
		else
		{
			OwnerCharacter->SetLifeSpan(DeathDestroyDelay);
		}
	}
}
