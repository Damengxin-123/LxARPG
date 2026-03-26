// Fill out your copyright notice in the Description page of Project Settings.


#include "LxUIBaseObject.h"

void ULxUIBaseObject::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	m_pPlayerCharacter = PlayerCharacter;
}

void ULxUIBaseObject::UpdatePlayerController(ALxPlayerController* PlayerController)
{
	m_pPlayerController = PlayerController;
}

void ULxUIBaseObject::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeUIComponents();
}

