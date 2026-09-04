#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxTradeContainerInteractionComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxItemBase;
class ULxItemSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTradeContainerStateChanged, ELxInteractionDataState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxTradeContainerSlotListChanged, const TArray<ULxItemSlotData*>&, TradeSlots);

/** 商城交易功能模块，负责商品列表、买入、卖出和交易状态。 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="商城交互模块")
class LXARPG_API ULxTradeContainerInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	ULxTradeContainerInteractionComponent();

	/** 应用功能节点提供的商城初始配置。 */
	void ApplyConfig(const FLxTradeContainerInteractionConfig& InConfig);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="获取交易物品槽位列表")
	void GetTradeItemSlotList(TArray<ULxItemSlotData*>& OutTradeSlots) const;

	ULxItemSlotData* GetTradeSlotAt(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="刷新交易槽位")
	void RefreshTradeSlots();

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="设置交易容器状态")
	void SetTradeContainerState(ELxInteractionDataState InState);

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="是否可以买入槽位物品")
	bool CanBuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent) const;

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="买入交易槽位物品")
	bool BuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="买入交易槽位物品到指定背包槽位")
	bool BuyTradeSlotToBackpackSlot(ULxItemSlotData* TradeSlot, ULxItemSlotData* TargetBackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="卖出背包槽位物品")
	bool SellBackpackSlot(ULxItemSlotData* BackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent);

	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="设置商品价值倍率")
	void SetTradeItemValueRate(float InTradeItemValueRate);

	UFUNCTION(BlueprintPure, Category="交互|商城", DisplayName="获取商品价值倍率")
	float GetTradeItemValueRate() const { return TradeItemValueRate; }

	/** 设置商城收购物品时使用的价值倍率。 */
	UFUNCTION(BlueprintCallable, Category="交互|商城", DisplayName="设置收购价值比例")
	void SetPurchaseValueRate(float InPurchaseValueRate);

	/** 获取商城收购物品时使用的价值倍率。 */
	UFUNCTION(BlueprintPure, Category="交互|商城", DisplayName="获取收购价值比例")
	float GetPurchaseValueRate() const { return PurchaseValueRate; }

	UPROPERTY(BlueprintAssignable, Category="交互|商城", DisplayName="交易容器状态改变")
	FOnLxTradeContainerStateChanged OnTradeContainerStateChanged;

	UPROPERTY(BlueprintAssignable, Category="交互|商城", DisplayName="交易槽位列表改变")
	FOnLxTradeContainerSlotListChanged OnTradeSlotListChanged;

protected:
	/** 功能模块绑定到交互提供组件后初始化商城槽位。 */
	virtual void OnInitializeInteractionFeature_Implementation() override;

	/** 功能模块释放前解除玩家数据监听。 */
	virtual void OnShutdownInteractionFeature_Implementation() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|商城", DisplayName="交易物品列表")
	TArray<FLxItemQuote> TradeItemList;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|商城", DisplayName="金币物品ID", meta=(Categories="物品"))
	FGameplayTag GoldItemIDTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="交互|商城", DisplayName="商品价值倍率", meta=(ClampMin="0.0", UIMin="0.0"))
	float TradeItemValueRate = 1.0f;

	/** 商城从玩家处收购物品时使用的价值倍率。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="交互|商城", DisplayName="收购价值比例", meta=(ClampMin="0.0", UIMin="0.0"))
	float PurchaseValueRate = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|商城", DisplayName="交易物品对象列表")
	TArray<TObjectPtr<ULxItemBase>> TradeItems;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|商城", DisplayName="交易物品槽位列表")
	TArray<TObjectPtr<ULxItemSlotData>> TradeItemSlotList;

	UPROPERTY(ReplicatedUsing=OnRep_TradeSlots)
	TArray<FLxItemQuote> ReplicatedTradeSlots;

private:
	void InitializeTradeSlots();
	void RebuildTradeItemList();
	void ApplyTradeItemValueRateToSlots();
	void BroadcastTradeSlotsChanged() const;
	void SyncReplicatedTradeSlots();
	void ApplyReplicatedTradeSlots();
	FLxItemQuote BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const;
	void BindPlayerDataTransfer(ULxCharacterDataTransferComponent* DataTransferComponent);
	void UnbindPlayerDataTransfer();
	bool BuildTradeItemQuote(ULxItemSlotData* TradeSlot, FLxItemQuote& OutItemQuote) const;
	bool BuildGoldCost(int32 Price, TArray<FLxItemQuote>& OutCostItemList) const;
	bool CanPutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote) const;
	bool PutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote);
	int32 CalculateItemPrice(ULxItemBase* Item, float ValueRate = 1.0f) const;
	int32 CalculateSlotPrice(ULxItemSlotData* Slot) const;

	UFUNCTION()
	void HandlePlayerBackpackItemChanged(const TArray<ULxItemSlotData*>& BackpackItems);

	UFUNCTION()
	void OnRep_TradeSlots();

	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterDataTransferComponent> BoundDataTransferComponent = nullptr;

	bool bTradeContainerInitialized = false;
};
