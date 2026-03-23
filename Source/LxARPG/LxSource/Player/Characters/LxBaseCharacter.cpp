// Fill out your copyright notice in the Description page of Project Settings.

#include "LxBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"

ALxBaseCharacter::ALxBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("角色移动组件"));
	m_pCharacterAttributeComponent = CreateDefaultSubobject<ULxCharacterAttributeComponent>(TEXT("角色属性组件"));
}

void ALxBaseCharacter::InitialCharacterInformation()
{
	if (m_pCharacterMoveComponent)
	{
		m_pCharacterMoveComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->BaseComponentInitialize();
	}
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
