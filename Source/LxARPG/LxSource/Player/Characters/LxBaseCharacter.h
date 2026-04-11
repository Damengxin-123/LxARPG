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

	/**
	 * @brief 获取角色移动组件。
	 *
	 * 该方法用于获取当前角色的移动组件实例，以便于访问和操作角色的移动相关功能和数据。
	 *
	 * @return 返回指向ULxCharacterMoveComponent的指针，如果移动组件未被初始化则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取移动组件")
	ULxCharacterMoveComponent* GetCharacterMoveComponent() const { return m_pCharacterMoveComponent; }

	/**
	 * @brief 获取角色属性组件。
	 *
	 * 该方法用于获取当前角色的属性组件实例，以便于访问和操作角色的各项属性数据。
	 *
	 * @return 返回指向ULxCharacterAttributeComponent的指针，如果属性组件未被初始化则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetCharacterAttributeComponent() const { return m_pCharacterAttributeComponent; }

	/**
	 * @brief 获取角色背包组件。
	 *
	 * 该方法用于获取当前角色的背包组件实例，以便于访问背包相关的功能和数据。
	 *
	 * @return 返回指向ULxCharacterBackpackComponent的指针，如果背包组件未被初始化则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色背包组件")
	ULxCharacterBackpackComponent* GetCharacterBackpackComponent() const { return m_pCharacterBackpackComponent; }

	/**
	 * @brief 获取角色装备组件。
	 *
	 * 该方法用于获取当前角色的装备组件实例，以便于访问和操作角色的装备相关功能和数据。
	 *
	 * @return 返回指向ULxCharacterEquipmentComponent的指针，如果装备组件未被初始化则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取角色装备组件")
	ULxCharacterEquipmentComponent* GetCharacterEquipmentComponent() const { return m_pCharacterEquipmentComponent; }

protected:
	/**
	 * @brief 角色移动组件。
	 *
	 * 该属性指向角色的移动组件实例，用于管理和控制角色的移动行为。通过此组件可以访问和修改与角色移动相关的功能和数据。
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色移动组件")
	TObjectPtr<ULxCharacterMoveComponent> m_pCharacterMoveComponent;

	/**
	 * @brief 角色属性组件。
	 *
	 * 该属性指向角色的属性组件实例，用于管理和控制角色的各项属性数据。通过此组件可以访问和修改与角色属性相关的功能和数据。
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> m_pCharacterAttributeComponent;

	/**
	 * @brief 角色背包组件。
	 *
	 * 该属性指向角色的背包组件实例，用于管理和控制角色背包相关的功能和数据。通过此组件可以访问和修改与角色背包相关的功能和数据。
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色背包组件")
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent;

	/**
	 * @brief 角色装备组件。
	 *
	 * 该属性指向角色的装备组件实例，用于管理和控制角色装备相关的功能和数据。通过此组件可以访问和修改与角色装备相关的功能和数据。
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件", DisplayName="角色装备组件")
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere)
	ELxCharacterState m_nCharacterState = ELxCharacterState::Idle;
};
