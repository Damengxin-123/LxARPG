#include "LxPlayerControlMoveComponent.h"

#include "GameFramework/PlayerController.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Components/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"

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

	if (!m_pLocalPlayerSubsystem && m_pOwnerCharacter && m_pOwnerCharacter->Controller)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(m_pOwnerCharacter->Controller))
		{
			if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				m_pLocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(LocalPlayer);
			}
		}
	}
}

void ULxPlayerControlMoveComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseComponentInitialize();
	InitMonitorRegistration();
}

void ULxPlayerControlMoveComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterMonitor();
	Super::EndPlay(EndPlayReason);
}

void ULxPlayerControlMoveComponent::HandleInputValue(FName InName, FLxInputValue InValue)
{
	if (!m_pMoveComponent)
	{
		BaseComponentInitialize();
	}
	if (!m_pMoveComponent)
	{
		return;
	}

	if (InName == m_MoveWInputActionID || InName == m_MoveAInputActionID
		|| InName == m_MoveDInputActionID || InName == m_MoveSInputActionID)
	{
		m_pMoveComponent->HandleMoveInput(InValue.m_sVector2D);
	}
	else if (InName == m_JumpInputActionID)
	{
		m_pMoveComponent->HandleJumpInput(InValue.m_blValue);
	}
}

void ULxPlayerControlMoveComponent::InitMonitorRegistration()
{
	if (!m_pLocalPlayerSubsystem)
	{
		BaseComponentInitialize();
	}
	if (!m_pLocalPlayerSubsystem)
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_MoveWInputActionID, InputReceive);
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_MoveAInputActionID, InputReceive);
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_MoveSInputActionID, InputReceive);
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_MoveDInputActionID, InputReceive);
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_JumpInputActionID, InputReceive);
}

void ULxPlayerControlMoveComponent::UnregisterMonitor()
{
	if (!m_pLocalPlayerSubsystem)
	{
		return;
	}

	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_MoveWInputActionID);
	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_MoveAInputActionID);
	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_MoveSInputActionID);
	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_MoveDInputActionID);
	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_JumpInputActionID);
}
