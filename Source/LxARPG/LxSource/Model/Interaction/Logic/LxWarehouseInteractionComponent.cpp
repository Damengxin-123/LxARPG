#include "LxWarehouseInteractionComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

ULxWarehouseInteractionComponent::ULxWarehouseInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InteractionActionType = ELxInteractionActionType::Warehouse;
}

void ULxWarehouseInteractionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitializeWarehouseSlots();
}

void ULxWarehouseInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeWarehouseSlots();
}

bool ULxWarehouseInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionComponent* PlayerInteractionComponent)
{
	if (!Super::ExecuteInteraction_Implementation(PlayerInteractionComponent))
	{
		return false;
	}

	// 仓库打开后保持在交互中状态，由仓库 UI 关闭或取消交互时恢复。
	SetWarehouseState(ELxInteractionDataState::Interacting);
	return true;
}

void ULxWarehouseInteractionComponent::GetWarehouseItemSlotList(TArray<ULxItemSlotData*>& OutWarehouseSlots) const
{
	OutWarehouseSlots.Reset();
	for (ULxItemSlotData* SlotData : WarehouseItemSlotList)
	{
		OutWarehouseSlots.Add(SlotData);
	}
}

void ULxWarehouseInteractionComponent::RefreshWarehouseSlots()
{
	RebuildWarehouseItemList();
	BroadcastWarehouseSlotsChanged();
	OnDataChange.Broadcast();
}

void ULxWarehouseInteractionComponent::SetWarehouseState(ELxInteractionDataState InState)
{
	SetInteractionState(InState);
	OnWarehouseStateChanged.Broadcast(GetInteractionState());
}

void ULxWarehouseInteractionComponent::InitializeWarehouseSlots()
{
	const int32 DesiredSlotCount = FMath::Max(1, WarehouseSlotCount);
	if (WarehouseItemSlotList.Num() == DesiredSlotCount)
	{
		RebuildWarehouseItemList();
		return;
	}

	for (ULxItemSlotData* SlotData : WarehouseItemSlotList)
	{
		if (SlotData)
		{
			SlotData->OnItemDataChanged.RemoveDynamic(this, &ULxWarehouseInteractionComponent::HandleWarehouseSlotChanged);
		}
	}

	WarehouseItemSlotList.Reset();
	WarehouseItemList.Reset();

	for (int32 Index = 0; Index < DesiredSlotCount; ++Index)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		// 仓库槽位允许拖入拖出，但物品格子会屏蔽右键使用逻辑。
		NewSlot->InitItemSlot(ELxItemSlotType::Warehouse, LxTag_Item, nullptr);
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxWarehouseInteractionComponent::HandleWarehouseSlotChanged);
		WarehouseItemSlotList.Add(NewSlot);
	}

	RefreshWarehouseSlots();
}

void ULxWarehouseInteractionComponent::RebuildWarehouseItemList()
{
	WarehouseItemList.Reset();
	for (ULxItemSlotData* SlotData : WarehouseItemSlotList)
	{
		if (SlotData && SlotData->IsValid() && SlotData->GetItem())
		{
			WarehouseItemList.AddUnique(SlotData->GetItem());
		}
	}
}

void ULxWarehouseInteractionComponent::BroadcastWarehouseSlotsChanged() const
{
	TArray<ULxItemSlotData*> SlotList;
	for (ULxItemSlotData* SlotData : WarehouseItemSlotList)
	{
		SlotList.Add(SlotData);
	}

	OnWarehouseSlotListChanged.Broadcast(SlotList);
}

void ULxWarehouseInteractionComponent::HandleWarehouseSlotChanged(ULxItemBase* InItemData)
{
	RefreshWarehouseSlots();
}
