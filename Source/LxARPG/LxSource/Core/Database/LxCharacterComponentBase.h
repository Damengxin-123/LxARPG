// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxComponentBase.h"
#include "LxCharacterComponentBase.generated.h"

class ALxBaseCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色组件基类")
class LXARPG_API ULxCharacterComponentBase : public ULxComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULxCharacterComponentBase(){};

protected:
	ALxBaseCharacter* GetCharacterOwner() const;
};
