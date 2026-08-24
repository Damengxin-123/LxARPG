#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxPlayerControlMoveComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterBehaviorControlComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家移动控制组件")
class LXARPG_API ULxPlayerControlMoveComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxPlayerControlMoveComponent();

	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Move Action ID", DisplayName="移动-前")
	ELxInputActionID m_MoveWInputActionID = ELxInputActionID::MoveForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Move Action ID", DisplayName="移动-后")
	ELxInputActionID m_MoveSInputActionID = ELxInputActionID::MoveBackward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Move Action ID", DisplayName="移动-左")
	ELxInputActionID m_MoveAInputActionID = ELxInputActionID::MoveLeft;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Move Action ID", DisplayName="移动-右")
	ELxInputActionID m_MoveDInputActionID = ELxInputActionID::MoveRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Action ID", DisplayName="跳跃")
	ELxInputActionID m_JumpInputActionID = ELxInputActionID::Jump;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Action ID", DisplayName="视角X")
	ELxInputActionID m_LookXInputActionID = ELxInputActionID::LookX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Player Input|Action ID", DisplayName="视角Y")
	ELxInputActionID m_LookYInputActionID = ELxInputActionID::LookY;

private:
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter = nullptr;

	/** 接收玩家输入并执行通用角色行为的控制组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterBehaviorControlComponent> m_pBehaviorControlComponent = nullptr;
};
