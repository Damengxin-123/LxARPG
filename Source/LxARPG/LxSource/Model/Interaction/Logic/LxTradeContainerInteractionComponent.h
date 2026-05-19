#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTradeContainerInteractionComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemBase;
class ULxItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTradeContainerStateChanged, ELxInteractionDataState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTradeContainerSlotListChanged, const TArray<ULxItemSlotData*>&, TradeSlots);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType, DisplayName="交易容器交互组件")
class LXARPG_API ULxTradeContainerInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxTradeContainerInteractionComponent();

	virtual void BaseComponentInitialize() override;
	virtual void BeginPlay() override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent) override;

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="获取交易物品槽位列表")
	void GetTradeItemSlotList(TArray<ULxItemSlotData*>& OutTradeSlots) const;

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="刷新交易槽位")
	void RefreshTradeSlots();

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="设置交易容器状态")
	void SetTradeContainerState(ELxInteractionDataState InState);

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="是否可以买入槽位物品")
	bool CanBuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent) const;

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="买入交易槽位物品")
	bool BuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="买入交易槽位物品到指定背包槽位")
	bool BuyTradeSlotToBackpackSlot(ULxItemSlotData* TradeSlot, ULxItemSlotData* TargetBackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="卖出背包槽位物品")
	bool SellBackpackSlot(ULxItemSlotData* BackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="Interaction|Trade", DisplayName="设置商品价值倍率")
	void SetTradeItemValueRate(float InTradeItemValueRate);

	UFUNCTION(BlueprintPure, Category="Interaction|Trade", DisplayName="获取商品价值倍率")
	float GetTradeItemValueRate() const { return TradeItemValueRate; }

	UPROPERTY(BlueprintAssignable, Category="Interaction|Trade", DisplayName="交易容器状态改变")
	FOnLxTradeContainerStateChanged OnTradeContainerStateChanged;

	UPROPERTY(BlueprintAssignable, Category="Interaction|Trade", DisplayName="交易槽位列表改变")
	FOnLxTradeContainerSlotListChanged OnTradeSlotListChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trade", DisplayName="交易物品列表")
	TArray<FLxItemQuote> TradeItemList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trade", DisplayName="金币物品ID", meta=(Categories="物品"))
	FGameplayTag GoldItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction|Trade", DisplayName="商品价值倍率", meta=(ClampMin="0.0", UIMin="0.0"))
	float TradeItemValueRate = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Interaction|Trade", DisplayName="交易物品对象列表")
	TArray<TObjectPtr<ULxItemBase>> TradeItems;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Interaction|Trade", DisplayName="交易物品槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> TradeItemSlotList;

private:
	void InitializeTradeSlots();
	void RebuildTradeItemList();
	void ApplyTradeItemValueRateToSlots();
	void BroadcastTradeSlotsChanged() const;
	void BindPlayerDataTransfer(ULxCharacterDataTransferComponent* DataTransferComponent);
	void UnbindPlayerDataTransfer();
	bool BuildTradeItemQuote(ULxItemSlotData* TradeSlot, FLxItemQuote& OutItemQuote) const;
	bool BuildGoldCost(int32 Price, TArray<FLxItemQuote>& OutCostItemList) const;
	bool CanPutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote) const;
	bool PutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote);
	int32 CalculateItemPrice(ULxItemBase* Item) const;
	int32 CalculateSlotPrice(ULxItemSlotData* Slot) const;

	UFUNCTION()
	void HandlePlayerBackpackItemChanged(const TArray<ULxItemSlotData*>& BackpackItems);

	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterDataTransferComponent> BoundDataTransferComponent = nullptr;

	bool bTradeContainerInitialized = false;
};
