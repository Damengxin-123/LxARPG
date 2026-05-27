#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxPlayerCharacter.generated.h"

class UCameraComponent;
class ULxInteractableComponent;
class ULxPlayerAimComponent;
class ULxPlayerControlMoveComponent;
class ULxPlayerInteractionComponent;
class USpringArmComponent;

/** 玩家角色，持有玩家移动、相机和交互管理组件。 */
UCLASS(Blueprintable, DisplayName="玩家角色")
class LXARPG_API ALxPlayerCharacter : public ALxBaseCharacter, public ILxInteractionReceiverInterface
{
	GENERATED_BODY()

public:
	/** 创建玩家角色对象，并初始化玩家专属组件。 */
	ALxPlayerCharacter();

	/** 初始化玩家角色专属组件。 */
	virtual void InitialCharacterInformation() override;

	UFUNCTION(BlueprintCallable, Category="角色|相机", DisplayName="获取相机弹簧臂")
	USpringArmComponent* GetCameraBoom() const { return m_pCameraBoom; }

	UFUNCTION(BlueprintCallable, Category="角色|相机", DisplayName="获取跟随相机")
	UCameraComponent* GetFollowCamera() const { return m_pFollowCamera; }

	UFUNCTION(BlueprintCallable, Category="组件|交互", DisplayName="获取玩家交互组件")
	ULxPlayerInteractionComponent* GetPlayerInteractionComponent() const { return m_pPlayerInteractionComponent; }

	/** 获取玩家瞄准组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|瞄准", DisplayName="获取玩家瞄准组件")
	ULxPlayerAimComponent* GetPlayerAimComponent() const { return m_pPlayerAimComponent; }

	virtual void ReceiveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;
	virtual void RemoveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;

protected:
	/** 玩家移动控制组件，用于处理本地玩家输入驱动的移动。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|输入", DisplayName="玩家移动控制组件")
	TObjectPtr<ULxPlayerControlMoveComponent> m_pPlayerControlMoveComponent;

	/** 玩家交互组件，用于收集和处理当前可交互对象。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|交互", DisplayName="玩家交互组件")
	TObjectPtr<ULxPlayerInteractionComponent> m_pPlayerInteractionComponent;

	/** 玩家瞄准组件，用于处理准星检测、瞄准相机和瞄准时角色转向。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|瞄准", DisplayName="玩家瞄准组件")
	TObjectPtr<ULxPlayerAimComponent> m_pPlayerAimComponent;

	/** 相机弹簧臂组件，用于控制跟随相机距离和旋转继承。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="相机弹簧臂")
	TObjectPtr<USpringArmComponent> m_pCameraBoom;

	/** 跟随相机组件，用于提供玩家角色视角。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="跟随相机")
	TObjectPtr<UCameraComponent> m_pFollowCamera;
};
