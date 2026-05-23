#include "LxPlayerSystemOperateComponent.h"

#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"

ULxPlayerSystemOperateComponent::ULxPlayerSystemOperateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxPlayerSystemOperateComponent::BaseComponentInitialize()
{
	if (!m_pPlayerController)
	{
		m_pPlayerController = Cast<ALxPlayerController>(GetOwner());
	}
	if (!m_pPlayerController || !m_pPlayerController->GetLocalPlayer())
	{
		UnregisterAllInputActionReceives();
		return;
	}

	RegisterInputActionReceive(m_ShowMouseCursorInputActionID);
}

void ULxPlayerSystemOperateComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseComponentInitialize();
}

void ULxPlayerSystemOperateComponent::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (InInputActionID != m_ShowMouseCursorInputActionID)
	{
		return;
	}

	if (!m_pPlayerController)
	{
		BaseComponentInitialize();
	}
	if (!m_pPlayerController)
	{
		return;
	}
	if (!m_pPlayerController->GetLocalPlayer())
	{
		return;
	}

	if (InValue.m_blValue)
	{
		m_pPlayerController->ShowCursorFun();
	}
	else
	{
		m_pPlayerController->HideCursorFun();
	}
}
