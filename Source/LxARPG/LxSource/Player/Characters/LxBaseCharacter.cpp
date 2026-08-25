#include "LxBaseCharacter.h"

#include "LxCharacterIDTags.h"
#include "LxCharacterNameTags.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationProcessComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/CloseCombat/Logic/LxCharacterCloseCombatComponent.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectCacheComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectProcessComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectTransferComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterEntryPackage.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
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
	CharacterIDTag = LxTag_CharacterID_DefaultCharacter;
	CharacterNameIDTag = LxTag_UnitNaming_DefaultNaming;
	m_pCharacterBehaviorControlComponent = CreateDefaultSubobject<ULxCharacterBehaviorControlComponent>(TEXT("角色行为控制组件"));
	m_pCharacterAnimationProcessComponent = CreateDefaultSubobject<ULxCharacterAnimationProcessComponent>(TEXT("CharacterAnimationProcessComponent"));
	m_pCharacterContentComponent = CreateDefaultSubobject<ULxCharacterContentComponent>(TEXT("角色内容组件"));
	m_pCharacterAttributeComponent = CreateDefaultSubobject<ULxCharacterAttributeComponent>(TEXT("CharacterAttributeComponent"));
	m_pCharacterSpecialAttributeComponent = m_pCharacterAttributeComponent;
	m_pCharacterEffectComponent = CreateDefaultSubobject<ULxCharacterEffectComponent>(TEXT("角色效果组件"));
	m_pCharacterEffectProcessComponent = m_pCharacterEffectComponent;
	m_pCharacterDataTransferComponent = CreateDefaultSubobject<ULxCharacterDataTransferComponent>(TEXT("CharacterDataTransferComponent"));
	m_pCharacterTestComponent = CreateDefaultSubobject<ULxCharacterTestComponent>(TEXT("CharacterTestComponent"));
	m_pCharacterCombatComponent = CreateDefaultSubobject<ULxCharacterCombatComponent>(TEXT("角色战斗组件"));
	m_pSkillCastComponent = m_pCharacterCombatComponent;
	m_pCharacterCloseCombatComponent = m_pCharacterCombatComponent;
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

ULxCharacterBackpackModule* ALxBaseCharacter::GetCharacterBackpackComponent() const
{
	return m_pCharacterContentComponent ? m_pCharacterContentComponent->GetBackpackModule() : nullptr;
}

ULxCharacterBuffModule* ALxBaseCharacter::GetCharacterBuffComponent() const
{
	return m_pCharacterContentComponent ? m_pCharacterContentComponent->GetBuffModule() : nullptr;
}

ULxCharacterEquipmentModule* ALxBaseCharacter::GetCharacterEquipmentComponent() const
{
	return m_pCharacterContentComponent ? m_pCharacterContentComponent->GetEquipmentModule() : nullptr;
}

ULxSkillBackpackModule* ALxBaseCharacter::GetSkillBackpackComponent() const
{
	return m_pCharacterContentComponent ? m_pCharacterContentComponent->GetSkillBackpackModule() : nullptr;
}

ULxCharacterProfessionModule* ALxBaseCharacter::GetCharacterProfessionComponent() const
{
	return m_pCharacterContentComponent ? m_pCharacterContentComponent->GetProfessionModule() : nullptr;
}

ULxCharacterEffectProcessModule* ALxBaseCharacter::GetCharacterEffectProcessComponent() const
{
	return m_pCharacterEffectComponent ? m_pCharacterEffectComponent->GetProcessModule() : nullptr;
}

ULxCharacterEffectCacheModule* ALxBaseCharacter::GetCharacterEffectCacheComponent() const
{
	return m_pCharacterEffectComponent ? m_pCharacterEffectComponent->GetCacheModule() : nullptr;
}

ULxCharacterEffectTransferModule* ALxBaseCharacter::GetCharacterEffectTransferComponent() const
{
	return m_pCharacterEffectComponent ? m_pCharacterEffectComponent->GetTransferModule() : nullptr;
}

ULxSkillCastModule* ALxBaseCharacter::GetSkillCastComponent() const
{
	return m_pCharacterCombatComponent ? m_pCharacterCombatComponent->GetSkillCastModule() : nullptr;
}

ULxCharacterCloseCombatModule* ALxBaseCharacter::GetCharacterCloseCombatComponent() const
{
	return m_pCharacterCombatComponent ? m_pCharacterCombatComponent->GetCloseCombatModule() : nullptr;
}

FGameplayTag ALxBaseCharacter::GetCharacterIDTag() const
{
	// 原生标签可能晚于角色类默认对象完成注册，因此未配置时在运行期返回已注册的默认角色标签。
	return CharacterIDTag.IsValid() ? CharacterIDTag : LxTag_CharacterID_DefaultCharacter.GetTag();
}

void ALxBaseCharacter::InitialCharacterInformation()
{
	if (IsInitialized)
	{
		return;
	}

	InitializeCharacterNamingText();
	if (m_pCharacterAnimationProcessComponent)
	{
		m_pCharacterAnimationProcessComponent->BaseComponentInitialize();
	}

	// 动画处理组件先绑定行为事件，避免行为组件初始化时发送的首个基础运动信号丢失。
	if (m_pCharacterBehaviorControlComponent)
	{
		m_pCharacterBehaviorControlComponent->BaseComponentInitialize();
	}

	if (m_pCharacterContentComponent)
	{
		m_pCharacterContentComponent->BaseComponentInitialize();
	}

	if (m_pCharacterAttributeComponent)
	{
		m_pCharacterAttributeComponent->BaseComponentInitialize();
	}

	if (m_pCharacterEffectComponent)
	{
		m_pCharacterEffectComponent->BaseComponentInitialize();
	}

	if (m_pCharacterDataTransferComponent)
	{
		m_pCharacterDataTransferComponent->BaseComponentInitialize();
		ApplyDefaultEntryConfig();
	}

	if (m_pCharacterTestComponent)
	{
		m_pCharacterTestComponent->BaseComponentInitialize();
	}

	if (m_pCharacterCombatComponent)
	{
		m_pCharacterCombatComponent->BaseComponentInitialize();
	}
	IsInitialized  = true;
}

void ALxBaseCharacter::ApplyDefaultEntryConfig()
{
	if (!HasAuthority() || m_pCharacterDataTransferComponent == nullptr || DefaultEntryConfig.IsEmpty())
	{
		return;
	}

	FLxCharacterEntryPackage DefaultEntryPackage;
	DefaultEntryPackage.EntrySource = ELxCharacterEntrySource::CharacterDefault;
	DefaultEntryPackage.EntryList.Reserve(DefaultEntryConfig.Num());

	for (const FLxEntryQuote& EntryQuote : DefaultEntryConfig)
	{
		if (ULxEntryObjectBase* EntryObject = ULxEntryObjectBase::CreateEnterObject(m_pCharacterDataTransferComponent, EntryQuote))
		{
			DefaultEntryPackage.EntryList.Add(EntryObject);
		}
	}

	m_pCharacterDataTransferComponent->ReceiveEntryPackage(DefaultEntryPackage);
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
}

void ALxBaseCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}
