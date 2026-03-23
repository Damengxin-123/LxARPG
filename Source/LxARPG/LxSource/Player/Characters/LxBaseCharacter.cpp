// Fill out your copyright notice in the Description page of Project Settings.


#include "LxBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Components/CharacterComponent/LxCharacterMoveComponent.h"


// Sets default values
ALxBaseCharacter::ALxBaseCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("角色移动组件"));
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

// Called when the game starts or when spawned
void ALxBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
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

// Called to bind functionality to input
void ALxBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
