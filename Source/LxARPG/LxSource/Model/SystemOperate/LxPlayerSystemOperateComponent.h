#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxPlayerSystemOperateComponent.generated.h"

class ALxPlayerController;
class ULxLocalPlayerSubsystem;

/**
 * 玩家系统级操作组件：用于处理非游戏性输入（例如按住Alt显示鼠标）。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="玩家系统操作组件")
class LXARPG_API ULxPlayerSystemOperateComponent : public ULxComponentBase, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	ULxPlayerSystemOperateComponent();

	/**
	 * @brief 初始化玩家系统操作组件。
	 *
	 * 缓存玩家控制器和本地玩家子系统，并注册系统级输入监听。
	 */
	virtual void BaseComponentInitialize() override;

	/**
	 * @brief 游戏开始时触发。
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief 组件结束运行时触发。
	 *
	 * @param EndPlayReason 本次结束播放的原因。
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * @brief 处理系统级输入值。
	 *
	 * 当前主要用于按下或释放指定按键时控制鼠标显示与隐藏。
	 *
	 * @param InName 输入行为名称。
	 * @param InValue 输入行为对应的值。
	 */
	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;

	/**
	 * @brief 注册系统操作组件需要监听的输入行为。
	 *
	 * 当前会注册显示鼠标相关的输入行为。
	 */
	virtual void InitMonitorRegistration() override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="玩家输入|系统操作", DisplayName="显示鼠标输入行为ID")
	FName m_ShowMouseCursorInputActionID = TEXT("System-ShowMouseCursor");

private:

	
	// 注销输入监听
	/**
	 * @brief 取消系统级输入监听注册。
	 */
	void UnregisterMonitor();


	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController;

	UPROPERTY()
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem;
};
