#include "LxWarehouseInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "Net/UnrealNetwork.h"

ULxWarehouseInteractionComponent::ULxWarehouseInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
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
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
	}
	InitializeWarehouseSlots();
}

void ULxWarehouseInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxWarehouseInteractionComponent, ReplicatedWarehouseSlots);
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
	SyncReplicatedWarehouseSlots();
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
		NewSlot->SetSlotIndex(Index);
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

ULxItemSlotData* ULxWarehouseInteractionComponent::GetWarehouseSlotAt(int32 SlotIndex) const
{
	return WarehouseItemSlotList.IsValidIndex(SlotIndex) ? WarehouseItemSlotList[SlotIndex] : nullptr;
}

bool ULxWarehouseInteractionComponent::MoveBackpackSlotToWarehouse(ULxCharacterBackpackModule* BackpackComponent, int32 BackpackSlotIndex, int32 WarehouseSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BackpackComponent)
	{
		return false;
	}

	ULxItemSlotData* SourceSlot = BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex);
	ULxItemSlotData* TargetSlot = GetWarehouseSlotAt(WarehouseSlotIndex);
	if (!SourceSlot || !TargetSlot)
	{
		return false;
	}

	const ELxItemSlotDropResult DropResult = TargetSlot->ItemEnterToThis(SourceSlot);
	const bool bSucceeded = DropResult == ELxItemSlotDropResult::Swapped
		|| DropResult == ELxItemSlotDropResult::StackedAll
		|| DropResult == ELxItemSlotDropResult::StackedPartial
		|| DropResult == ELxItemSlotDropResult::EnterSuccess;
	if (!bSucceeded)
	{
		return false;
	}

	BackpackComponent->SyncReplicatedBackpackSlots();
	SyncReplicatedWarehouseSlots();
	return true;
}

bool ULxWarehouseInteractionComponent::MoveWarehouseSlotToBackpack(ULxCharacterBackpackModule* BackpackComponent, int32 WarehouseSlotIndex, int32 BackpackSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BackpackComponent)
	{
		return false;
	}

	ULxItemSlotData* SourceSlot = GetWarehouseSlotAt(WarehouseSlotIndex);
	ULxItemSlotData* TargetSlot = BackpackComponent->GetBackpackSlotAt(BackpackSlotIndex);
	if (!SourceSlot || !TargetSlot)
	{
		return false;
	}

	const ELxItemSlotDropResult DropResult = TargetSlot->ItemEnterToThis(SourceSlot);
	const bool bSucceeded = DropResult == ELxItemSlotDropResult::Swapped
		|| DropResult == ELxItemSlotDropResult::StackedAll
		|| DropResult == ELxItemSlotDropResult::StackedPartial
		|| DropResult == ELxItemSlotDropResult::EnterSuccess;
	if (!bSucceeded)
	{
		return false;
	}

	BackpackComponent->SyncReplicatedBackpackSlots();
	SyncReplicatedWarehouseSlots();
	return true;
}

bool ULxWarehouseInteractionComponent::MoveWarehouseSlot(int32 SourceWarehouseSlotIndex, int32 TargetWarehouseSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	ULxItemSlotData* SourceSlot = GetWarehouseSlotAt(SourceWarehouseSlotIndex);
	ULxItemSlotData* TargetSlot = GetWarehouseSlotAt(TargetWarehouseSlotIndex);
	if (!SourceSlot || !TargetSlot || SourceSlot == TargetSlot)
	{
		return false;
	}

	const ELxItemSlotDropResult DropResult = TargetSlot->ItemEnterToThis(SourceSlot);
	const bool bSucceeded = DropResult == ELxItemSlotDropResult::Swapped
		|| DropResult == ELxItemSlotDropResult::StackedAll
		|| DropResult == ELxItemSlotDropResult::StackedPartial
		|| DropResult == ELxItemSlotDropResult::EnterSuccess;
	if (!bSucceeded)
	{
		return false;
	}

	SyncReplicatedWarehouseSlots();
	return true;
}

void ULxWarehouseInteractionComponent::SyncReplicatedWarehouseSlots()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedWarehouseSlots.Reset();
	ReplicatedWarehouseSlots.Reserve(WarehouseItemSlotList.Num());
	for (ULxItemSlotData* SlotData : WarehouseItemSlotList)
	{
		ReplicatedWarehouseSlots.Add(BuildItemQuoteFromSlot(SlotData));
	}
}

void ULxWarehouseInteractionComponent::ApplyReplicatedWarehouseSlots()
{
	const int32 DesiredSlotCount = FMath::Max(WarehouseSlotCount, ReplicatedWarehouseSlots.Num());
	if (WarehouseItemSlotList.Num() != DesiredSlotCount)
	{
		WarehouseSlotCount = DesiredSlotCount;
		InitializeWarehouseSlots();
	}

	for (int32 Index = 0; Index < WarehouseItemSlotList.Num(); ++Index)
	{
		ULxItemSlotData* SlotData = WarehouseItemSlotList[Index];
		if (!SlotData)
		{
			continue;
		}

		SlotData->SetSlotIndex(Index);
		if (!ReplicatedWarehouseSlots.IsValidIndex(Index)
			|| !ReplicatedWarehouseSlots[Index].ItemIDTag.IsValid()
			|| ReplicatedWarehouseSlots[Index].ItemCount <= 0)
		{
			SlotData->ClearItem();
			continue;
		}

		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ReplicatedWarehouseSlots[Index]);
		if (NewItem)
		{
			SlotData->SetItem(NewItem);
		}
		else
		{
			SlotData->ClearItem();
		}
	}

	RebuildWarehouseItemList();
	BroadcastWarehouseSlotsChanged();
	OnDataChange.Broadcast();
}

FLxItemQuote ULxWarehouseInteractionComponent::BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const
{
	if (!SlotData || !SlotData->IsValid() || !SlotData->GetItem())
	{
		return FLxItemQuote();
	}

	return FLxItemQuote(SlotData->GetItem()->ItemIDTag(), SlotData->GetItem()->ItemCount());
}

void ULxWarehouseInteractionComponent::OnRep_WarehouseSlots()
{
	ApplyReplicatedWarehouseSlots();
}
