// Fill out your copyright notice in the Description page of Project Settings.


#include "LxLocalPlayerSubsystem.h"

#include "LxARPG/LxSource/Model/Input/LxInputComponent.h"

ULxLocalPlayerSubsystem* ULxLocalPlayerSubsystem::GetFromLocalPlayer(const ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<ULxLocalPlayerSubsystem>();
}

void ULxLocalPlayerSubsystem::RegisterInputReceive(FName InInputName,
                                                   TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}
	m_pInputComponentQuote->RegisterInputReceive(InInputName, InRegisterObj);
}

void ULxLocalPlayerSubsystem::UnregisterInputReceive(FName InInputName)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}
	m_pInputComponentQuote->UnregisterInputReceive(InInputName);
}

void ULxLocalPlayerSubsystem::SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote)
{
	if (InUInputComponentQuote)
	{
		m_pInputComponentQuote = InUInputComponentQuote;
	}
}
