#include "LxTradeContainerInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "Net/UnrealNetwork.h"

namespace
{
	/** 交易界面每行按十个槽位补齐，并至少保留十个可接收出售操作的槽位。 */
	int32 GetTradeDisplaySlotCount(int32 ItemCount)
	{
		constexpr int32 SlotsPerGroup = 10;
		return FMath::DivideAndRoundUp(FMath::Max(1, ItemCount), SlotsPerGroup) * SlotsPerGroup;
	}

	ALxPlayerController* GetPlayerControllerFromDataTransfer(ULxCharacterDataTransferComponent* DataTransferComponent)
	{
		const ALxBaseCharacter* OwnerCharacter = DataTransferComponent ? Cast<ALxBaseCharacter>(DataTransferComponent->GetOwner()) : nullptr;
		return OwnerCharacter ? Cast<ALxPlayerController>(OwnerCharacter->GetController()) : nullptr;
	}
}

ULxTradeContainerInteractionComponent::ULxTradeContainerInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::TradeContainer;
}

void ULxTradeContainerInteractionComponent::ApplyConfig(const FLxTradeContainerInteractionConfig& InConfig)
{
	TradeItemConfigs = InConfig.TradeItems;
	if (TradeItemConfigs.IsEmpty())
	{
		// 旧配置中正数表示有限库存；历史上使用负数表达无限商品时按单件无限库存迁移。
		for (const FLxItemQuote& LegacyItem : InConfig.ItemList)
		{
			FLxTradeItemConfig MigratedItem;
			MigratedItem.ItemIDTag = LegacyItem.ItemIDTag;
			MigratedItem.ItemCount = FMath::Max(1, LegacyItem.ItemCount);
			MigratedItem.bLimitedStock = LegacyItem.ItemCount >= 0;
			TradeItemConfigs.Add(MigratedItem);
		}
	}
	// 兼容旧蓝图中尚未填写金币标签的配置，避免所有有价格的买卖被静默拒绝。
	GoldItemIDTag = InConfig.GoldItemIDTag.IsValid() ? InConfig.GoldItemIDTag : LxTag_Item_Material_Currency_Gold;
	TradeItemValueRate = FMath::Max(0.0f, InConfig.SellValueRate);
	PurchaseValueRate = FMath::Max(0.0f, InConfig.PurchaseValueRate);
}

void ULxTradeContainerInteractionComponent::OnInitializeInteractionFeature_Implementation()
{
	Super::OnInitializeInteractionFeature_Implementation();
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
	}
	InitializeTradeSlots();
}

void ULxTradeContainerInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxTradeContainerInteractionComponent, ReplicatedTradeSlots);
	DOREPLIFETIME(ULxTradeContainerInteractionComponent, PurchaseValueRate);
}

void ULxTradeContainerInteractionComponent::OnShutdownInteractionFeature_Implementation()
{
	UnbindPlayerDataTransfer();
	Super::OnShutdownInteractionFeature_Implementation();
}

bool ULxTradeContainerInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	const ALxBaseCharacter* OwnerCharacter = PlayerInteractionComponent ? Cast<ALxBaseCharacter>(PlayerInteractionComponent->GetOwner()) : nullptr;
	BindPlayerDataTransfer(OwnerCharacter ? OwnerCharacter->GetCharacterDataTransferComponent() : nullptr);
	SetTradeContainerState(ELxInteractionDataState::Interacting);
	RefreshTradeSlots();
	return true;
}

void ULxTradeContainerInteractionComponent::GetTradeItemSlotList(TArray<ULxItemSlotData*>& OutTradeSlots) const
{
	OutTradeSlots.Reset();
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		OutTradeSlots.Add(SlotData);
	}
}

ULxItemSlotData* ULxTradeContainerInteractionComponent::GetTradeSlotAt(int32 SlotIndex) const
{
	return TradeItemSlotList.IsValidIndex(SlotIndex) ? TradeItemSlotList[SlotIndex] : nullptr;
}

void ULxTradeContainerInteractionComponent::RefreshTradeSlots()
{
	ApplyTradeItemValueRateToSlots();
	RebuildTradeItemList();
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		SlotData->SetCanTrade(CanBuyTradeSlot(SlotData, BoundDataTransferComponent));
	}

	BroadcastTradeSlotsChanged();
	NotifyFeatureDataChanged();
	SyncReplicatedTradeSlots();
}

void ULxTradeContainerInteractionComponent::SetTradeContainerState(ELxInteractionDataState InState)
{
	SetInteractionState(InState);
	if (InState != ELxInteractionDataState::Interacting)
	{
		UnbindPlayerDataTransfer();
	}
	OnTradeContainerStateChanged.Broadcast(GetInteractionState());
}

bool ULxTradeContainerInteractionComponent::CanBuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent) const
{
	if (DataTransferComponent == nullptr || TradeSlot == nullptr || TradeSlot->GetSlotType() != ELxItemSlotType::Transaction)
	{
		return false;
	}

	FLxItemQuote ItemQuote;
	if (!BuildTradeItemQuote(TradeSlot, ItemQuote))
	{
		return false;
	}

	if (!DataTransferComponent->CanAddItemListToBackpack({ItemQuote}))
	{
		return false;
	}

	TArray<FLxItemQuote> CostItemList;
	const int32 Price = CalculateSlotPrice(TradeSlot);
	if (!BuildGoldCost(Price, CostItemList))
	{
		return Price == 0;
	}

	return DataTransferComponent->CheckHaveBackpackItemList(CostItemList);
}

bool ULxTradeContainerInteractionComponent::BuyTradeSlot(ULxItemSlotData* TradeSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		ALxPlayerController* PlayerController = GetPlayerControllerFromDataTransfer(DataTransferComponent);
		if (PlayerController == nullptr || TradeSlot == nullptr || TradeSlot->GetSlotIndex() == INDEX_NONE)
		{
			return false;
		}

		PlayerController->ServerBuyTradeSlot(OwnerActor, GetRuntimeNodeIndex(), TradeSlot->GetSlotIndex());
		return true;
	}

	if (!CanBuyTradeSlot(TradeSlot, DataTransferComponent))
	{
		RefreshTradeSlots();
		return false;
	}

	FLxItemQuote ItemQuote;
	BuildTradeItemQuote(TradeSlot, ItemQuote);

	TArray<FLxItemQuote> CostItemList;
	const bool bHasCost = BuildGoldCost(CalculateSlotPrice(TradeSlot), CostItemList);
	if (bHasCost && !DataTransferComponent->RemoveItemListFromBackpack(CostItemList))
	{
		RefreshTradeSlots();
		return false;
	}

	if (!DataTransferComponent->AddItemListToBackpack({ItemQuote}))
	{
		if (bHasCost)
		{
			DataTransferComponent->AddItemListToBackpack(CostItemList);
		}
		RefreshTradeSlots();
		return false;
	}

	ConsumePurchasedStock(TradeSlot, ItemQuote.ItemCount);
	RefreshTradeSlots();
	return true;
}

bool ULxTradeContainerInteractionComponent::BuyTradeSlotToBackpackSlot(ULxItemSlotData* TradeSlot, ULxItemSlotData* TargetBackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		ALxPlayerController* PlayerController = GetPlayerControllerFromDataTransfer(DataTransferComponent);
		if (PlayerController == nullptr
			|| TradeSlot == nullptr
			|| TargetBackpackSlot == nullptr
			|| TradeSlot->GetSlotIndex() == INDEX_NONE
			|| TargetBackpackSlot->GetSlotIndex() == INDEX_NONE)
		{
			return false;
		}

		PlayerController->ServerBuyTradeSlotToBackpackSlot(
			OwnerActor, GetRuntimeNodeIndex(), TradeSlot->GetSlotIndex(), TargetBackpackSlot->GetSlotIndex());
		return true;
	}

	if (DataTransferComponent == nullptr || TradeSlot == nullptr || TradeSlot->GetSlotType() != ELxItemSlotType::Transaction)
	{
		return false;
	}

	FLxItemQuote ItemQuote;
	if (!BuildTradeItemQuote(TradeSlot, ItemQuote) || !CanPutItemQuoteInBackpackSlot(TargetBackpackSlot, ItemQuote))
	{
		RefreshTradeSlots();
		return false;
	}

	TArray<FLxItemQuote> CostItemList;
	const bool bHasCost = BuildGoldCost(CalculateSlotPrice(TradeSlot), CostItemList);
	if (bHasCost && !DataTransferComponent->CheckHaveBackpackItemList(CostItemList))
	{
		RefreshTradeSlots();
		return false;
	}

	if (bHasCost && !DataTransferComponent->RemoveItemListFromBackpack(CostItemList))
	{
		RefreshTradeSlots();
		return false;
	}

	if (!PutItemQuoteInBackpackSlot(TargetBackpackSlot, ItemQuote))
	{
		if (bHasCost)
		{
			DataTransferComponent->AddItemListToBackpack(CostItemList);
		}
		RefreshTradeSlots();
		return false;
	}

	ConsumePurchasedStock(TradeSlot, ItemQuote.ItemCount);
	RefreshTradeSlots();
	return true;
}

bool ULxTradeContainerInteractionComponent::SellBackpackSlot(ULxItemSlotData* BackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		ALxPlayerController* PlayerController = GetPlayerControllerFromDataTransfer(DataTransferComponent);
		if (PlayerController == nullptr || BackpackSlot == nullptr || BackpackSlot->GetSlotIndex() == INDEX_NONE)
		{
			return false;
		}

		PlayerController->ServerSellBackpackSlot(
			OwnerActor, GetRuntimeNodeIndex(), BackpackSlot->GetSlotIndex());
		return true;
	}

	if (BackpackSlot == nullptr
		|| BackpackSlot->GetSlotType() != ELxItemSlotType::Backpack
		|| !BackpackSlot->IsValid()
		|| DataTransferComponent == nullptr)
	{
		return false;
	}

	const int32 Price = GetBackpackSlotSellPrice(BackpackSlot);
	TArray<FLxItemQuote> GoldItemList;
	if (!BuildGoldCost(Price, GoldItemList))
	{
		return false;
	}

	ULxItemBase* SoldItem = BackpackSlot->GetItem();
	BackpackSlot->ClearItem();
	if (!DataTransferComponent->AddItemListToBackpack(GoldItemList))
	{
		BackpackSlot->SetItem(SoldItem);
		RefreshTradeSlots();
		return false;
	}

	RefreshTradeSlots();
	return true;
}

int32 ULxTradeContainerInteractionComponent::GetBackpackSlotSellPrice(ULxItemSlotData* BackpackSlot) const
{
	return BackpackSlot && BackpackSlot->GetSlotType() == ELxItemSlotType::Backpack
		? CalculateItemPrice(BackpackSlot->GetItem(), PurchaseValueRate)
		: 0;
}

int32 ULxTradeContainerInteractionComponent::CalculateStackTotalPrice(int32 UnitPrice, int32 ItemCount, float ValueRate)
{
	const int64 BaseValue = static_cast<int64>(FMath::Max(0, UnitPrice))
		* static_cast<int64>(FMath::Max(0, ItemCount));
	const double AdjustedValue = static_cast<double>(BaseValue) * static_cast<double>(FMath::Max(0.0f, ValueRate));
	if (AdjustedValue <= 0.0)
	{
		return 0;
	}

	const int64 RoundedValue = static_cast<int64>(AdjustedValue + 0.5);
	return RoundedValue > MAX_int32 ? MAX_int32 : static_cast<int32>(RoundedValue);
}

bool ULxTradeContainerInteractionComponent::IsTradeSlotLimitedStock(ULxItemSlotData* TradeSlot) const
{
	return TradeSlot && TradeItemConfigs.IsValidIndex(TradeSlot->GetSlotIndex())
		&& TradeItemConfigs[TradeSlot->GetSlotIndex()].bLimitedStock;
}

void ULxTradeContainerInteractionComponent::SetTradeItemValueRate(float InTradeItemValueRate)
{
	const float NewTradeItemValueRate = FMath::Max(0.0f, InTradeItemValueRate);
	if (FMath::IsNearlyEqual(TradeItemValueRate, NewTradeItemValueRate))
	{
		return;
	}

	TradeItemValueRate = NewTradeItemValueRate;
	ApplyTradeItemValueRateToSlots();
	RefreshTradeSlots();
}

void ULxTradeContainerInteractionComponent::SetPurchaseValueRate(float InPurchaseValueRate)
{
	const float NewPurchaseValueRate = FMath::Max(0.0f, InPurchaseValueRate);
	if (FMath::IsNearlyEqual(PurchaseValueRate, NewPurchaseValueRate))
	{
		return;
	}

	PurchaseValueRate = NewPurchaseValueRate;
	RefreshTradeSlots();
}

void ULxTradeContainerInteractionComponent::InitializeTradeSlots()
{
	const int32 DisplaySlotCount = GetTradeDisplaySlotCount(TradeItemConfigs.Num());
	if (bTradeContainerInitialized && TradeItemSlotList.Num() == DisplaySlotCount)
	{
		RefreshTradeSlots();
		return;
	}

	TradeItemSlotList.Reset();
	TradeItems.Reset();

	for (int32 Index = 0; Index < DisplaySlotCount; ++Index)
	{
		ULxItemBase* NewItem = TradeItemConfigs.IsValidIndex(Index)
			? ULxItemBase::CreateItemObject(this, TradeItemConfigs[Index].ToItemQuote())
			: nullptr;
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->SetSlotIndex(Index);
		NewSlot->InitItemSlot(ELxItemSlotType::Transaction, LxTag_Item, NewItem);
		NewSlot->SetItemValueRate(TradeItemValueRate);

		if (NewItem && NewItem->ItemIsValid())
		{
			TradeItems.Add(NewItem);
		}
		TradeItemSlotList.Add(NewSlot);
	}

	bTradeContainerInitialized = true;
	RefreshTradeSlots();
}

void ULxTradeContainerInteractionComponent::RebuildTradeItemList()
{
	TradeItems.Reset();
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		if (SlotData && SlotData->IsValid() && SlotData->GetItem())
		{
			TradeItems.AddUnique(SlotData->GetItem());
		}
	}
}

void ULxTradeContainerInteractionComponent::ApplyTradeItemValueRateToSlots()
{
	TradeItemValueRate = FMath::Max(0.0f, TradeItemValueRate);
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		if (SlotData)
		{
			SlotData->SetItemValueRate(TradeItemValueRate);
		}
	}
}

void ULxTradeContainerInteractionComponent::BroadcastTradeSlotsChanged() const
{
	TArray<ULxItemSlotData*> SlotList;
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		SlotList.Add(SlotData);
	}

	OnTradeSlotListChanged.Broadcast(SlotList);
}

void ULxTradeContainerInteractionComponent::SyncReplicatedTradeSlots()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedTradeSlots.Reset();
	ReplicatedTradeSlots.Reserve(TradeItemSlotList.Num());
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		ReplicatedTradeSlots.Add(BuildItemQuoteFromSlot(SlotData));
	}

	// 库存变化后立即推动所属可交互对象复制，及时刷新客户端商城数量。
	GetOwner()->ForceNetUpdate();
}

void ULxTradeContainerInteractionComponent::ApplyReplicatedTradeSlots()
{
	const int32 DesiredSlotCount = GetTradeDisplaySlotCount(FMath::Max(TradeItemConfigs.Num(), ReplicatedTradeSlots.Num()));
	if (TradeItemSlotList.Num() != DesiredSlotCount)
	{
		TradeItemSlotList.Reset();
		TradeItems.Reset();
		for (int32 Index = 0; Index < DesiredSlotCount; ++Index)
		{
			ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
			NewSlot->SetSlotIndex(Index);
			NewSlot->InitItemSlot(ELxItemSlotType::Transaction, LxTag_Item, nullptr);
			NewSlot->SetItemValueRate(TradeItemValueRate);
			TradeItemSlotList.Add(NewSlot);
		}
		bTradeContainerInitialized = true;
	}

	for (int32 Index = 0; Index < TradeItemSlotList.Num(); ++Index)
	{
		ULxItemSlotData* SlotData = TradeItemSlotList[Index];
		if (!SlotData)
		{
			continue;
		}

		SlotData->SetSlotIndex(Index);
		SlotData->SetItemValueRate(TradeItemValueRate);
		if (!ReplicatedTradeSlots.IsValidIndex(Index)
			|| !ReplicatedTradeSlots[Index].ItemIDTag.IsValid()
			|| ReplicatedTradeSlots[Index].ItemCount <= 0)
		{
			SlotData->ClearItem();
			continue;
		}

		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ReplicatedTradeSlots[Index]);
		if (NewItem)
		{
			SlotData->SetItem(NewItem);
		}
		else
		{
			SlotData->ClearItem();
		}
	}

	RefreshTradeSlots();
}

FLxItemQuote ULxTradeContainerInteractionComponent::BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const
{
	if (!SlotData || !SlotData->IsValid() || !SlotData->GetItem())
	{
		return FLxItemQuote();
	}

	return FLxItemQuote(SlotData->GetItem()->ItemIDTag(), SlotData->GetItem()->ItemCount());
}

void ULxTradeContainerInteractionComponent::BindPlayerDataTransfer(ULxCharacterDataTransferComponent* DataTransferComponent)
{
	if (BoundDataTransferComponent == DataTransferComponent)
	{
		return;
	}

	UnbindPlayerDataTransfer();
	BoundDataTransferComponent = DataTransferComponent;
	if (BoundDataTransferComponent)
	{
		BoundDataTransferComponent->OnBackpackItemChanged.AddDynamic(this, &ULxTradeContainerInteractionComponent::HandlePlayerBackpackItemChanged);
	}
}

void ULxTradeContainerInteractionComponent::UnbindPlayerDataTransfer()
{
	if (BoundDataTransferComponent)
	{
		BoundDataTransferComponent->OnBackpackItemChanged.RemoveDynamic(this, &ULxTradeContainerInteractionComponent::HandlePlayerBackpackItemChanged);
	}
	BoundDataTransferComponent = nullptr;
}

bool ULxTradeContainerInteractionComponent::BuildTradeItemQuote(ULxItemSlotData* TradeSlot, FLxItemQuote& OutItemQuote) const
{
	if (TradeSlot == nullptr || !TradeSlot->IsValid() || TradeSlot->GetItem() == nullptr)
	{
		return false;
	}

	OutItemQuote = FLxItemQuote(TradeSlot->GetItem()->ItemIDTag(), TradeSlot->GetItem()->ItemCount());
	return OutItemQuote.ItemIDTag.IsValid() && OutItemQuote.ItemCount > 0;
}

bool ULxTradeContainerInteractionComponent::BuildGoldCost(int32 Price, TArray<FLxItemQuote>& OutCostItemList) const
{
	OutCostItemList.Reset();
	if (Price <= 0 || !GoldItemIDTag.IsValid())
	{
		return false;
	}

	OutCostItemList.Add(FLxItemQuote(GoldItemIDTag, Price));
	return true;
}

bool ULxTradeContainerInteractionComponent::CanPutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote) const
{
	if (TargetBackpackSlot == nullptr
		|| TargetBackpackSlot->GetSlotType() != ELxItemSlotType::Backpack
		|| !TargetBackpackSlot->ItemIsEnter()
		|| !ItemQuote.ItemIDTag.IsValid()
		|| ItemQuote.ItemCount <= 0)
	{
		return false;
	}

	if (!TargetBackpackSlot->IsValid())
	{
		return true;
	}

	ULxItemBase* ExistingItem = TargetBackpackSlot->GetItem();
	if (ExistingItem == nullptr
		|| ExistingItem->ItemIDTag() != ItemQuote.ItemIDTag
		|| !ExistingItem->ItemIsStackable())
	{
		return false;
	}

	const FLxItemInformationBase ItemInformation = ExistingItem->ItemInformation();
	return ExistingItem->ItemCount() + ItemQuote.ItemCount <= FMath::Max(1, ItemInformation.ItemCountMax);
}

bool ULxTradeContainerInteractionComponent::PutItemQuoteInBackpackSlot(ULxItemSlotData* TargetBackpackSlot, const FLxItemQuote& ItemQuote)
{
	if (!CanPutItemQuoteInBackpackSlot(TargetBackpackSlot, ItemQuote))
	{
		return false;
	}

	ULxItemBase* NewItem = ULxItemBase::CreateItemObject(TargetBackpackSlot->GetOuter(), ItemQuote);
	if (NewItem == nullptr || !NewItem->ItemIsValid())
	{
		return false;
	}

	if (!TargetBackpackSlot->IsValid())
	{
		return TargetBackpackSlot->SetItem(NewItem);
	}

	ULxItemBase* ExistingItem = TargetBackpackSlot->GetItem();
	if (ExistingItem == nullptr || !ExistingItem->ItemStack(NewItem) || NewItem->ItemIsValid())
	{
		return false;
	}

	TargetBackpackSlot->OnItemDataChanged.Broadcast(ExistingItem);
	return true;
}

void ULxTradeContainerInteractionComponent::ConsumePurchasedStock(ULxItemSlotData* TradeSlot, int32 PurchasedCount)
{
	if (!TradeSlot || PurchasedCount <= 0 || !IsTradeSlotLimitedStock(TradeSlot))
	{
		return;
	}

	ULxItemBase* CurrentItem = TradeSlot->GetItem();
	if (!CurrentItem || CurrentItem->ItemCount() <= PurchasedCount)
	{
		TradeSlot->ClearItem();
		return;
	}

	const FLxItemQuote RemainingItem(CurrentItem->ItemIDTag(), CurrentItem->ItemCount() - PurchasedCount);
	if (ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, RemainingItem))
	{
		TradeSlot->SetItem(NewItem);
	}
	else
	{
		TradeSlot->ClearItem();
	}
}

int32 ULxTradeContainerInteractionComponent::CalculateItemPrice(ULxItemBase* Item, float ValueRate) const
{
	if (Item == nullptr || !Item->ItemIsValid())
	{
		return 0;
	}

	const FLxItemInformationBase ItemInformation = Item->ItemInformation();
	return CalculateStackTotalPrice(ItemInformation.ItemSellPrice, static_cast<int32>(Item->ItemCount()), ValueRate);
}

int32 ULxTradeContainerInteractionComponent::CalculateSlotPrice(ULxItemSlotData* Slot) const
{
	return Slot ? Slot->GetItemValue() : 0;
}

void ULxTradeContainerInteractionComponent::HandlePlayerBackpackItemChanged(const TArray<ULxItemSlotData*>& BackpackItems)
{
	RefreshTradeSlots();
}

void ULxTradeContainerInteractionComponent::OnRep_TradeSlots()
{
	ApplyReplicatedTradeSlots();
}
