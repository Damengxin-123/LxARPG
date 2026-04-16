#include "LxBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"

ALxBaseCharacter::ALxBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("CharacterMoveComponent"));
	m_pCharacterAttributeComponent = CreateDefaultSubobject<ULxCharacterAttributeComponent>(TEXT("CharacterAttributeComponent"));
	m_pCharacterBackpackComponent = CreateDefaultSubobject<ULxCharacterBackpackComponent>(TEXT("CharacterBackpackComponent"));
	m_pCharacterEquipmentComponent = CreateDefaultSubobject<ULxCharacterEquipmentComponent>(TEXT("CharacterEquipmentComponent"));
}

void ALxBaseCharacter::InitialCharacterInformation()
{
	if (IsInitialized)
	{
		return;
	}
	if (m_pCharacterMoveComponent)
	{
		m_pCharacterMoveComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->BaseComponentInitialize();
	}

	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->BaseComponentInitialize();
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->BaseComponentInitialize();
	}
	IsInitialized  = true;
}

void ALxBaseCharacter::SetCharacterState(const ELxCharacterState InState)
{
	OnCharacterStateChange.Broadcast(InState);
	m_nCharacterState = InState;
}

const ELxCharacterState ALxBaseCharacter::GetCurrentState()
{
	return m_nCharacterState;
}

void ALxBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ALxBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (GetCharacterMovement()->Velocity.X + GetCharacterMovement()->Velocity.Y <= 0)
	{
		if (m_nCharacterState == ELxCharacterState::Moving ||
			m_nCharacterState == ELxCharacterState::JumpEnd)
		{
			SetCharacterState(ELxCharacterState::Idle);
		}
	}
}

void ALxBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
