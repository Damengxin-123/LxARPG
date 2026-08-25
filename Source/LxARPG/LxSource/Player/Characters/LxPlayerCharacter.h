#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxPlayerCharacter.generated.h"

class UCameraComponent;
class ULxInteractableComponent;
class ULxPlayerAimModule;
class ULxPlayerControlComponent;
class ULxPlayerInteractionModule;
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
	ULxPlayerInteractionModule* GetPlayerInteractionComponent() const;

	/** 获取玩家瞄准组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|瞄准", DisplayName="获取玩家瞄准组件")
	ULxPlayerAimModule* GetPlayerAimComponent() const;

	/** 获取统一管理玩家移动、瞄准和交互模块的玩家操控组件。 */
	UFUNCTION(BlueprintCallable, Category="组件|玩家操控", DisplayName="获取玩家操控组件")
	ULxPlayerControlComponent* GetPlayerControlComponent() const { return m_pPlayerControlComponent; }

	virtual void ReceiveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;
	virtual void RemoveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;

protected:
	/** 玩家操控组件，统一持有移动输入、瞄准和交互模块。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|玩家操控", DisplayName="玩家操控组件")
	TObjectPtr<ULxPlayerControlComponent> m_pPlayerControlComponent;

	/** 相机弹簧臂组件，用于控制跟随相机距离和旋转继承。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="相机弹簧臂")
	TObjectPtr<USpringArmComponent> m_pCameraBoom;

	/** 跟随相机组件，用于提供玩家角色视角。 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="跟随相机")
	TObjectPtr<UCameraComponent> m_pFollowCamera;
};
