#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxPlayerInteractionComponent.generated.h"

class ULxInteractableComponent;
class ULxInteractionNode;

/** 玩家角色持有的交互管理组件，维护候选队列、选项刷新和多级交互导航。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DeprecatedNode,
	DeprecationMessage="请使用玩家操控组件中的玩家交互模块"), Blueprintable, DisplayName="玩家交互组件（旧版）")
class LXARPG_API ULxPlayerInteractionComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	virtual void BaseComponentInitialize() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;
	virtual void InitMonitorRegistration() override;

	/** 将可交互对象组件加入候选队列。先加入的对象会优先显示。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="添加可交互组件")
	void AddInteractableComponent(ULxInteractableComponent* InInteractableComponent);

	/** 从候选队列移除指定可交互对象组件。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="移除可交互组件")
	void RemoveInteractableComponent(ULxInteractableComponent* InInteractableComponent);

	/** 清空所有候选交互对象，并取消当前交互。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="清空可交互组件")
	void ClearInteractableComponents();

	/** 获取当前待交互队列。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="获取待交互队列")
	TArray<ULxInteractableComponent*> GetInteractableQueue() const;

	/** 刷新入口交互选项，并广播给UI。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="刷新入口选项")
	void RefreshEntranceOptions();

	/** 刷新当前多级交互选项，并广播给UI。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="刷新当前交互选项")
	void RefreshCurrentInteractionOptions();

	/** 使用入口选项数组下标选择一个入口交互。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="按下标选择入口选项")
	void SelectEntranceOptionByIndex(int32 OptionIndex);

	/** 选择并处理一个交互选项。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="选择交互选项")
	void SelectInteractionOption(const FLxInteractionOption& Option);

	/** 激活一个交互选项，并广播给对话、交易等交互UI处理显示或后续逻辑。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="激活交互选项")
	bool ActivateInteractionOption(const FLxInteractionOption& Option);

	/** 返回当前交互节点的上级节点。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="返回上级交互节点")
	void BackToParentInteractionNode();

	/** 取消当前交互并清空当前选项。 */
	UFUNCTION(BlueprintCallable, Category="交互", DisplayName="取消交互")
	void CancelInteraction();

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="入口选项更新事件")
	FOnLxInteractionOptionListUpdated OnEntranceOptionsUpdated;

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="当前交互选项更新事件")
	FOnLxInteractionOptionListUpdated OnCurrentInteractionOptionsUpdated;

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="交互选项执行事件")
	FOnLxInteractionOptionExecuted OnInteractionOptionExecuted;

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="交互选项激活事件")
	FOnLxInteractionOptionActivated OnInteractionOptionActivated;

	UPROPERTY(BlueprintAssignable, Category="交互", DisplayName="交互取消事件")
	FOnLxInteractionCancelled OnInteractionCancelled;

private:
	/** 按进入交互范围顺序保存的候选可交互对象队列。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxInteractableComponent>> InteractableQueue;

	/** 当前正在处理的可交互对象组件。 */
	UPROPERTY()
	TObjectPtr<ULxInteractableComponent> CurrentInteractableComponent = nullptr;

	/** 当前所在的交互树节点。 */
	UPROPERTY()
	TObjectPtr<ULxInteractionNode> CurrentInteractionNode = nullptr;

	/** 最近一次广播给入口UI的选项缓存。 */
	UPROPERTY()
	TArray<FLxInteractionOption> CachedEntranceOptions;

	/** 最近一次广播给多级交互UI的选项缓存。 */
	UPROPERTY()
	TArray<FLxInteractionOption> CachedCurrentOptions;

	FLxInteractionOption BuildOption(ULxInteractableComponent* SourceComponent, ULxInteractionNode* Node, bool bIsBackOption = false) const;
	bool ShouldShowInEntranceOptions(const ULxInteractionNode* Node) const;
	bool ValidateInteractionNodePlacement(const ULxInteractionNode* Node) const;
	void BindInteractableComponent(ULxInteractableComponent* InInteractableComponent);
	void UnbindInteractableComponent(ULxInteractableComponent* InInteractableComponent);
	void RemoveInvalidInteractables();

	UFUNCTION()
	void HandleInteractableOptionsChanged();
};
