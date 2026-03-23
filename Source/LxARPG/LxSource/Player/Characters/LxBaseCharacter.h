// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxCharacterStateEnum.h"
#include "GameFramework/Character.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterMoveComponent;

// 用于通知蓝图的委托，用于告知此角色的状态已经发生改变
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

/**
 * @brief 基础角色类，用于创建游戏中的基础角色。
 *
 * 该类继承自ACharacter，并提供了一些额外的功能，如角色状态设置、角色移动组件的获取等。
 * 通过蓝图可配置，显示名称为"基础角色"。
 */
UCLASS(Blueprintable, DisplayName="基础角色")
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ALxBaseCharacter();
	// 初始化角色逻辑信息，适用于在引擎的初始化逻辑走完之后，再进行初始化
	virtual void InitialCharacterInformation() {};

	virtual void SetCharacterState(const ELxCharacterState InState);

	virtual const ELxCharacterState GetCurrentState();

	UPROPERTY(BlueprintAssignable, DisplayName="当前角色状态发生改变")
	FOnCharacterStateChange OnCharacterStateChange;


	
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

	UPROPERTY(EditAnywhere, DisplayName="当前角色的状态")
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;
};
