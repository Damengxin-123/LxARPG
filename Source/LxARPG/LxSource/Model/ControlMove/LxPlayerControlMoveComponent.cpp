#include "LxPlayerControlMoveComponent.h"

#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	bool IsControlledByLocalPlayer(const ALxBaseCharacter* InCharacter)
	{
		const APlayerController* PlayerController = InCharacter
			? Cast<APlayerController>(InCharacter->GetController())
			: nullptr;
		return PlayerController && PlayerController->GetLocalPlayer();
	}
}

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
	if (!m_pBehaviorControlComponent && m_pOwnerCharacter)
	{
		m_pBehaviorControlComponent = m_pOwnerCharacter->GetCharacterBehaviorControlComponent();
	}
	if (!IsControlledByLocalPlayer(m_pOwnerCharacter))
	{
		UnregisterAllInputActionReceives();
		return;
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
	if (!m_pBehaviorControlComponent)
	{
		BaseComponentInitialize();
	}
	if (!m_pBehaviorControlComponent)
	{
		return;
	}
	if (!IsControlledByLocalPlayer(m_pOwnerCharacter))
	{
		return;
	}

	if (InInputActionID == m_MoveWInputActionID || InInputActionID == m_MoveAInputActionID
		|| InInputActionID == m_MoveDInputActionID || InInputActionID == m_MoveSInputActionID)
	{
		m_pBehaviorControlComponent->HandleMoveInput(InValue.m_sVector2D);
	}
	else if (InInputActionID == m_JumpInputActionID)
	{
		m_pBehaviorControlComponent->HandleJumpInput(InValue.m_blValue);
	}
	else if (InInputActionID == m_LookXInputActionID || InInputActionID == m_LookYInputActionID)
	{
		m_pBehaviorControlComponent->HandleLookInput(InValue.m_sVector2D);
	}
}
