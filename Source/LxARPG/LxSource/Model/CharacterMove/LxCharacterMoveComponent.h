#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxCharacterMoveComponent.generated.h"

class ALxBaseCharacter;

/**
 * 角色移动组件，封装移动和跳跃行为。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色移动组件")
class LXARPG_API ULxCharacterMoveComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterMoveComponent();

	/**
	 * @brief 初始化角色移动组件。
	 *
	 * 负责缓存所属角色引用，供后续移动、跳跃和视角控制使用。
	 */
	virtual void BaseComponentInitialize() override;

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色移动")
	/**
	 * @brief 处理角色移动输入。
	 *
	 * @param InMoveValue 二维移动输入值，通常分别表示前后和左右方向。
	 */
	void HandleMoveInput(const FVector2D& InMoveValue);

	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="角色跳跃")
	/**
	 * @brief 处理角色跳跃输入。
	 *
	 * @param bPressed 为 true 表示开始跳跃，为 false 表示结束跳跃输入。
	 */
	void HandleJumpInput(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="角色|视角", DisplayName="角色视角")
	/**
	 * @brief 处理角色视角输入。
	 *
	 * @param InMoveValue 二维视角输入值，通常分别表示水平和垂直旋转。
	 */
	void HandleLookInput(const FVector2D& InMoveValue);

	/** 增加移动转向锁，锁定期间移动输入仍然生效，但不会由移动方向旋转角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="增加移动转向锁")
	void AddMoveRotationLock();

	/** 移除移动转向锁，所有锁移除后恢复移动方向旋转角色。 */
	UFUNCTION(BlueprintCallable, Category="角色|移动", DisplayName="移除移动转向锁")
	void RemoveMoveRotationLock();

	/** 判断移动输入是否允许旋转角色。 */
	UFUNCTION(BlueprintPure, Category="角色|移动", DisplayName="是否允许移动转向")
	bool CanRotateByMoveInput() const { return MoveRotationLockCount <= 0; }

	
private:
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 移动转向锁计数，瞄准和技能释放可同时锁定，全部释放后才恢复。 */
	UPROPERTY(Transient)
	int32 MoveRotationLockCount = 0;
};
