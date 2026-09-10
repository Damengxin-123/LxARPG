#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LxWaitItemTransferCompletedAsyncAction.generated.h"

class ULxInteractableComponent;

/** 等待物品传递的异步执行事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxWaitItemTransferCompletedEvent);

/** 等待可交互对象成功完成一次物品传递的异步蓝图节点。 */
UCLASS(BlueprintType, DisplayName="等待物品传递完成异步节点")
class LXARPG_API ULxWaitItemTransferCompletedAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 在权威端创建异步节点，并等待指定可交互对象组件完成下一次物品传递。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="交互|物品传递|异步", DisplayName="等待物品传递完成",
		meta=(BlueprintInternalUseOnly="true"))
	static ULxWaitItemTransferCompletedAsyncAction* WaitForItemTransferCompleted(
		UPARAM(DisplayName="可交互对象组件") ULxInteractableComponent* InInteractableComponent);

	/** 成功完成下一次物品传递时执行一次。 */
	UPROPERTY(BlueprintAssignable, Category="交互|物品传递|异步事件", DisplayName="完成")
	FOnLxWaitItemTransferCompletedEvent Completed;

	/** 可交互对象组件无效或提前结束运行时执行一次。 */
	UPROPERTY(BlueprintAssignable, Category="交互|物品传递|异步事件", DisplayName="已取消")
	FOnLxWaitItemTransferCompletedEvent Cancelled;

	/** 绑定物品传递完成事件并开始等待。 */
	virtual void Activate() override;

private:
	/** 接收可交互对象组件的物品传递完成通知。 */
	UFUNCTION()
	void HandleItemTransferCompleted();

	/** 接收可交互对象组件结束运行通知。 */
	void HandleInteractableComponentEndPlay();

	/** 解除事件绑定，并按完成或组件失效结果结束异步节点。 */
	void FinishWaiting(bool bWasCancelled);

	/** 当前等待物品传递完成事件的可交互对象组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxInteractableComponent> InteractableComponent = nullptr;

	/** 标记异步节点是否已经结束，防止重复广播输出事件。 */
	bool bFinished = false;
};
