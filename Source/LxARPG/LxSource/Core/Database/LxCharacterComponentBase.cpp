// Fill out your copyright notice in the Description page of Project Settings.

#include "LxCharacterComponentBase.h"

#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ALxBaseCharacter* ULxCharacterComponentBase::GetCharacterOwner() const
{
	AActor* OwnerActor = GetOwner();
	while (OwnerActor)
	{
		if (ALxBaseCharacter* OwnerCharacter = Cast<ALxBaseCharacter>(OwnerActor))
		{
			return OwnerCharacter;
		}

		OwnerActor = OwnerActor->GetAttachParentActor();
	}

	return nullptr;
};
