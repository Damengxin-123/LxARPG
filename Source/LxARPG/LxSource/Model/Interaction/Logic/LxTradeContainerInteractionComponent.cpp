#include "LxTradeContainerInteractionComponent.h"

#include "LxPlayerInteractionComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxTradeContainerInteractionComponent::ULxTradeContainerInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionActionType = ELxInteractionActionType::TradeContainer;
}

void ULxTradeContainerInteractionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitializeTradeSlots();
}

void ULxTradeContainerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeTradeSlots();
}

void ULxTradeContainerInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindPlayerDataTransfer();
	Super::EndPlay(EndPlayReason);
}

bool ULxTradeContainerInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent)
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

void ULxTradeContainerInteractionComponent::RefreshTradeSlots()
{
	ApplyTradeItemValueRateToSlots();
	RebuildTradeItemList();
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		SlotData->SetCanTrade(CanBuyTradeSlot(SlotData, BoundDataTransferComponent));
	}

	BroadcastTradeSlotsChanged();
	OnDataChange.Broadcast();
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

	RefreshTradeSlots();
	return true;
}

bool ULxTradeContainerInteractionComponent::BuyTradeSlotToBackpackSlot(ULxItemSlotData* TradeSlot, ULxItemSlotData* TargetBackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
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

	RefreshTradeSlots();
	return true;
}

bool ULxTradeContainerInteractionComponent::SellBackpackSlot(ULxItemSlotData* BackpackSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
	if (BackpackSlot == nullptr
		|| BackpackSlot->GetSlotType() != ELxItemSlotType::Backpack
		|| !BackpackSlot->IsValid()
		|| DataTransferComponent == nullptr)
	{
		return false;
	}

	const int32 Price = CalculateSlotPrice(BackpackSlot);
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

void ULxTradeContainerInteractionComponent::InitializeTradeSlots()
{
	if (bTradeContainerInitialized && TradeItemSlotList.Num() == TradeItemList.Num())
	{
		RefreshTradeSlots();
		return;
	}

	TradeItemSlotList.Reset();
	TradeItems.Reset();

	for (const FLxItemQuote& ItemQuote : TradeItemList)
	{
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ItemQuote);
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
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

int32 ULxTradeContainerInteractionComponent::CalculateItemPrice(ULxItemBase* Item) const
{
	if (Item == nullptr || !Item->ItemIsValid())
	{
		return 0;
	}

	const FLxItemInformationBase ItemInformation = Item->ItemInformation();
	return FMath::Max(0, ItemInformation.ItemSellPrice);
}

int32 ULxTradeContainerInteractionComponent::CalculateSlotPrice(ULxItemSlotData* Slot) const
{
	return Slot ? Slot->GetItemValue() : 0;
}

void ULxTradeContainerInteractionComponent::HandlePlayerBackpackItemChanged(const TArray<ULxItemSlotData*>& BackpackItems)
{
	RefreshTradeSlots();
}
