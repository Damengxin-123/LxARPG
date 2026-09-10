#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxWaitMechanismStateChangedAsyncAction.generated.h"

class ULxInteractableComponent;

/** 机关状态改变的异步执行事件，并向蓝图提供当前机关状态。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxWaitMechanismStateChangedEvent,
	ELxMechanismState, MechanismState);

/** 持续监听可交互对象中任一机关功能模块状态改变的异步蓝图节点。 */
UCLASS(BlueprintType, DisplayName="监听机关状态改变异步节点")
class LXARPG_API ULxWaitMechanismStateChangedAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 创建异步节点，并持续监听指定可交互对象组件中的机关状态改变。 */
	UFUNCTION(BlueprintCallable, Category="交互|机关|异步", DisplayName="监听机关状态改变",
		meta=(BlueprintInternalUseOnly="true"))
	static ULxWaitMechanismStateChangedAsyncAction* WaitForMechanismStateChanged(
		UPARAM(DisplayName="可交互对象组件") ULxInteractableComponent* InInteractableComponent);

	/** 每次机关状态改变时执行，并输出改变后的机关状态。 */
	UPROPERTY(BlueprintAssignable, Category="交互|机关|异步事件", DisplayName="状态改变")
	FOnLxWaitMechanismStateChangedEvent StateChanged;

	/** 可交互对象组件无效或提前结束运行时执行一次，并输出最后已知的机关状态。 */
	UPROPERTY(BlueprintAssignable, Category="交互|机关|异步事件", DisplayName="已取消")
	FOnLxWaitMechanismStateChangedEvent Cancelled;

	/** 绑定机关状态改变事件并开始持续监听。 */
	virtual void Activate() override;

private:
	/** 接收可交互对象组件的机关状态改变通知。 */
	UFUNCTION()
	void HandleMechanismStateChanged(ELxMechanismState NewState);

	/** 接收可交互对象组件结束运行通知。 */
	void HandleInteractableComponentEndPlay();

	/** 解除事件绑定，并在组件失效时结束异步节点。 */
	void FinishAsCancelled(ELxMechanismState LastState);

	/** 当前等待机关状态改变事件的可交互对象组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractableComponent> InteractableComponent = nullptr;

	/** 最后一次获取到的机关状态，供取消输出使用。 */
	ELxMechanismState LastKnownState = ELxMechanismState::Closed;

	/** 标记异步节点是否已经结束，防止重复广播输出事件。 */
	bool bFinished = false;
};
