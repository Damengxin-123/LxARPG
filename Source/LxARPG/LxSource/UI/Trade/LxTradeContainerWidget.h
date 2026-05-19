#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionOption.h"
#include "LxTradeContainerWidget.generated.h"

class ULxItemSlotData;
class ULxPlayerInteractionComponent;
class ULxTradeContainerInteractionComponent;

UCLASS(Blueprintable, BlueprintType, DisplayName="交易容器界面")
class LXARPG_API ULxTradeContainerWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionComponent* InPlayerInteractionComponent);

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="设置交易容器组件")
	void SetTradeContainerComponent(ULxTradeContainerInteractionComponent* InTradeContainerComponent);

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="关闭交易交互")
	void CloseTradeInteraction();

	UFUNCTION(BlueprintCallable, Category="Trade UI", DisplayName="获取交易物品UI数据列表")
	TArray<UObject*> GetTradeItemUIDataList();

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易物品列表更新")
	void OnTradeItemListUpdated(const TArray<UObject*>& TradeItemUIDataList);

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易容器打开")
	void OnTradeContainerOpened(ULxTradeContainerInteractionComponent* OpenedTradeContainerComponent);

	UFUNCTION(BlueprintImplementableEvent, Category="Trade UI", DisplayName="交易容器关闭")
	void OnTradeContainerClosed();

private:
	void ShowTradeInteraction();
	void HideTradeInteraction();
	void SetMouseCursorVisible(bool bInVisible);
	void BindPlayerInteractionComponent();
	void UnbindPlayerInteractionComponent();
	void BindTradeContainerComponent();
	void UnbindTradeContainerComponent();
	void RefreshTradeItemList();

	UFUNCTION()
	void HandleInteractionOptionActivated(const FLxInteractionOption& Option, ELxInteractionActionType InteractionType);

	UFUNCTION()
	void HandleInteractionCancelled();

	UFUNCTION()
	void HandleTradeSlotListChanged(const TArray<ULxItemSlotData*>& TradeSlots);

	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionComponent> PlayerInteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxTradeContainerInteractionComponent> TradeContainerComponent = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> TradeItemSlotList;
};
