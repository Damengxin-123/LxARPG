#include "LxCharacterMoveComponent.h"

#include "GameFramework/Controller.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterMoveComponent::ULxCharacterMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterMoveComponent::BaseComponentInitialize()
{
	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}
}

void ULxCharacterMoveComponent::HandleMoveInput(const FVector2D& InMoveValue)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (!m_pOwnerCharacter || !m_pOwnerCharacter->Controller)
	{
		return;
	}

	const FRotator ControlRotation = m_pOwnerCharacter->Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	m_pOwnerCharacter->AddMovementInput(ForwardDirection, InMoveValue.Y);
	m_pOwnerCharacter->AddMovementInput(RightDirection, InMoveValue.X);
}

void ULxCharacterMoveComponent::HandleJumpInput(bool bPressed)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (!m_pOwnerCharacter)
	{
		return;
	}

	if (bPressed)
	{
		m_pOwnerCharacter->Jump();
	}
	else
	{
		m_pOwnerCharacter->StopJumping();
	}
}
