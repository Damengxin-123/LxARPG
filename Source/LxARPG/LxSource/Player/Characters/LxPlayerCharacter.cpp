#include "LxPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerAimModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterFactionAttributeObject.h"

ALxPlayerCharacter::ALxPlayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	m_pPlayerControlComponent = CreateDefaultSubobject<ULxPlayerControlComponent>(TEXT("玩家操控组件"));

	m_pCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("相机弹簧臂"));
	m_pCameraBoom->SetupAttachment(RootComponent);
	m_pCameraBoom->TargetArmLength = 360.f;
	m_pCameraBoom->bUsePawnControlRotation = true;
	m_pCameraBoom->bInheritPitch = true;
	m_pCameraBoom->bInheritYaw = true;
	m_pCameraBoom->bInheritRoll = true;

	m_pFollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("跟随相机"));
	m_pFollowCamera->SetupAttachment(m_pCameraBoom, USpringArmComponent::SocketName);
	m_pFollowCamera->bUsePawnControlRotation = false;
}

ULxPlayerInteractionModule* ALxPlayerCharacter::GetPlayerInteractionComponent() const
{
	return m_pPlayerControlComponent ? m_pPlayerControlComponent->GetInteractionModule() : nullptr;
}

ULxPlayerAimModule* ALxPlayerCharacter::GetPlayerAimComponent() const
{
	return m_pPlayerControlComponent ? m_pPlayerControlComponent->GetAimModule() : nullptr;
}

void ALxPlayerCharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();

	// 旧玩家蓝图没有保存阵营标签时补齐默认市民阵营，确保敌方过滤可以识别怪物角色。
	if (m_pCharacterAttributeComponent)
	{
		ULxCharacterFactionAttributeObject* FactionObject =
			m_pCharacterAttributeComponent->GetFactionAttributeObject();
		if (FactionObject && FactionObject->IsFactionConfigurationEmpty())
		{
			FLxCharacterFactionData DefaultPlayerFaction;
			DefaultPlayerFaction.FriendlyTags.AddTag(
				FGameplayTag::RequestGameplayTag(FName(TEXT("阵营.种类.市民"))));
			DefaultPlayerFaction.HostileTags.AddTag(
				FGameplayTag::RequestGameplayTag(FName(TEXT("阵营.种类.怪物"))));
			FactionObject->SetCharacterFactionData(DefaultPlayerFaction);
		}
	}

	if (m_pPlayerControlComponent)
	{
		m_pPlayerControlComponent->BaseComponentInitialize();
	}
}

void ALxPlayerCharacter::ReceiveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent)
{
	if (ULxPlayerInteractionModule* PlayerInteractionModule = GetPlayerInteractionComponent())
	{
		PlayerInteractionModule->AddInteractableComponent(InInteractableComponent);
	}
}

void ALxPlayerCharacter::RemoveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent)
{
	if (ULxPlayerInteractionModule* PlayerInteractionModule = GetPlayerInteractionComponent())
	{
		PlayerInteractionModule->RemoveInteractableComponent(InInteractableComponent);
	}
}
