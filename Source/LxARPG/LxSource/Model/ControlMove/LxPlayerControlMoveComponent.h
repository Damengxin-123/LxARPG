#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
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

	/**
	 * @brief 初始化玩家移动控制组件。
	 *
	 * 会缓存所属角色、移动组件和本地玩家子系统，并准备输入监听注册。
	 */
	virtual void BaseComponentInitialize() override;

	/**
	 * @brief 游戏开始时触发。
	 *
	 * 预留给运行时初始化或调试扩展使用。
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 组件结束运行时触发。
	 *
	 * @param EndPlayReason 本次结束播放的原因。
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 处理本组件监听到的输入值。
	 *
	 * 根据输入行为名称把输入转发给角色移动组件，驱动移动、跳跃和视角行为。
	 *
	 * @param InName 输入行为名称。
	 * @param InValue 输入行为对应的值。
	 */
	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;

	/**
	 * @brief 向本地玩家子系统注册本组件需要监听的输入行为。
	 *
	 * 会注册移动、跳跃和视角相关输入。
	 */
	virtual void InitMonitorRegistration() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动行为ID", DisplayName="移动-前")
	FName m_MoveWInputActionID = TEXT("Move-W");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动行为ID", DisplayName="移动-后")
	FName m_MoveSInputActionID = TEXT("Move-S");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动行为ID", DisplayName="移动-左")
	FName m_MoveAInputActionID = TEXT("Move-A");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|移动行为ID", DisplayName="移动-右")
	FName m_MoveDInputActionID = TEXT("Move-D");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|输入行为ID", DisplayName="跳跃")
	FName m_JumpInputActionID = TEXT("Jump");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|输入行为ID", DisplayName="视角X")
	FName m_LookXInputActionID = TEXT("Look-X");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|输入行为ID", DisplayName="视角Y")
	FName m_LookYInputActionID = TEXT("Look-Y");

private:
	/**
	 * @brief 取消本组件的全部输入监听注册。
	 *
	 * 组件销毁或结束运行时调用，避免残留无效监听。
	 */
	void UnregisterMonitor();

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	UPROPERTY()
	TObjectPtr<ULxCharacterMoveComponent> m_pMoveComponent;

	UPROPERTY()
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem;
};
