#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Interaction/Interface/LxInteractionReceiverInterface.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxPlayerCharacter.generated.h"

class UCameraComponent;
class ULxInteractableComponent;
class ULxPlayerControlMoveComponent;
class ULxPlayerInteractionComponent;
class USpringArmComponent;

/** 玩家角色，持有玩家移动、相机和交互管理组件。 */
UCLASS(Blueprintable, DisplayName="玩家角色")
class LXARPG_API ALxPlayerCharacter : public ALxBaseCharacter, public ILxInteractionReceiverInterface
{
	GENERATED_BODY()

public:
	ALxPlayerCharacter();

	/** 初始化玩家角色专属组件。 */
	virtual void InitialCharacterInformation() override;

	UFUNCTION(BlueprintCallable, Category="角色|相机", DisplayName="获取相机弹簧臂")
	USpringArmComponent* GetCameraBoom() const { return m_pCameraBoom; }

	UFUNCTION(BlueprintCallable, Category="角色|相机", DisplayName="获取跟随相机")
	UCameraComponent* GetFollowCamera() const { return m_pFollowCamera; }

	UFUNCTION(BlueprintCallable, Category="组件", DisplayName="获取玩家交互组件")
	ULxPlayerInteractionComponent* GetPlayerInteractionComponent() const { return m_pPlayerInteractionComponent; }

	virtual void ReceiveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;
	virtual void RemoveInteractableComponent_Implementation(ULxInteractableComponent* InInteractableComponent) override;

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|输入", DisplayName="玩家移动控制组件")
	TObjectPtr<ULxPlayerControlMoveComponent> m_pPlayerControlMoveComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|交互", DisplayName="玩家交互组件")
	TObjectPtr<ULxPlayerInteractionComponent> m_pPlayerInteractionComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="相机弹簧臂")
	TObjectPtr<USpringArmComponent> m_pCameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="跟随相机")
	TObjectPtr<UCameraComponent> m_pFollowCamera;
};
