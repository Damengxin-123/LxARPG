#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlModuleBase.h"
#include "LxPlayerMoveInputModule.generated.h"

class ALxBaseCharacter;
class ULxCharacterBehaviorControlComponent;

/** 玩家移动输入 UObject 模块，负责将本地输入转换为角色运动请求。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="玩家移动输入模块")
class LXARPG_API ULxPlayerMoveInputModule : public ULxPlayerControlModuleBase
{
	GENERATED_BODY()

public:
	/** 初始化玩家移动输入监听和角色运动引用。 */
	virtual void InitializeModule(ULxPlayerControlComponent* InOwnerComponent) override;

	/** 释放玩家移动输入模块。 */
	virtual void ShutdownModule() override;

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

