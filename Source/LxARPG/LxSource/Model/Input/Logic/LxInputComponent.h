// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxInputComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class APlayerController;
class UEnhancedInputComponent;
class ULxPlayerSettingsSubsystem;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="输入组件")
class LXARPG_API ULxInputComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 创建不逐帧更新的玩家输入组件。 */
	ULxInputComponent();

	/** 初始化输入映射上下文、输入配置和增强输入绑定。 */
	virtual void BaseComponentInitialize() override;

	/** 使用默认配置表和当前玩家存档重建按键，保留现有行为对象及事件绑定。 */
	UFUNCTION(BlueprintCallable, Category="输入|玩家设置", DisplayName="重新应用玩家按键")
	void RebuildInputMappings();

	/** 暂停、改键或卸载前显式释放已经按下的成对输入，防止冲刺和蓄力状态残留。 */
	void ReleaseHeldInputActions();

	/** 处理持续触发类型的输入事件。 */
	UFUNCTION(Category="输入|事件", DisplayName="处理持续输入")
	void HandleContinuousAction(const FInputActionInstance& Instance);

	/** 处理按下/释放类型的输入事件。 */
	UFUNCTION(Category="输入|事件", DisplayName="处理按下及释放输入")
	void HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge);

protected:
	/** 关卡卸载时移除本组件的映射及绑定，不影响其他输入上下文。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** 运行时独立映射，不修改配置表或共享资产。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="输入|运行时", DisplayName="玩家输入映射")
	TObjectPtr<UInputMappingContext> m_pDefaultMappingContext;

	/** 当前控制器的行为实例，在改键时复用以避免重复事件绑定。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="输入|运行时", DisplayName="输入行为实例")
	TMap<ELxInputActionID, TObjectPtr<UInputAction>> m_mapUserInputActionTable;

	/** 提供玩家设置变化通知的独立存档子系统。 */
	TWeakObjectPtr<ULxPlayerSettingsSubsystem> PlayerSettings;
	/** 绑定所属的增强输入组件，用于精确清理本组件创建的绑定。 */
	TWeakObjectPtr<UEnhancedInputComponent> BoundInputComponent;
	/** 本组件创建的绑定句柄，不清除控制器上其他系统的绑定。 */
	TArray<uint32> InputBindingHandles;
	/** 已派发按下且尚未释放的行为，独立于增强输入重建时会清除的内部状态。 */
	TSet<ELxInputActionID> HeldInputActions;
	/** 输入行为和事件绑定已经成功创建。 */
	bool bInputBindingsInitialized = false;
};
