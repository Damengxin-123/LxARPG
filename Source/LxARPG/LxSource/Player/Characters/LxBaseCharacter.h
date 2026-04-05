#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LxCharacterStateEnum.h"
#include "LxBaseCharacter.generated.h"

class ULxCharacterAttributeComponent;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;
class ULxCharacterMoveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterStateChange, const ELxCharacterState, State);

UCLASS(Blueprintable)
class LXARPG_API ALxBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ALxBaseCharacter();

	/**
	 * @brief 初始化角色运行时信息。
	 *
	 * 会初始化角色身上的核心组件，并同步初始状态数据。
	 */
	virtual void InitialCharacterInformation();

	/**
	 * @brief 设置角色当前状态。
	 *
	 * @param InState 要切换到的新角色状态。
	 */
	virtual void SetCharacterState(const ELxCharacterState InState);

	/**
	 * @brief 获取角色当前状态。
	 *
	 * @return 返回角色当前状态枚举值。
	 */
	virtual const ELxCharacterState GetCurrentState();

	UPROPERTY(BlueprintAssignable)
	FOnCharacterStateChange OnCharacterStateChange;

protected:
	/**
	 * @brief 角色进入游戏时触发。
	 */
	virtual void BeginPlay() override;

public:
	/**
	 * @brief 角色每帧更新。
	 *
	 * @param DeltaTime 当前帧与上一帧之间的时间差。
	 */
	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief 绑定角色输入组件。
	 *
	 * @param PlayerInputComponent 当前角色可用的输入组件。
	 */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable, Category="Character|Movement")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Attribute")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Item")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	UFUNCTION(BlueprintCallable, Category="Character|Item")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Movement")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Attribute")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Item")
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Components|Item")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;

	UPROPERTY(EditAnywhere)
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;
};
