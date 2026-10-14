#include "LxSafeItemOperations.h"

#include "LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "GameFramework/Actor.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** 仅在游戏线程同步提交期间持有的容器集合，锁持续覆盖最终通知。 */
	TSet<const UObject*> BusyContainers;

	/** 同时锁住来源和目标容器，并在所有批次通知结束后释放。 */
	struct FScopedSafeOperationLock
	{
		/** 保持参与容器存活，回调触发垃圾回收时也不会提前释放模块。 */
		TArray<TStrongObjectPtr<UObject>> Containers;
		/** 保持全部真实槽位存活，隔离容器或通知回调触发回收时仍可安全完成通知。 */
		TArray<TStrongObjectPtr<ULxItemSlotData>> ProtectedSlots;
		/** 当前作用域是否成功获取全部容器锁。 */
		bool bAcquired = false;

		/** 先确认所有容器空闲，再一次获取锁，失败不修改任何数据。 */
		FScopedSafeOperationLock(ULxItemSlotData* Source, const TArray<TObjectPtr<ULxItemSlotData>>& Slots)
		{
			TArray<UObject*> Owners;
			TArray<ULxItemSlotData*> UniqueSlots;
			if (Source) UniqueSlots.AddUnique(Source);
			for (ULxItemSlotData* Slot : Slots) if (Slot) UniqueSlots.AddUnique(Slot);
			for (ULxItemSlotData* Slot : UniqueSlots) Owners.AddUnique(Slot->GetOuter());
			for (UObject* Owner : Owners) if (!IsValid(Owner) || BusyContainers.Contains(Owner)) return;
			for (ULxItemSlotData* Slot : UniqueSlots) ProtectedSlots.Emplace(Slot);
			for (UObject* Owner : Owners)
			{
				Containers.Emplace(Owner);
				BusyContainers.Add(Owner);
			}
			bAcquired = true;
		}

		/** 保证正常结束和提前失败均释放已经获取的锁。 */
		~FScopedSafeOperationLock()
		{
			if (bAcquired) for (const TStrongObjectPtr<UObject>& Owner : Containers) BusyContainers.Remove(Owner.Get());
		}
	};

	/** 写入可直接交给界面显示的失败原因。 */
	bool Fail(FText& OutReason, const TCHAR* Reason)
	{
		OutReason = FText::FromString(Reason);
		return false;
	}

	/** 交易格只能通过买卖函数提交，不能把展示商品作为普通存取来源取走。 */
	bool IsExtractableSourceSlot(const ULxItemSlotData* Slot)
	{
		return LxSafeItemOperations::IsSupportedSlot(Slot)
			|| (IsValid(Slot) && Slot->GetSlotType() == ELxItemSlotType::TreasureChest);
	}

	/** 精确比较实例属性；堆叠时只忽略数量，装备身份仍参与比较。 */
	bool SameRecord(const FLxItemSaveRecord& A, const FLxItemSaveRecord& B, bool bCompareCount = true)
	{
		if (A.ItemIDTag != B.ItemIDTag || (bCompareCount && A.ItemCount != B.ItemCount)
			|| A.EquipmentInstanceID != B.EquipmentInstanceID || A.bHasEquipmentState != B.bHasEquipmentState
			|| A.Equipment.EquipmentID != B.Equipment.EquipmentID || A.Equipment.Enhancement != B.Equipment.Enhancement
			|| A.Entries.Num() != B.Entries.Num() || A.Equipment.NormalEntries.Num() != B.Equipment.NormalEntries.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Entries.Num(); ++Index)
		{
			const FLxItemEntryConfig& Left = A.Entries[Index];
			const FLxItemEntryConfig& Right = B.Entries[Index];
			if (Left.EntryLogicType != Right.EntryLogicType || Left.EntryQuote.EntryID != Right.EntryQuote.EntryID
				|| Left.EntryQuote.EntryProportion != Right.EntryQuote.EntryProportion || Left.EntryQuote.EntryCD != Right.EntryQuote.EntryCD)
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < A.Equipment.NormalEntries.Num(); ++Index)
		{
			if (A.Equipment.NormalEntries[Index].EntryID != B.Equipment.NormalEntries[Index].EntryID
				|| A.Equipment.NormalEntries[Index].Proportion != B.Equipment.NormalEntries[Index].Proportion)
			{
				return false;
			}
		}
		return true;
	}

	/** 客户端只能预检查；关联到非权威容器的槽位不可写入。 */
	bool CanWriteSlot(ULxItemSlotData* Slot)
	{
		if (const ULxCharacterBackpackModule* Backpack = Slot ? Slot->GetTypedOuter<ULxCharacterBackpackModule>() : nullptr)
		{
			return !Backpack->GetOwner() || Backpack->GetOwner()->HasAuthority();
		}
		if (const ULxWarehouseInteractionComponent* Warehouse = Slot ? Slot->GetTypedOuter<ULxWarehouseInteractionComponent>() : nullptr)
		{
			return !Warehouse->GetOwner() || Warehouse->GetOwner()->HasAuthority();
		}
		if (const ULxTreasureChestInteractionComponent* Chest = Slot ? Slot->GetTypedOuter<ULxTreasureChestInteractionComponent>() : nullptr)
		{
			return !Chest->GetOwner() || Chest->GetOwner()->HasAuthority();
		}
		return Slot != nullptr;
	}

	/** 批量提交作用域，来源和目标模块各自只刷新一次最终状态。 */
	struct FScopedSafeItemBatch
	{
		/** 本次操作涉及的背包模块。 */
		TArray<ULxCharacterBackpackModule*> Backpacks;
		/** 本次操作涉及的仓库模块。 */
		TArray<ULxWarehouseInteractionComponent*> Warehouses;
		/** 本次作为来源参与提取的宝箱模块。 */
		TArray<ULxTreasureChestInteractionComponent*> TreasureChests;

		/** 收集槽位所属模块，避免重复开启同一批次。 */
		void AddSlot(ULxItemSlotData* Slot)
		{
			if (ULxCharacterBackpackModule* Backpack = Slot ? Slot->GetTypedOuter<ULxCharacterBackpackModule>() : nullptr)
			{
				Backpacks.AddUnique(Backpack);
			}
			if (ULxWarehouseInteractionComponent* Warehouse = Slot ? Slot->GetTypedOuter<ULxWarehouseInteractionComponent>() : nullptr)
			{
				Warehouses.AddUnique(Warehouse);
			}
			if (ULxTreasureChestInteractionComponent* Chest = Slot ? Slot->GetTypedOuter<ULxTreasureChestInteractionComponent>() : nullptr)
			{
				TreasureChests.AddUnique(Chest);
			}
		}

		/** 准备提交全部槽位后，暂停模块中间刷新。 */
		FScopedSafeItemBatch(ULxItemSlotData* Source, const TArray<TObjectPtr<ULxItemSlotData>>& Slots)
		{
			AddSlot(Source);
			for (ULxItemSlotData* Slot : Slots) AddSlot(Slot);
			for (ULxCharacterBackpackModule* Backpack : Backpacks) Backpack->BeginSafeItemBatch();
			for (ULxWarehouseInteractionComponent* Warehouse : Warehouses) Warehouse->BeginSafeItemBatch();
			for (ULxTreasureChestInteractionComponent* Chest : TreasureChests) Chest->BeginSafeItemBatch();
		}

		/** 所有槽位已经写入后统一重建绑定、更新界面并同步。 */
		~FScopedSafeItemBatch()
		{
			for (ULxCharacterBackpackModule* Backpack : Backpacks) Backpack->EndSafeItemBatch();
			for (ULxWarehouseInteractionComponent* Warehouse : Warehouses) Warehouse->EndSafeItemBatch();
			for (ULxTreasureChestInteractionComponent* Chest : TreasureChests) Chest->EndSafeItemBatch();
		}
	};

	/** 一次自动放置中已经预校验的堆叠数量修改。 */
	struct FStackChange
	{
		/** 接收合并数量的真实目标槽位。 */
		ULxItemSlotData* Slot = nullptr;
		/** 合并后的数量。 */
		int32 NewCount = 0;
	};

	/** 不修改数据的完整接收计划，只有全部容量够用时才能提交。 */
	struct FAcceptPlan
	{
		/** 要合并的现有堆叠。 */
		TArray<FStackChange> StackChanges;
		/** 接收原实例余量的独立空格。 */
		ULxItemSlotData* EmptySlot = nullptr;
		/** 合并后保留在原实例中的数量。 */
		int32 RemainingCount = 0;
	};

	/** 校验来源仍持有建立凭证时的同一完整实例。 */
	bool ValidateTicket(ULxItemTransferTicket* Ticket, FText& OutReason)
	{
		if (IsValid(Ticket) && LxSafeItemOperations::IsSlotBusy(Ticket->SourceSlot))
		{
			return Fail(OutReason, TEXT("容器正在更新物品，请稍后再操作。"));
		}
		if (!IsValid(Ticket) || Ticket->bCompleted || !IsExtractableSourceSlot(Ticket->SourceSlot)
			|| !LxSafeItemOperations::IsSupportedItem(Ticket->Item) || !Ticket->SourceSlot->ItemIsLeave()
			|| Ticket->SourceSlot->GetItem() != Ticket->Item || Ticket->Count != Ticket->ExpectedItem.ItemCount
			|| Ticket->Count != Ticket->Item->ItemCount()
			|| !LxSafeItemOperations::MatchesExpectedRecord(Ticket->SourceSlot, Ticket->ExpectedItem))
		{
			return Fail(OutReason, TEXT("来源物品已经变化或转移已结束，请重新选择物品。"));
		}
		return true;
	}

	/** 规划同属性堆叠与空格，拒绝重复实例、自身容器与不足容量。 */
	bool BuildAcceptPlan(ULxItemTransferTicket* Ticket, const TArray<TObjectPtr<ULxItemSlotData>>& Slots,
		FAcceptPlan& Plan, FText& OutReason)
	{
		if (!ValidateTicket(Ticket, OutReason)) return false;
		Plan.RemainingCount = Ticket->Count;
		TSet<ULxItemSlotData*> SeenSlots;
		TSet<ULxItemBase*> SeenItems;
		ULxItemSlotData* FirstEmptySlot = nullptr;
		for (ULxItemSlotData* Slot : Slots)
		{
			if (!Slot) continue;
			if (LxSafeItemOperations::IsSlotBusy(Slot)) return Fail(OutReason, TEXT("目标容器正在更新物品，请稍后再操作。"));
			if (Ticket->SourceSlot->GetSlotType() == ELxItemSlotType::TreasureChest
				&& Slot->GetSlotType() != ELxItemSlotType::Backpack)
			{
				return Fail(OutReason, TEXT("宝箱物品只能快速取出到背包。"));
			}
			if (!LxSafeItemOperations::IsSupportedSlot(Slot) || SeenSlots.Contains(Slot) || Slot == Ticket->SourceSlot)
			{
				return Fail(OutReason, TEXT("目标容器不支持此操作，或与来源容器相同。"));
			}
			SeenSlots.Add(Slot);
			ULxItemBase* Existing = Slot->GetItem();
			if (Existing)
			{
				if (Existing == Ticket->Item || SeenItems.Contains(Existing))
				{
					return Fail(OutReason, TEXT("目标容器包含重复物品引用，无法安全接收。"));
				}
				SeenItems.Add(Existing);
			}
			if (!Slot->ItemIsEnter() || !Ticket->Item->ItemIDTag().MatchesTag(Slot->GetItemTypeTag())) continue;
			if (!Slot->IsValid())
			{
				if (!FirstEmptySlot) FirstEmptySlot = Slot;
				continue;
			}
			if (Plan.RemainingCount <= 0 || Ticket->Item->ItemType() == ELxItemType::Equipment
				|| !LxSafeItemOperations::IsSupportedItem(Existing) || Existing->ItemType() != Ticket->Item->ItemType()) continue;
			FLxItemSaveRecord ExistingRecord;
			if (!LxItemSaveData::CaptureItem(Existing, ExistingRecord))
			{
				return Fail(OutReason, TEXT("目标物品实例数据不完整，无法安全接收。"));
			}
			if (!SameRecord(Ticket->ExpectedItem, ExistingRecord, false)) continue;
			const int32 Available = FMath::Max(0, Existing->ItemInformation().ItemCountMax - Existing->ItemCount());
			const int32 Added = FMath::Min(Plan.RemainingCount, Available);
			if (Added > 0)
			{
				const int32 NewCount = Existing->ItemCount() + Added;
				if (!Existing->CanSetItemCountSafely(NewCount)) return Fail(OutReason, TEXT("目标堆叠数量无效。"));
				Plan.StackChanges.Add({Slot, NewCount});
				Plan.RemainingCount -= Added;
			}
		}
		if (Plan.RemainingCount > 0)
		{
			if (!FirstEmptySlot) return Fail(OutReason, TEXT("目标容器空间不足，无法接收整堆物品。"));
			Plan.EmptySlot = FirstEmptySlot;
		}
		if (!Ticket->Item->CanSetItemCountSafely(Plan.RemainingCount)) return Fail(OutReason, TEXT("待转移物品数量无效。"));
		return true;
	}
}

void ULxItemTransferTicket::Cancel()
{
	bCompleted = true;
}

bool LxSafeItemOperations::IsContainerBusy(const UObject* Container)
{
	return Container && BusyContainers.Contains(Container);
}

bool LxSafeItemOperations::AcquireContainerOperationLocks(const TArray<UObject*>& Containers)
{
	if (Containers.IsEmpty()) return false;
	for (UObject* Container : Containers)
	{
		if (!IsValid(Container) || BusyContainers.Contains(Container)) return false;
	}
	for (UObject* Container : Containers) BusyContainers.Add(Container);
	return true;
}

void LxSafeItemOperations::ReleaseContainerOperationLocks(const TArray<UObject*>& Containers)
{
	for (UObject* Container : Containers) BusyContainers.Remove(Container);
}

bool LxSafeItemOperations::IsSlotBusy(const ULxItemSlotData* Slot)
{
	if (!Slot) return false;
	// 快捷引用格的父级可能是界面；仍必须遵守其所引用真实物品所属容器的事务锁。
	const ULxItemBase* Item = Slot->GetItem();
	return IsContainerBusy(Slot->GetOuter()) || (Item && IsContainerBusy(Item->GetOuter()));
}

bool LxSafeItemOperations::IsSupportedSlot(const ULxItemSlotData* Slot)
{
	return IsValid(Slot) && (Slot->GetSlotType() == ELxItemSlotType::Backpack || Slot->GetSlotType() == ELxItemSlotType::Warehouse);
}

bool LxSafeItemOperations::IsQuickTransferSourceSlot(const ULxItemSlotData* Slot)
{
	return IsSupportedSlot(Slot) || (IsValid(Slot) && (Slot->GetSlotType() == ELxItemSlotType::TreasureChest
		|| Slot->GetSlotType() == ELxItemSlotType::Transaction));
}

bool LxSafeItemOperations::IsSupportedItem(ULxItemBase* Item)
{
	if (!IsValid(Item) || !Item->ItemIsValid()) return false;
	const ELxItemType Type = Item->ItemType();
	if (Type == ELxItemType::Equipment) return Item->ItemCount() == 1;
	return (Type == ELxItemType::Consumable || Type == ELxItemType::Material) && Item->CanSetItemCountSafely(Item->ItemCount());
}

bool LxSafeItemOperations::MatchesExpectedRecord(ULxItemSlotData* SourceSlot, const FLxItemSaveRecord& ExpectedRecord)
{
	FLxItemSaveRecord CurrentRecord;
	return IsQuickTransferSourceSlot(SourceSlot) && IsSupportedItem(SourceSlot->GetItem())
		&& LxItemSaveData::CaptureItem(SourceSlot->GetItem(), CurrentRecord) && SameRecord(CurrentRecord, ExpectedRecord);
}

ULxItemTransferTicket* LxSafeItemOperations::BeginSafeExtract(ULxItemSlotData* SourceSlot, FText& OutReason)
{
	OutReason = FText::GetEmpty();
	if (IsSlotBusy(SourceSlot))
	{
		Fail(OutReason, TEXT("容器正在更新物品，请稍后再操作。"));
		return nullptr;
	}
	if (!IsExtractableSourceSlot(SourceSlot) || !SourceSlot->ItemIsLeave() || !IsSupportedItem(SourceSlot->GetItem()))
	{
		Fail(OutReason, TEXT("此槽位或物品不支持快速存取。"));
		return nullptr;
	}
	FLxItemSaveRecord Record;
	if (!LxItemSaveData::CaptureItem(SourceSlot->GetItem(), Record))
	{
		Fail(OutReason, TEXT("来源物品数据不完整，无法安全提取。"));
		return nullptr;
	}
	ULxItemTransferTicket* Ticket = NewObject<ULxItemTransferTicket>(SourceSlot->GetOuter());
	Ticket->SourceSlot = SourceSlot;
	Ticket->Item = SourceSlot->GetItem();
	Ticket->ExpectedItem = MoveTemp(Record);
	Ticket->Count = Ticket->Item->ItemCount();
	return Ticket;
}

bool LxSafeItemOperations::CanAccept(ULxItemTransferTicket* Ticket, const TArray<TObjectPtr<ULxItemSlotData>>& TargetSlots, FText& OutReason)
{
	OutReason = FText::GetEmpty();
	FAcceptPlan Plan;
	return BuildAcceptPlan(Ticket, TargetSlots, Plan, OutReason);
}

bool LxSafeItemOperations::Accept(ULxItemTransferTicket* Ticket, const TArray<TObjectPtr<ULxItemSlotData>>& TargetSlots, FText& OutReason)
{
	// 服务器票据通常是调用栈中的临时对象，必须跨Rename和数据通知回调保持存活。
	const TStrongObjectPtr<ULxItemTransferTicket> TicketLifetime(IsValid(Ticket) ? Ticket : nullptr);
	OutReason = FText::GetEmpty();
	FAcceptPlan Plan;
	if (!BuildAcceptPlan(Ticket, TargetSlots, Plan, OutReason)) return false;
	if (!CanWriteSlot(Ticket->SourceSlot)) return Fail(OutReason, TEXT("物品转移必须由服务器执行。"));
	for (ULxItemSlotData* Slot : TargetSlots)
	{
		if (Slot && !CanWriteSlot(Slot)) return Fail(OutReason, TEXT("物品转移必须由服务器执行。"));
	}

	FScopedSafeOperationLock OperationLock(Ticket->SourceSlot, TargetSlots);
	if (!OperationLock.bAcquired) return Fail(OutReason, TEXT("容器正在更新物品，请稍后再操作。"));
	ULxItemBase* Incoming = Ticket->Item;
	if (Plan.EmptySlot && Incoming->GetOuter() != Plan.EmptySlot->GetOuter())
	{
		// 先验证并迁移完整实例归属；子词条继续以原物品为父级，不改变实例身份或打造属性。
		const ERenameFlags Flags = REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty;
		if (!Incoming->Rename(nullptr, Plan.EmptySlot->GetOuter(), Flags | REN_Test)
			|| !Incoming->Rename(nullptr, Plan.EmptySlot->GetOuter(), Flags))
		{
			return Fail(OutReason, TEXT("无法安全迁移物品归属，本次转移已取消。"));
		}
	}
	// 计划中的数量和目标均已校验；锁在批次之后释放，所有通知回调均无法再次修改参与容器。
	FScopedSafeItemBatch Batch(Ticket->SourceSlot, TargetSlots);
	const bool bIncomingCountChanged = Incoming->ItemCount() != Plan.RemainingCount;
	Ticket->SourceSlot->ClearItem(false);
	for (const FStackChange& Change : Plan.StackChanges) Change.Slot->GetItem()->SetItemCountSafely(Change.NewCount, false);
	Incoming->SetItemCountSafely(Plan.RemainingCount, false);
	if (Plan.EmptySlot) Plan.EmptySlot->SetItem(Incoming, false);
	Ticket->bCompleted = true;

	// 先完成全部数据写入，随后让格子和快捷引用读取最终数量；模块在批次结束时统一刷新。
	for (const FStackChange& Change : Plan.StackChanges) Change.Slot->GetItem()->BroadcastItemCountChanged();
	if (bIncomingCountChanged) Incoming->BroadcastItemCountChanged();
	Ticket->SourceSlot->NotifyItemDataChanged();
	if (Plan.EmptySlot) Plan.EmptySlot->NotifyItemDataChanged();
	return true;
}

bool LxSafeItemOperations::CanSplitItem(ULxItemSlotData* SourceSlot, FText& OutReason)
{
	OutReason = FText::GetEmpty();
	if (IsSlotBusy(SourceSlot)) return Fail(OutReason, TEXT("容器正在更新物品，请稍后再操作。"));
	if (!IsSupportedSlot(SourceSlot) || !SourceSlot->ItemIsLeave() || !IsSupportedItem(SourceSlot->GetItem()))
	{
		return Fail(OutReason, TEXT("此槽位或物品不支持拆分。"));
	}
	ULxItemBase* SourceItem = SourceSlot->GetItem();
	if (SourceItem->ItemType() == ELxItemType::Equipment || SourceItem->ItemInformation().ItemCountMax <= 1
		|| SourceItem->ItemCount() <= 1)
	{
		return Fail(OutReason, TEXT("只有数量大于1的消耗品或材料堆叠可以拆分。"));
	}
	return true;
}

bool LxSafeItemOperations::Split(ULxItemSlotData* SourceSlot, const TArray<TObjectPtr<ULxItemSlotData>>& Slots,
	int32 Count, FText& OutReason)
{
	if (!CanSplitItem(SourceSlot, OutReason)) return false;
	ULxItemBase* SourceItem = SourceSlot->GetItem();
	if (Count < 1 || Count >= SourceItem->ItemCount())
	{
		return Fail(OutReason, TEXT("拆分数量必须为1到当前数量减1。"));
	}
	if (!Slots.Contains(SourceSlot) || !CanWriteSlot(SourceSlot))
	{
		return Fail(OutReason, TEXT("拆分必须在来源容器内由服务器执行。"));
	}
	ULxItemSlotData* TargetSlot = nullptr;
	TSet<ULxItemSlotData*> SeenSlots;
	TSet<ULxItemBase*> SeenItems;
	for (ULxItemSlotData* Slot : Slots)
	{
		if (!Slot) continue;
		if (!IsSupportedSlot(Slot) || !CanWriteSlot(Slot) || IsSlotBusy(Slot) || SeenSlots.Contains(Slot)
			|| Slot->GetSlotType() != SourceSlot->GetSlotType() || Slot->GetOuter() != SourceSlot->GetOuter())
		{
			return Fail(OutReason, TEXT("容器槽位配置无效，无法安全拆分。"));
		}
		SeenSlots.Add(Slot);
		if (ULxItemBase* Existing = Slot->GetItem())
		{
			if (SeenItems.Contains(Existing)) return Fail(OutReason, TEXT("容器含有重复物品引用，无法安全拆分。"));
			SeenItems.Add(Existing);
		}
		if (!TargetSlot && Slot != SourceSlot && !Slot->IsValid() && Slot->ItemIsEnter()
			&& SourceItem->ItemIDTag().MatchesTag(Slot->GetItemTypeTag())) TargetSlot = Slot;
	}
	if (!TargetSlot) return Fail(OutReason, TEXT("没有可用的独立空格，无法拆分物品。"));
	FLxItemSaveRecord NewRecord;
	if (!LxItemSaveData::CaptureItem(SourceItem, NewRecord)) return Fail(OutReason, TEXT("物品实例数据不完整，无法拆分。"));
	NewRecord.ItemCount = Count;
	ULxItemBase* NewItem = LxItemSaveData::CreateItem(TargetSlot->GetOuter(), NewRecord);
	const int32 Remaining = SourceItem->ItemCount() - Count;
	if (!IsSupportedItem(NewItem) || !SourceItem->CanSetItemCountSafely(Remaining))
	{
		return Fail(OutReason, TEXT("无法保留物品实例属性，本次拆分已取消。"));
	}

	// 新实例在任何数量修改之前准备完成；只向预选空格放入，不调用自动堆叠流程。
	FScopedSafeOperationLock OperationLock(SourceSlot, Slots);
	if (!OperationLock.bAcquired) return Fail(OutReason, TEXT("容器正在更新物品，请稍后再操作。"));
	FScopedSafeItemBatch Batch(SourceSlot, Slots);
	SourceItem->SetItemCountSafely(Remaining, false);
	TargetSlot->SetItem(NewItem, false);
	SourceItem->BroadcastItemCountChanged();
	TargetSlot->NotifyItemDataChanged();
	return true;
}

void LxSafeItemOperations::Cancel(ULxItemTransferTicket* Ticket)
{
	if (IsValid(Ticket)) Ticket->Cancel();
}
