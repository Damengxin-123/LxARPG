#include "LxPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerAimModule.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"

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
