#include "LxPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LxARPG/LxSource/Model/ControlMove/LxPlayerControlMoveComponent.h"

ALxPlayerCharacter::ALxPlayerCharacter()
{
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	m_pPlayerControlMoveComponent = CreateDefaultSubobject<ULxPlayerControlMoveComponent>(TEXT("玩家移动控制组件"));
	m_pCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("弹簧臂"));
	m_pCameraBoom->SetupAttachment(RootComponent);
	m_pCameraBoom->TargetArmLength = 360.f;
	m_pCameraBoom->bUsePawnControlRotation = true;
	m_pCameraBoom->bInheritPitch = true;
	m_pCameraBoom->bInheritYaw = true;
	m_pCameraBoom->bInheritRoll = false;

	m_pFollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("跟随相机"));
	m_pFollowCamera->SetupAttachment(m_pCameraBoom, USpringArmComponent::SocketName);
	m_pFollowCamera->bUsePawnControlRotation = false;
}

void ALxPlayerCharacter::InitialCharacterInformation()
{
	Super::InitialCharacterInformation();

	if (m_pPlayerControlMoveComponent)
	{
		m_pPlayerControlMoveComponent->BaseComponentInitialize();
	}
}
