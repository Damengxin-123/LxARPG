// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxComponentBase.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxCharacterComponentBase.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXARPG_API ULxCharacterComponentBase : public ULxComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULxCharacterComponentBase();

protected:
	ALxBaseCharacter* GetCharacterOwner() const
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
	
};
