#include "LxTreasureChestInteractionComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

ULxTreasureChestInteractionComponent::ULxTreasureChestInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionActionType = ELxInteractionActionType::TreasureChest;
}

void ULxTreasureChestInteractionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitializeTreasureChestSlots();
}

void ULxTreasureChestInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeTreasureChestSlots();
}

bool ULxTreasureChestInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	SetTreasureChestState(ELxInteractionDataState::Interacting);
	return true;
}

void ULxTreasureChestInteractionComponent::GetTreasureChestItemSlotList(TArray<ULxItemSlotData*>& OutTreasureChestSlots) const
{
	OutTreasureChestSlots.Reset();
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		OutTreasureChestSlots.Add(SlotData);
	}
}

void ULxTreasureChestInteractionComponent::RefreshTreasureChestSlots()
{
	RebuildTreasureChestItemList();
	BroadcastTreasureChestSlotsChanged();
	OnDataChange.Broadcast();
}

void ULxTreasureChestInteractionComponent::SetTreasureChestState(ELxInteractionDataState InState)
{
	SetInteractionState(InState);
	OnTreasureChestStateChanged.Broadcast(GetInteractionState());
}

void ULxTreasureChestInteractionComponent::InitializeTreasureChestSlots()
{
	// 已经初始化且配置项数量未变时，只需要刷新有效物品缓存。
	if (bTreasureChestInitialized && TreasureChestItemSlotList.Num() == TreasureChestItemList.Num())
	{
		RebuildTreasureChestItemList();
		return;
	}

	// 重建前解除旧槽位绑定，避免旧对象继续回调本组件。
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		if (SlotData)
		{
			SlotData->OnItemDataChanged.RemoveDynamic(this, &ULxTreasureChestInteractionComponent::HandleTreasureChestSlotChanged);
		}
	}

	TreasureChestItemSlotList.Reset();
	TreasureChestItems.Reset();
	bCompletionBroadcasted = false;

	for (const FLxItemQuote& ItemQuote : TreasureChestItemList)
	{
		// 每个配置项创建一个独立物品对象和一个只可取出的宝箱槽位。
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ItemQuote);
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->InitItemSlot(ELxItemSlotType::TreasureChest, LxTag_Item, NewItem);
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxTreasureChestInteractionComponent::HandleTreasureChestSlotChanged);

		if (NewItem && NewItem->ItemIsValid())
		{
			TreasureChestItems.Add(NewItem);
		}
		TreasureChestItemSlotList.Add(NewSlot);
	}

	bTreasureChestInitialized = true;
	RefreshTreasureChestSlots();
}

void ULxTreasureChestInteractionComponent::RebuildTreasureChestItemList()
{
	TreasureChestItems.Reset();
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		if (SlotData && SlotData->IsValid() && SlotData->GetItem())
		{
			TreasureChestItems.AddUnique(SlotData->GetItem());
		}
	}
}

void ULxTreasureChestInteractionComponent::BroadcastTreasureChestSlotsChanged() const
{
	TArray<ULxItemSlotData*> SlotList;
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		SlotList.Add(SlotData);
	}

	OnTreasureChestSlotListChanged.Broadcast(SlotList);
}

void ULxTreasureChestInteractionComponent::CheckAcquireCompletion()
{
	// 完成事件只广播一次；空宝箱不主动触发完成。
	if (bCompletionBroadcasted || TreasureChestItemSlotList.IsEmpty())
	{
		return;
	}

	const int32 TargetCount = GetAcquireCompletionTargetCount();
	if (TargetCount <= 0 || GetTakenItemEntryCount() < TargetCount)
	{
		return;
	}

	bCompletionBroadcasted = true;
	SetTreasureChestState(ELxInteractionDataState::Finished);
	OnItemAcquireCompleted.Broadcast();
}

int32 ULxTreasureChestInteractionComponent::GetTakenItemEntryCount() const
{
	int32 TakenCount = 0;
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		if (SlotData == nullptr || !SlotData->IsValid())
		{
			++TakenCount;
		}
	}
	return TakenCount;
}

int32 ULxTreasureChestInteractionComponent::GetAcquireCompletionTargetCount() const
{
	const int32 ItemEntryCount = TreasureChestItemSlotList.Num();
	if (AcquireCompletionLimit <= 0)
	{
		return ItemEntryCount;
	}
	return FMath::Min(AcquireCompletionLimit, ItemEntryCount);
}

void ULxTreasureChestInteractionComponent::HandleTreasureChestSlotChanged(ULxItemBase* InItemData)
{
	RefreshTreasureChestSlots();
	CheckAcquireCompletion();
}
