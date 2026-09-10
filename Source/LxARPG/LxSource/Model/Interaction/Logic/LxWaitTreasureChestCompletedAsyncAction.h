#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LxWaitTreasureChestCompletedAsyncAction.generated.h"

class ULxInteractableComponent;

/** 等待宝箱交互完成的异步执行事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxWaitTreasureChestCompletedEvent);

/** 等待可交互对象中的宝箱达到物品获取完成条件的异步蓝图节点。 */
UCLASS(BlueprintType, DisplayName="等待宝箱交互完成异步节点")
class LXARPG_API ULxWaitTreasureChestCompletedAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 创建异步节点，并等待指定可交互对象组件中的宝箱完成下一次交互。 */
	UFUNCTION(BlueprintCallable, Category="交互|宝箱|异步", DisplayName="等待宝箱交互完成",
		meta=(BlueprintInternalUseOnly="true"))
	static ULxWaitTreasureChestCompletedAsyncAction* WaitForTreasureChestCompleted(
		UPARAM(DisplayName="可交互对象组件") ULxInteractableComponent* InInteractableComponent);

	/** 宝箱达到物品获取完成条件时执行一次。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱|异步事件", DisplayName="完成")
	FOnLxWaitTreasureChestCompletedEvent Completed;

	/** 可交互对象组件无效或提前结束运行时执行一次。 */
	UPROPERTY(BlueprintAssignable, Category="交互|宝箱|异步事件", DisplayName="已取消")
	FOnLxWaitTreasureChestCompletedEvent Cancelled;

	/** 绑定宝箱交互完成事件并开始等待。 */
	virtual void Activate() override;

private:
	/** 接收可交互对象组件的宝箱交互完成通知。 */
	UFUNCTION()
	void HandleTreasureChestCompleted();

	/** 接收可交互对象组件结束运行通知。 */
	void HandleInteractableComponentEndPlay();

	/** 解除事件绑定，并按完成或组件失效结果结束异步节点。 */
	void FinishWaiting(bool bWasCancelled);

	/** 当前等待宝箱交互完成事件的可交互对象组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractableComponent> InteractableComponent = nullptr;

	/** 标记异步节点是否已经结束，防止重复广播输出事件。 */
	bool bFinished = false;
};
