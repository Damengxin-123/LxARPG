#include "LxBaseCharacter.h"

#include "LxCharacterNameTags.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationMotionAnalysisComponent.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Model/CloseCombat/Logic/LxCharacterCloseCombatComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectCacheComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectTransferComponent.h"
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
	CharacterNameIDTag = LxTag_UnitNaming_DefaultNaming;
	m_pCharacterMoveComponent = CreateDefaultSubobject<ULxCharacterMoveComponent>(TEXT("CharacterMoveComponent"));
	m_pCharacterAnimationMotionAnalysisComponent = CreateDefaultSubobject<ULxCharacterAnimationMotionAnalysisComponent>(TEXT("CharacterAnimationMotionAnalysisComponent"));
	m_pCharacterAnimationProcessComponent = CreateDefaultSubobject<ULxCharacterAnimationProcessComponent>(TEXT("CharacterAnimationProcessComponent"));
	m_pCharacterBackpackComponent = CreateDefaultSubobject<ULxCharacterBackpackComponent>(TEXT("CharacterBackpackComponent"));
	m_pCharacterEquipmentComponent = CreateDefaultSubobject<ULxCharacterEquipmentComponent>(TEXT("CharacterEquipmentComponent"));
	m_pCharacterBuffComponent = CreateDefaultSubobject<ULxCharacterBuffComponent>(TEXT("CharacterBuffComponent"));
	m_pCharacterStateComponent = CreateDefaultSubobject<ULxCharacterStateComponent>(TEXT("CharacterStateComponent"));
	m_pCharacterAttributeComponent = CreateDefaultSubobject<ULxCharacterAttributeComponent>(TEXT("CharacterAttributeComponent"));
	m_pCharacterLifecycleComponent = CreateDefaultSubobject<ULxCharacterLifecycleComponent>(TEXT("CharacterLifecycleComponent"));
	m_pCharacterEffectProcessComponent = CreateDefaultSubobject<ULxCharacterEffectProcessComponent>(TEXT("CharacterEffectProcessComponent"));
	m_pCharacterEffectCacheComponent = CreateDefaultSubobject<ULxCharacterEffectCacheComponent>(TEXT("CharacterEffectCacheComponent"));
	m_pCharacterEffectTransferComponent = CreateDefaultSubobject<ULxCharacterEffectTransferComponent>(TEXT("CharacterEffectTransferComponent"));
	m_pSkillBackpackComponent = CreateDefaultSubobject<ULxSkillBackpackComponent>(TEXT("SkillBackpackComponent"));
	m_pCharacterProfessionComponent = CreateDefaultSubobject<ULxCharacterProfessionComponent>(TEXT("CharacterProfessionComponent"));
	m_pCharacterDataTransferComponent = CreateDefaultSubobject<ULxCharacterDataTransferComponent>(TEXT("CharacterDataTransferComponent"));
	m_pCharacterTestComponent = CreateDefaultSubobject<ULxCharacterTestComponent>(TEXT("CharacterTestComponent"));
	m_pSkillCastComponent = CreateDefaultSubobject<ULxSkillCastComponent>(TEXT("SkillCastComponent"));
	m_pCharacterCloseCombatComponent = CreateDefaultSubobject<ULxCharacterCloseCombatComponent>(TEXT("CharacterCloseCombatComponent"));
	m_pSkillReleaseAnchorPoint = CreateDefaultSubobject<ULxCharacterAnchorPointComponent>(TEXT("SkillReleaseAnchorPoint"));
	m_pSkillReleaseAnchorPoint->SetupAttachment(GetRootComponent());
	m_pSkillReleaseAnchorPoint->SetRelativeLocation(FVector(80.f, 0.f, 60.f));
	m_pSkillReleaseAnchorPoint->SetAnchorPointType(ELxCharacterAnchorPointType::SkillRelease);
	m_pAuraEffectAnchorPoint = CreateDefaultSubobject<ULxCharacterAnchorPointComponent>(TEXT("AuraEffectAnchorPoint"));
	m_pAuraEffectAnchorPoint->SetupAttachment(GetRootComponent());
	m_pAuraEffectAnchorPoint->SetAnchorPointType(ELxCharacterAnchorPointType::AuraEffect);
}

FTransform ALxBaseCharacter::GetSkillReleaseAnchorTransform() const
{
	return m_pSkillReleaseAnchorPoint->GetComponentTransform();
}

void ALxBaseCharacter::InitialCharacterInformation()
{
	if (IsInitialized)
	{
		return;
	}

	InitializeCharacterNamingText();
	if (m_pCharacterMoveComponent)
	{
		m_pCharacterMoveComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAnimationMotionAnalysisComponent)
	{
		m_pCharacterAnimationMotionAnalysisComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAnimationProcessComponent)
	{
		m_pCharacterAnimationProcessComponent->BaseComponentInitialize();
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

	if (m_pCharacterEffectProcessComponent)
	{
		m_pCharacterEffectProcessComponent->BaseComponentInitialize();
	}

	if (m_pCharacterEffectCacheComponent)
	{
		m_pCharacterEffectCacheComponent->BaseComponentInitialize();
	}

	if (m_pCharacterEffectTransferComponent)
	{
		m_pCharacterEffectTransferComponent->BaseComponentInitialize();
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

	if (m_pCharacterCloseCombatComponent)
	{
		m_pCharacterCloseCombatComponent->BaseComponentInitialize();
	}
	IsInitialized  = true;
}


void ALxBaseCharacter::InitializeCharacterNamingText()
{
	CharacterNamingText = FText::GetEmpty();
	if (!CharacterNameIDTag.IsValid())
	{
		CharacterNameIDTag = LxTag_UnitNaming_DefaultNaming;
	}

	if (!CharacterNamingTable)
	{
		return;
	}

	const FString ContextString = TEXT("ALxBaseCharacter::InitializeCharacterNamingText");
	const FLxCharacterNamingRow* NamingRow = CharacterNamingTable->FindRow<FLxCharacterNamingRow>(CharacterNameIDTag.GetTagName(), ContextString, false);
	if (!NamingRow)
	{
		for (const TPair<FName, uint8*>& RowPair : CharacterNamingTable->GetRowMap())
		{
			const FLxCharacterNamingRow* CurrentRow = reinterpret_cast<const FLxCharacterNamingRow*>(RowPair.Value);
			if (CurrentRow && CurrentRow->NamingIDTag == CharacterNameIDTag)
			{
				NamingRow = CurrentRow;
				break;
			}
		}
	}

	if (NamingRow)
	{
		CharacterNamingText = NamingRow->NamingText;
	}
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
			if (m_pCharacterAnimationMotionAnalysisComponent)
			{
				FLxCharacterMotionSignal MotionSignal;
				MotionSignal.MotionType = ELxCharacterMotionType::Idle;
				MotionSignal.MotionDirection = FVector::ZeroVector;
				MotionSignal.MotionSpeed = 0.0f;
				MotionSignal.bLoop = true;
				m_pCharacterAnimationMotionAnalysisComponent->ReceiveBaseMotionEvent(MotionSignal);
			}
		}
	}
}

void ALxBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
