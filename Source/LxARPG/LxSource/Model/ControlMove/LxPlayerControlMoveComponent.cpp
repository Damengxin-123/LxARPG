#include "LxPlayerControlMoveComponent.h"

#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxPlayerControlMoveComponent::ULxPlayerControlMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxPlayerControlMoveComponent::BaseComponentInitialize()
{
	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}
	if (!m_pMoveComponent && m_pOwnerCharacter)
	{
		m_pMoveComponent = m_pOwnerCharacter->GetCharacterMoveComponent();
	}
	RegisterInputActionReceive(m_MoveWInputActionID);
	RegisterInputActionReceive(m_MoveSInputActionID);
	RegisterInputActionReceive(m_MoveAInputActionID);
	RegisterInputActionReceive(m_MoveDInputActionID);
	RegisterInputActionReceive(m_LookXInputActionID);
	RegisterInputActionReceive(m_LookYInputActionID);
	RegisterInputActionReceive(m_JumpInputActionID);
}

void ULxPlayerControlMoveComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseComponentInitialize();
}

void ULxPlayerControlMoveComponent::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (!m_pMoveComponent)
	{
		BaseComponentInitialize();
	}
	if (!m_pMoveComponent)
	{
		return;
	}

	if (InInputActionID == m_MoveWInputActionID || InInputActionID == m_MoveAInputActionID
		|| InInputActionID == m_MoveDInputActionID || InInputActionID == m_MoveSInputActionID)
	{
		m_pMoveComponent->HandleMoveInput(InValue.m_sVector2D);
	}
	else if (InInputActionID == m_JumpInputActionID)
	{
		m_pMoveComponent->HandleJumpInput(InValue.m_blValue);
	}
	else if (InInputActionID == m_LookXInputActionID || InInputActionID == m_LookYInputActionID)
	{
		m_pMoveComponent->HandleLookInput(InValue.m_sVector2D);
	}
}
