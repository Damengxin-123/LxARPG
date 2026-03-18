#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxPlayerCharacter.generated.h"

class UCameraComponent;
class ULxPlayerControlMoveComponent;
class USpringArmComponent;

UCLASS(Blueprintable, DisplayName="玩家角色")
class LXARPG_API ALxPlayerCharacter : public ALxBaseCharacter
{
	GENERATED_BODY()

public:
	ALxPlayerCharacter();

	virtual void InitialCharacterInformation() override;

	UFUNCTION(BlueprintCallable, Category="角色|相机")
	USpringArmComponent* GetCameraBoom() const { return m_pCameraBoom; }

	UFUNCTION(BlueprintCallable, Category="角色|相机")
	UCameraComponent* GetFollowCamera() const { return m_pFollowCamera; }

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|输入", DisplayName="玩家移动控制组件")
	TObjectPtr<ULxPlayerControlMoveComponent> m_pPlayerControlMoveComponent;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="弹簧臂")
	TObjectPtr<USpringArmComponent> m_pCameraBoom;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="组件|相机", DisplayName="跟随相机")
	TObjectPtr<UCameraComponent> m_pFollowCamera;
};
