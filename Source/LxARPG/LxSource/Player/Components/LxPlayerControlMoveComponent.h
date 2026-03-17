#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/LxInputReceiveInterface.h"
#include "LxPlayerControlMoveComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterMoveComponent;
class ULxLocalPlayerSubsystem;

/**
 * 玩家移动控制组件：负责注册输入监听并将输入值转发给角色移动组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家移动控制组件")
class LXARPG_API ULxPlayerControlMoveComponent : public ULxComponentBase, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	ULxPlayerControlMoveComponent();

	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;
	virtual void InitMonitorRegistration() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动", DisplayName="移动-前")
	FName m_MoveWInputActionID = TEXT("Move-W");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动", DisplayName="移动-后")
	FName m_MoveSInputActionID = TEXT("Move-S");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动", DisplayName="移动-左")
	FName m_MoveAInputActionID = TEXT("Move-A");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动", DisplayName="移动-右")
	FName m_MoveDInputActionID = TEXT("Move-D");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="输入", DisplayName="跳跃输入行为ID")
	FName m_JumpInputActionID = TEXT("Jump");

private:
	void UnregisterMonitor();

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TObjectPtr<ULxCharacterMoveComponent> m_pMoveComponent;

	UPROPERTY()
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem;
};
