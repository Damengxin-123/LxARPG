#include "LxPlayerSystemOperateComponent.h"

#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxLocalPlayerSubsystem.h"

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

	if (!m_pLocalPlayerSubsystem && m_pPlayerController)
	{
		if (const ULocalPlayer* LocalPlayer = m_pPlayerController->GetLocalPlayer())
		{
			m_pLocalPlayerSubsystem = ULxLocalPlayerSubsystem::GetFromLocalPlayer(LocalPlayer);
		}
	}
	InitMonitorRegistration();
}

void ULxPlayerSystemOperateComponent::BeginPlay()
{
	Super::BeginPlay();

}

void ULxPlayerSystemOperateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterMonitor();
	Super::EndPlay(EndPlayReason);
}

void ULxPlayerSystemOperateComponent::HandleInputValue(FName InName, FLxInputValue InValue)
{
	if (InName != m_ShowMouseCursorInputActionID)
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
	if (InValue.m_blValue)
	{
		m_pPlayerController->ShowCursorFun();
	}
	else
	{
		m_pPlayerController->HideCursorFun();
	}
}

void ULxPlayerSystemOperateComponent::InitMonitorRegistration()
{
	if (!m_pLocalPlayerSubsystem)
	{
		BaseComponentInitialize();
	}
	if (!m_pLocalPlayerSubsystem)
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));
	m_pLocalPlayerSubsystem->RegisterInputReceive(m_ShowMouseCursorInputActionID, InputReceive);
}

void ULxPlayerSystemOperateComponent::UnregisterMonitor()
{
	if (!m_pLocalPlayerSubsystem)
	{
		return;
	}

	m_pLocalPlayerSubsystem->UnregisterInputReceive(m_ShowMouseCursorInputActionID);
}
