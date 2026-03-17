// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterMoveComponent;

UCLASS(Blueprintable, DisplayName="基础角色")
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALxBaseCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category="角色|移动")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|移动", DisplayName="角色移动组件")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;
};
