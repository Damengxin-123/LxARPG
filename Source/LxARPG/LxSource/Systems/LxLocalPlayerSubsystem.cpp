// Fill out your copyright notice in the Description page of Project Settings.


#include "LxLocalPlayerSubsystem.h"

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
}

void ULxLocalPlayerSubsystem::UnregisterInputReceive(FName InInputName)
{
	if (!m_pInputComponentQuote)
	{
		return;
	}
}

void ULxLocalPlayerSubsystem::SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote)
{
	if (InUInputComponentQuote)
	{
		m_pInputComponentQuote = InUInputComponentQuote;
	}
}
