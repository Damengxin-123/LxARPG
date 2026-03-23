// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxCharacterStateEnum.h"
#include "GameFramework/Character.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterMoveComponent;
class ULxCharacterAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

UCLASS(Blueprintable, DisplayName="基础角色")
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALxBaseCharacter();

	virtual void InitialCharacterInformation();

	virtual void SetCharacterState(const ELxCharacterState InState);

	virtual const ELxCharacterState GetCurrentState();

	UPROPERTY(BlueprintAssignable, DisplayName="当前角色状态发生改变")
	FOnCharacterStateChange OnCharacterStateChange;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category="角色|移动")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	UFUNCTION(BlueprintCallable, Category="角色|属性")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|移动", DisplayName="角色移动组件")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|属性", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	UPROPERTY(EditAnywhere, DisplayName="当前角色状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;
};
