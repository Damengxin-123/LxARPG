#include "LxBaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/Damage/Logic/LxCharacterDamageComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Lifecycle/Logic/LxCharacterLifecycleComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillBackpackComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/Skill/LxSkillCastComponent.h"
#include "LxARPG/LxSource/Model/State/Logic/LxCharacterStateComponent.h"
#include "LxARPG/LxSource/Model/Test/Logic/LxCharacterTestComponent.h"
#include "Net/UnrealNetwork.h"

ALxBaseCharacter::ALxBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("CharacterMoveComponent"));
	m_pCharacterBackpackComponent = CreateDefaultSubobject<ULxCharacterBackpackComponent>(TEXT("CharacterBackpackComponent"));
	m_pCharacterEquipmentComponent = CreateDefaultSubobject<ULxCharacterEquipmentComponent>(TEXT("CharacterEquipmentComponent"));
	m_pCharacterBuffComponent = CreateDefaultSubobject<ULxCharacterBuffComponent>(TEXT("CharacterBuffComponent"));
	m_pCharacterStateComponent = CreateDefaultSubobject<ULxCharacterStateComponent>(TEXT("CharacterStateComponent"));
	m_pCharacterAttributeComponent = CreateDefaultSubobject<ULxCharacterAttributeComponent>(TEXT("CharacterAttributeComponent"));
	m_pCharacterLifecycleComponent = CreateDefaultSubobject<ULxCharacterLifecycleComponent>(TEXT("CharacterLifecycleComponent"));
	m_pCharacterDamageComponent = CreateDefaultSubobject<ULxCharacterDamageComponent>(TEXT("CharacterDamageComponent"));
	m_pSkillBackpackComponent = CreateDefaultSubobject<ULxSkillBackpackComponent>(TEXT("SkillBackpackComponent"));
	m_pCharacterProfessionComponent = CreateDefaultSubobject<ULxCharacterProfessionComponent>(TEXT("CharacterProfessionComponent"));
	m_pCharacterDataTransferComponent = CreateDefaultSubobject<ULxCharacterDataTransferComponent>(TEXT("CharacterDataTransferComponent"));
	m_pCharacterTestComponent = CreateDefaultSubobject<ULxCharacterTestComponent>(TEXT("CharacterTestComponent"));
	m_pSkillCastComponent = CreateDefaultSubobject<ULxSkillCastComponent>(TEXT("SkillCastComponent"));
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

	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->BaseComponentInitialize();
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->BaseComponentInitialize();
	}

	if (m_pCharacterBuffComponent)
	{
		m_pCharacterBuffComponent->BaseComponentInitialize();
	}

	if (m_pCharacterStateComponent)
	{
		m_pCharacterStateComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->BaseComponentInitialize();
	}

	if (m_pCharacterLifecycleComponent)
	{
		m_pCharacterLifecycleComponent->BaseComponentInitialize();
	}

	if (m_pCharacterDamageComponent)
	{
		m_pCharacterDamageComponent->BaseComponentInitialize();
	}

	if (m_pSkillBackpackComponent)
	{
		m_pSkillBackpackComponent->BaseComponentInitialize();
	}

	if (m_pCharacterProfessionComponent)
	{
		m_pCharacterProfessionComponent->BaseComponentInitialize();
	}
	
	if (m_pCharacterDataTransferComponent)
	{
		m_pCharacterDataTransferComponent->BaseComponentInitialize();
	}

	if (m_pCharacterTestComponent)
	{
		m_pCharacterTestComponent->BaseComponentInitialize();
	}

	if (m_pSkillCastComponent)
	{
		m_pSkillCastComponent->BaseComponentInitialize();
	}
	IsInitialized  = true;
}

void ALxBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALxBaseCharacter, m_nCharacterState);
}

void ALxBaseCharacter::SetCharacterState(const ELxCharacterState InState)
{
	if (m_nCharacterState == InState)
	{
		return;
	}

	m_nCharacterState = InState;
	OnCharacterStateChange.Broadcast(m_nCharacterState);

	if (!HasAuthority() && IsLocallyControlled())
	{
		ServerSetCharacterState(InState);
	}
}

const ELxCharacterState ALxBaseCharacter::GetCurrentState()
{
	return m_nCharacterState;
}

void ALxBaseCharacter::ServerSetCharacterState_Implementation(ELxCharacterState InState)
{
	SetCharacterState(InState);
}

void ALxBaseCharacter::ServerSetCharacterRotation_Implementation(FRotator InRotation)
{
	InRotation.Pitch = 0.0f;
	InRotation.Roll = 0.0f;
	SetActorRotation(InRotation);
}

void ALxBaseCharacter::OnRep_CharacterState()
{
	OnCharacterStateChange.Broadcast(m_nCharacterState);
}

void ALxBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitialCharacterInformation();
}

void ALxBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!HasAuthority() && !IsLocallyControlled())
	{
		return;
	}

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
