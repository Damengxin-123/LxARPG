#include "LxTreasureChestInteractionComponent.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "Net/UnrealNetwork.h"

ULxTreasureChestInteractionComponent::ULxTreasureChestInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::TreasureChest;
}

void ULxTreasureChestInteractionComponent::ApplyConfig(const FLxTreasureChestInteractionConfig& InConfig)
{
	TreasureChestItemList = InConfig.ItemList;
	AcquireCompletionLimit = FMath::Max(0, InConfig.AcquireCompletionLimit);
}

void ULxTreasureChestInteractionComponent::OnInitializeInteractionFeature_Implementation()
{
	Super::OnInitializeInteractionFeature_Implementation();
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
	}
	InitializeTreasureChestSlots();
}

void ULxTreasureChestInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxTreasureChestInteractionComponent, ReplicatedTreasureChestSlots);
}

bool ULxTreasureChestInteractionComponent::ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent)
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
	NotifyFeatureDataChanged();
	SyncReplicatedTreasureChestSlots();
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

	for (int32 Index = 0; Index < TreasureChestItemList.Num(); ++Index)
	{
		const FLxItemQuote& ItemQuote = TreasureChestItemList[Index];
		// 每个配置项创建一个独立物品对象和一个只可取出的宝箱槽位。
		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ItemQuote);
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->SetSlotIndex(Index);
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

ULxItemSlotData* ULxTreasureChestInteractionComponent::GetTreasureChestSlotAt(int32 SlotIndex) const
{
	return TreasureChestItemSlotList.IsValidIndex(SlotIndex) ? TreasureChestItemSlotList[SlotIndex] : nullptr;
}

bool ULxTreasureChestInteractionComponent::MoveTreasureChestSlotToBackpack(ULxCharacterBackpackModule* BackpackComponent, int32 TreasureChestSlotIndex, int32 BackpackSlotIndex)
{
	if (AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		const ALxBaseCharacter* OwnerCharacter = BackpackComponent ? Cast<ALxBaseCharacter>(BackpackComponent->GetOwner()) : nullptr;
		ALxPlayerController* PlayerController = OwnerCharacter ? Cast<ALxPlayerController>(OwnerCharacter->GetController()) : nullptr;
		if (PlayerController == nullptr)
		{
			return false;
		}

		PlayerController->ServerMoveTreasureChestSlotToBackpack(
			OwnerActor, GetRuntimeNodeIndex(), TreasureChestSlotIndex, BackpackSlotIndex);
		return true;
	}

	if (!GetOwner() || !GetOwner()->HasAuthority() || !BackpackComponent)
	{
		return false;
	}

	ULxItemSlotData* SourceSlot = GetTreasureChestSlotAt(TreasureChestSlotIndex);
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
	RefreshTreasureChestSlots();
	CheckAcquireCompletion();
	return true;
}

void ULxTreasureChestInteractionComponent::SyncReplicatedTreasureChestSlots()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ReplicatedTreasureChestSlots.Reset();
	ReplicatedTreasureChestSlots.Reserve(TreasureChestItemSlotList.Num());
	for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
	{
		ReplicatedTreasureChestSlots.Add(BuildItemQuoteFromSlot(SlotData));
	}
}

void ULxTreasureChestInteractionComponent::ApplyReplicatedTreasureChestSlots()
{
	const int32 DesiredSlotCount = FMath::Max(TreasureChestItemList.Num(), ReplicatedTreasureChestSlots.Num());
	if (TreasureChestItemSlotList.Num() != DesiredSlotCount)
	{
		for (ULxItemSlotData* SlotData : TreasureChestItemSlotList)
		{
			if (SlotData)
			{
				SlotData->OnItemDataChanged.RemoveDynamic(this, &ULxTreasureChestInteractionComponent::HandleTreasureChestSlotChanged);
			}
		}

		TreasureChestItemSlotList.Reset();
		TreasureChestItems.Reset();
		for (int32 Index = 0; Index < DesiredSlotCount; ++Index)
		{
			ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
			NewSlot->SetSlotIndex(Index);
			NewSlot->InitItemSlot(ELxItemSlotType::TreasureChest, LxTag_Item, nullptr);
			NewSlot->OnItemDataChanged.AddDynamic(this, &ULxTreasureChestInteractionComponent::HandleTreasureChestSlotChanged);
			TreasureChestItemSlotList.Add(NewSlot);
		}
		bTreasureChestInitialized = true;
	}

	for (int32 Index = 0; Index < TreasureChestItemSlotList.Num(); ++Index)
	{
		ULxItemSlotData* SlotData = TreasureChestItemSlotList[Index];
		if (!SlotData)
		{
			continue;
		}

		SlotData->SetSlotIndex(Index);
		if (!ReplicatedTreasureChestSlots.IsValidIndex(Index)
			|| !ReplicatedTreasureChestSlots[Index].ItemIDTag.IsValid()
			|| ReplicatedTreasureChestSlots[Index].ItemCount <= 0)
		{
			SlotData->ClearItem();
			continue;
		}

		ULxItemBase* NewItem = ULxItemBase::CreateItemObject(this, ReplicatedTreasureChestSlots[Index]);
		if (NewItem)
		{
			SlotData->SetItem(NewItem);
		}
		else
		{
			SlotData->ClearItem();
		}
	}

	RebuildTreasureChestItemList();
	BroadcastTreasureChestSlotsChanged();
	NotifyFeatureDataChanged();
	CheckAcquireCompletion();
}

FLxItemQuote ULxTreasureChestInteractionComponent::BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const
{
	if (!SlotData || !SlotData->IsValid() || !SlotData->GetItem())
	{
		return FLxItemQuote();
	}

	return FLxItemQuote(SlotData->GetItem()->ItemIDTag(), SlotData->GetItem()->ItemCount());
}

void ULxTreasureChestInteractionComponent::OnRep_TreasureChestSlots()
{
	ApplyReplicatedTreasureChestSlots();
}
