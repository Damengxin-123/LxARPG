#include "LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveHelpers.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "UObject/StrongObjectPtr.h"

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

	/** 单个真实槽位的交易前后状态；所有创建和容量验证先于数据写入。 */
	struct FTradeSlotState
	{
		/** 保持槽位及其原实例存活，通知回调触发回收也不影响提交。 */
		TStrongObjectPtr<ULxItemSlotData> Slot;
		/** 保留交易前完整对象，失败时从未移除该引用。 */
		TStrongObjectPtr<ULxItemBase> OriginalItem;
		/** 交易完成后该槽位持有的原实例或预先创建的完整实例。 */
		TStrongObjectPtr<ULxItemBase> FinalItem;
		/** 来源的完整属性快照，用于最终检查。 */
		FLxItemSaveRecord OriginalRecord;
		/** 提交时写入的最终堆叠数量。 */
		int32 FinalCount = 0;
	};

	/** 商品、金币和背包槽位共同组成的全量交易计划。 */
	struct FAtomicTradePlan
	{
		/** 真实背包模块；负责最终的一次批量刷新。 */
		ULxCharacterBackpackModule* Backpack = nullptr;
		/** 商城所属的事务锁对象。 */
		ULxTradeContainerInteractionComponent* Trade = nullptr;
		/** 所有背包槽位和本次商品槽位的完整最终状态。 */
		TArray<FTradeSlotState> States;
		/** 需要从商城迁移到背包的有限库存原实例。 */
		ULxItemBase* MigratingItem = nullptr;

		/** 采集真实槽位，拒绝忙碌容器、错误格子和重复对象引用。 */
		bool Initialize(ULxTradeContainerInteractionComponent* InTrade, ULxCharacterBackpackModule* InBackpack, bool bReadOnly = false)
		{
			Trade = InTrade;
			Backpack = InBackpack;
			if (!Trade || !Backpack || (!bReadOnly && (LxSafeItemOperations::IsContainerBusy(Trade)
				|| LxSafeItemOperations::IsContainerBusy(Backpack)))) return false;
			TSet<ULxItemSlotData*> Slots;
			TSet<ULxItemBase*> Items;
			for (ULxItemSlotData* Slot : Backpack->GetAllItems())
			{
				if (!Slot || Slot->GetSlotType() != ELxItemSlotType::Backpack
					|| Slot->GetTypedOuter<ULxCharacterBackpackModule>() != Backpack || Slots.Contains(Slot)
					|| (!bReadOnly && LxSafeItemOperations::IsSlotBusy(Slot))) return false;
				Slots.Add(Slot);
				ULxItemBase* Item = Slot->GetItem();
				if (Item && (!Item->ItemIsValid() || Items.Contains(Item))) return false;
				if (Item) Items.Add(Item);
				FTradeSlotState& State = States.AddDefaulted_GetRef();
				State.Slot.Reset(Slot);
				State.OriginalItem.Reset(Item);
				State.FinalItem.Reset(Item);
				State.FinalCount = Item ? Item->ItemCount() : 0;
				if (!LxItemSaveData::CaptureItem(Item, State.OriginalRecord)) return false;
			}
			return !States.IsEmpty();
		}

		/** 从虚拟金币余额中扣款，不修改真实金币对象。 */
		bool RemoveCurrency(FGameplayTag CurrencyID, int32 Count)
		{
			if (Count <= 0) return true;
			if (!CurrencyID.IsValid()) return false;
			int64 Available = 0;
			for (const FTradeSlotState& State : States)
				if (State.FinalItem && State.FinalItem->ItemIDTag() == CurrencyID) Available += State.FinalCount;
			if (Available < Count) return false;
			int32 Remaining = Count;
			for (FTradeSlotState& State : States)
			{
				if (!State.FinalItem || State.FinalItem->ItemIDTag() != CurrencyID) continue;
				const int32 Removed = FMath::Min(Remaining, State.FinalCount);
				if (Removed > 0 && !State.Slot->ItemIsLeave()) return false;
				State.FinalCount -= Removed;
				Remaining -= Removed;
				if (State.FinalCount == 0) State.FinalItem.Reset();
				if (Remaining == 0) break;
			}
			return Remaining == 0;
		}

		/** 仅允许完整属性相同的普通物品合并，装备始终保持独立身份。 */
		bool CanMerge(const FLxItemSaveRecord& Record, const FTradeSlotState& State) const
		{
			if (!State.FinalItem || State.FinalItem->ItemType() == ELxItemType::Equipment) return false;
			FLxItemSaveRecord Existing;
			if (!LxItemSaveData::CaptureItem(State.FinalItem.Get(), Existing)) return false;
			FLxItemSaveRecord Comparable = Record;
			Comparable.ItemCount = Existing.ItemCount;
			return FLxItemSaveRecord::StaticStruct()->CompareScriptStruct(&Existing, &Comparable, 0);
		}

		/** 先合并同属性堆叠，再预先构建所有新增实例，容量不足时丢弃计划。 */
		bool AddRecord(const FLxItemSaveRecord& Record, ULxItemBase* Original = nullptr, ULxItemSlotData* OnlySlot = nullptr)
		{
			ULxItemBase* Prototype = Original;
			TStrongObjectPtr<ULxItemBase> CreatedPrototype;
			if (!Prototype)
			{
				FLxItemSaveRecord Single = Record;
				Single.ItemCount = 1;
				CreatedPrototype.Reset(LxItemSaveData::CreateItem(Backpack, Single));
				Prototype = CreatedPrototype.Get();
			}
			if (!LxSafeItemOperations::IsSupportedItem(Prototype) || Record.ItemCount <= 0) return false;
			const int32 Maximum = Prototype->ItemType() == ELxItemType::Equipment ? 1 : Prototype->ItemInformation().ItemCountMax;
			if (Maximum <= 0 || (Prototype->ItemType() == ELxItemType::Equipment && Record.ItemCount != 1)) return false;
			int32 Remaining = Record.ItemCount;
			for (FTradeSlotState& State : States)
			{
				if (OnlySlot && State.Slot.Get() != OnlySlot) continue;
				if (State.Slot->GetSlotType() != ELxItemSlotType::Backpack || !State.Slot->ItemIsEnter()
					|| !Record.ItemIDTag.MatchesTag(State.Slot->GetItemTypeTag()) || !CanMerge(Record, State)) continue;
				const int32 Added = FMath::Min(Remaining, FMath::Max(0, Maximum - State.FinalCount));
				State.FinalCount += Added;
				Remaining -= Added;
				if (Remaining == 0) return true;
			}
			for (FTradeSlotState& State : States)
			{
				if (OnlySlot && State.Slot.Get() != OnlySlot) continue;
				if (State.Slot->GetSlotType() != ELxItemSlotType::Backpack || State.FinalItem
					|| !State.Slot->ItemIsEnter() || !Record.ItemIDTag.MatchesTag(State.Slot->GetItemTypeTag())) continue;
				const int32 Added = FMath::Min(Remaining, Maximum);
				if (Original)
				{
					State.FinalItem.Reset(Original);
					Original = nullptr;
				}
				else
				{
					FLxItemSaveRecord NewRecord = Record;
					NewRecord.ItemCount = Added;
					State.FinalItem.Reset(LxItemSaveData::CreateItem(Backpack, NewRecord));
					if (!State.FinalItem || !State.FinalItem->CanSetItemCountSafely(Added)) return false;
				}
				State.FinalCount = Added;
				Remaining -= Added;
				if (Remaining == 0) return true;
			}
			return false;
		}

		/** 校验原槽位仍为规划时的完整状态，随后统一写入并发布最终状态。 */
		bool Commit()
		{
			TArray<UObject*> Containers = {Trade, Backpack};
			/** 通知回调可以触发回收，因此锁住的容器也持有强引用直到通知结束。 */
			TArray<TStrongObjectPtr<UObject>> ProtectedContainers;
			/** 保留最终容器中的实例，移除的金币和售出物品才需要归零。 */
			TSet<ULxItemBase*> RetainedItems;
			for (const FTradeSlotState& State : States) if (State.FinalItem) RetainedItems.Add(State.FinalItem.Get());
			for (const FTradeSlotState& State : States)
			{
				FLxItemSaveRecord CurrentRecord;
				if (State.Slot->GetItem() != State.OriginalItem.Get()
					|| !LxItemSaveData::CaptureItem(State.Slot->GetItem(), CurrentRecord)
					|| !FLxItemSaveRecord::StaticStruct()->CompareScriptStruct(&CurrentRecord, &State.OriginalRecord, 0)
					|| (State.FinalItem && !State.FinalItem->CanSetItemCountSafely(State.FinalCount))
					|| (State.OriginalItem && !RetainedItems.Contains(State.OriginalItem.Get())
						&& !State.OriginalItem->CanSetItemCountSafely(0))) return false;
				Containers.AddUnique(State.Slot->GetOuter());
				if (State.OriginalItem) Containers.AddUnique(State.OriginalItem->GetOuter());
			}
			for (UObject* Container : Containers) ProtectedContainers.Emplace(Container);
			if (!LxSafeItemOperations::AcquireContainerOperationLocks(Containers)) return false;
			/** 无论提前失败还是通知结束，都释放金币与物品共用的容器锁。 */
			struct FUnlock
			{
				/** 当前交易持有的全部容器。 */
				TArray<UObject*>& Containers;
				/** 释放本作用域持有的锁。 */
				~FUnlock() { LxSafeItemOperations::ReleaseContainerOperationLocks(Containers); }
			} Unlock{Containers};
			if (MigratingItem)
			{
				bool bPlacedOriginal = false;
				for (const FTradeSlotState& State : States)
					bPlacedOriginal |= State.Slot->GetSlotType() == ELxItemSlotType::Backpack && State.FinalItem.Get() == MigratingItem;
				if (bPlacedOriginal && MigratingItem->GetOuter() != Backpack)
				{
					constexpr ERenameFlags Flags = REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty;
					if (!MigratingItem->Rename(nullptr, Backpack, Flags | REN_Test)
						|| !MigratingItem->Rename(nullptr, Backpack, Flags)) return false;
				}
			}
			Backpack->BeginSafeItemBatch();
			/** 数量事件在全部槽位写入后去重发布，快捷引用也能失效已售出物品。 */
			TSet<ULxItemBase*> CountChangedItems;
			for (FTradeSlotState& State : States)
			{
				if (State.FinalItem && State.FinalItem->ItemCount() != State.FinalCount)
				{
					State.FinalItem->SetItemCountSafely(State.FinalCount, false);
					CountChangedItems.Add(State.FinalItem.Get());
				}
				if (State.OriginalItem && !RetainedItems.Contains(State.OriginalItem.Get()))
				{
					State.OriginalItem->SetItemCountSafely(0, false);
					CountChangedItems.Add(State.OriginalItem.Get());
				}
				if (State.FinalItem.Get() == State.OriginalItem.Get()) continue;
				if (State.FinalItem) State.Slot->SetItem(State.FinalItem.Get(), false);
				else State.Slot->ClearItem(false);
			}
			for (ULxItemBase* Item : CountChangedItems) Item->BroadcastItemCountChanged();
			for (FTradeSlotState& State : States)
				if (State.FinalItem.Get() != State.OriginalItem.Get()
					&& (!State.FinalItem || !CountChangedItems.Contains(State.FinalItem.Get()))) State.Slot->NotifyItemDataChanged();
			Backpack->EndSafeItemBatch();
			Trade->RefreshTradeSlots();
			return true;
		}
	};

	/** 从中转组件查询其真实背包，拒绝把其他角色的槽位带入买卖。 */
	ULxCharacterBackpackModule* GetTradeBackpack(ULxCharacterDataTransferComponent* DataTransfer)
	{
		TArray<ULxItemSlotData*> Slots;
		if (DataTransfer) DataTransfer->GetAllBackpackItems(Slots);
		return Slots.IsEmpty() || !Slots[0] ? nullptr : Slots[0]->GetTypedOuter<ULxCharacterBackpackModule>();
	}

	/** 根据价格和有限库存属性构建整次购买计划。 */
	bool BuildPurchasePlan(ULxTradeContainerInteractionComponent* Trade, ULxCharacterBackpackModule* Backpack,
		ULxItemSlotData* TradeSlot, ULxItemSlotData* TargetSlot, FGameplayTag GoldID, FAtomicTradePlan& Plan, bool bReadOnly = false)
	{
		if (!Trade || !TradeSlot || Trade->GetTradeSlotAt(TradeSlot->GetSlotIndex()) != TradeSlot
			|| TradeSlot->GetSlotType() != ELxItemSlotType::Transaction || !TradeSlot->ItemIsLeave()
			|| !LxSafeItemOperations::IsSupportedItem(TradeSlot->GetItem()) || !Plan.Initialize(Trade, Backpack, bReadOnly)) return false;
		if (TargetSlot && !Backpack->GetAllItems().Contains(TargetSlot)) return false;
		for (const FTradeSlotState& State : Plan.States)
			if (State.OriginalItem.Get() == TradeSlot->GetItem()) return false;
		FLxItemSaveRecord Purchased;
		const int32 Price = ULxTradeContainerInteractionComponent::CalculateStackTotalPrice(
			TradeSlot->GetItem()->ItemInformation().ItemSellPrice, TradeSlot->GetItem()->ItemCount(), Trade->GetTradeItemValueRate());
		if (!LxItemSaveData::CaptureItem(TradeSlot->GetItem(), Purchased)
			|| !Plan.RemoveCurrency(GoldID, Price)) return false;
		const bool bLimited = Trade->IsTradeSlotLimitedStock(TradeSlot);
		if (!bLimited && Purchased.bHasEquipmentState) Purchased.EquipmentInstanceID = FGuid::NewGuid();
		if (!Plan.AddRecord(Purchased, bLimited ? TradeSlot->GetItem() : nullptr, TargetSlot)) return false;
		FTradeSlotState& Stock = Plan.States.AddDefaulted_GetRef();
		Stock.Slot.Reset(TradeSlot);
		Stock.OriginalItem.Reset(TradeSlot->GetItem());
		if (!LxItemSaveData::CaptureItem(TradeSlot->GetItem(), Stock.OriginalRecord)) return false;
		if (!bLimited)
		{
			Stock.FinalItem.Reset(TradeSlot->GetItem());
			Stock.FinalCount = TradeSlot->GetItem()->ItemCount();
		}
		else Plan.MigratingItem = TradeSlot->GetItem();
		return true;
	}

	/** 将待出售格子在虚拟计划中释放，再完整规划金币收入的堆叠与空格。 */
	bool BuildSalePlan(ULxTradeContainerInteractionComponent* Trade, ULxCharacterBackpackModule* Backpack,
		ULxItemSlotData* Source, FGameplayTag GoldID, FAtomicTradePlan& Plan)
	{
		if (!Source || Source->GetSlotType() != ELxItemSlotType::Backpack || !Source->ItemIsLeave()
			|| !LxSafeItemOperations::IsSupportedItem(Source->GetItem()) || !Plan.Initialize(Trade, Backpack)) return false;
		const int32 Price = Trade->GetBackpackSlotSellPrice(Source);
		if (Price <= 0 || !GoldID.IsValid()) return false;
		bool bFound = false;
		for (FTradeSlotState& State : Plan.States)
			if (State.Slot.Get() == Source)
			{
				State.FinalItem.Reset();
				State.FinalCount = 0;
				bFound = true;
				break;
			}
		if (!bFound) return false;
		TStrongObjectPtr<ULxItemBase> Gold(ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(GoldID, 1)));
		FLxItemSaveRecord GoldRecord;
		if (!LxItemSaveData::CaptureItem(Gold.Get(), GoldRecord) || !LxSafeItemOperations::IsSupportedItem(Gold.Get())) return false;
		GoldRecord.ItemCount = Price;
		return Plan.AddRecord(GoldRecord);
	}
}

ULxTradeContainerInteractionComponent::ULxTradeContainerInteractionComponent()
{
	InteractionActionType = ELxInteractionActionType::TradeContainer;
}

bool ULxTradeContainerInteractionComponent::CapturePersistentData(FLxInteractionFeatureSaveRecord& OutRecord) const
{
	if (!Super::CapturePersistentData(OutRecord)) return false;
	for (int32 Index = 0; Index < TradeItemConfigs.Num(); ++Index)
	{
		const FLxTradeItemConfig& Config = TradeItemConfigs[Index];
		if (!Config.bLimitedStock || !Config.ItemIDTag.IsValid()) continue;
		if (!TradeItemSlotList.IsValidIndex(Index) || !IsValid(TradeItemSlotList[Index])
			|| TradeItemSlotList[Index]->GetSlotIndex() != Index) return false;
		ULxItemBase* Item = TradeItemSlotList[Index]->GetItem();
		if (Item && (!IsValid(Item) || !Item->ItemIsValid() || Item->ItemIDTag() != Config.ItemIDTag)) return false;
		FLxInteractionItemSaveRecord& SavedItem = OutRecord.ItemSlots.AddDefaulted_GetRef();
		SavedItem.SlotIndex = Index;
		// 售罄槽位仍保存配置中的物品ID和零数量，避免读档时重新补货或误用于替换商品。
		SavedItem.ItemIDTag = Config.ItemIDTag;
		SavedItem.ItemCount = Item ? static_cast<int32>(Item->ItemCount()) : 0;
        if (!LxInteractionSaveHelpers::CaptureEquipment(Item, SavedItem)) return false;
	}
	return true;
}

bool ULxTradeContainerInteractionComponent::RestorePersistentData(const FLxInteractionFeatureSaveRecord& InRecord)
{
	if (LxSafeItemOperations::IsContainerBusy(this)) return false;
	if (!CanRestorePersistentData(InRecord)) return false;
	// 完整商品列表始终以当前配置为准；无限商品、新增商品和已替换商品不继承旧库存。
	TArray<FLxInteractionItemSaveRecord> CurrentItems;
	CurrentItems.Reserve(TradeItemConfigs.Num());
	for (int32 Index = 0; Index < TradeItemConfigs.Num(); ++Index)
	{
		FLxInteractionItemSaveRecord& Item = CurrentItems.AddDefaulted_GetRef();
		Item.SlotIndex = Index;
		Item.ItemIDTag = TradeItemConfigs[Index].ItemIDTag;
		Item.ItemCount = Item.ItemIDTag.IsValid() ? LxItemQuantity::Clamp(TradeItemConfigs[Index].ItemCount) : 0;
		// 零数量的初始配置保留空商品槽，无限商品也不会因此创建一个装备实例。
		if (Item.ItemCount == 0) Item.ItemIDTag = FGameplayTag();
	}

	TArray<FLxInteractionItemSaveRecord> SavedItems;
	if (!LxInteractionSaveHelpers::ReadItemSlots(InRecord, SavedItems)) return false;

	TSet<int32> SavedIndices;
	for (const FLxInteractionItemSaveRecord& SavedItem : SavedItems)
	{
		if (SavedItem.SlotIndex < 0 || SavedIndices.Contains(SavedItem.SlotIndex)
			|| !LxItemQuantity::IsInRange(SavedItem.ItemCount)
			|| (!SavedItem.ItemIDTag.IsValid() && (SavedItem.ItemCount > 0 || InRecord.DataVersion != 0))) return false;
		SavedIndices.Add(SavedItem.SlotIndex);
		if (!TradeItemConfigs.IsValidIndex(SavedItem.SlotIndex)) continue;
		const FLxTradeItemConfig& Config = TradeItemConfigs[SavedItem.SlotIndex];
		// 旧档的售罄商品缺少ID，只能按原槽位识别；新档始终校验商品ID。
		if (!Config.bLimitedStock || !Config.ItemIDTag.IsValid()
			|| (SavedItem.ItemIDTag.IsValid() && SavedItem.ItemIDTag != Config.ItemIDTag)) continue;
		FLxInteractionItemSaveRecord& Item = CurrentItems[SavedItem.SlotIndex];
		Item.ItemCount = SavedItem.ItemCount;
        Item.EquipmentInstanceID = SavedItem.EquipmentInstanceID;
        Item.bHasEquipmentState = SavedItem.bHasEquipmentState;
        Item.Equipment = SavedItem.Equipment;
		// 通用槽位重建工具使用空ID表示空槽，保存的商品身份仅用于上面的匹配校验。
		Item.ItemIDTag = Item.ItemCount > 0 ? Config.ItemIDTag : FGameplayTag();
	}

	TArray<TObjectPtr<ULxItemSlotData>> RestoredSlots;
	if (!LxInteractionSaveHelpers::BuildRestoredSlots(this, CurrentItems,
		GetTradeDisplaySlotCount(TradeItemConfigs.Num()), ELxItemSlotType::Transaction, RestoredSlots)) return false;
	UnbindPlayerDataTransfer();
	TradeItemSlotList = MoveTemp(RestoredSlots);
	bTradeContainerInitialized = true;
	Super::RestorePersistentData(InRecord);
	RefreshTradeSlots();
	OnTradeContainerStateChanged.Broadcast(GetInteractionState());
	return true;
}

void ULxTradeContainerInteractionComponent::ApplyConfig(const FLxTradeContainerInteractionConfig& InConfig)
{
	if (LxSafeItemOperations::IsContainerBusy(this)) return;
	TradeItemConfigs = InConfig.TradeItems;
	// 商城每项配置代表一个真实物品堆叠；批量交易金币仍由交易计划按堆叠拆分。
	for (FLxTradeItemConfig& TradeItemConfig : TradeItemConfigs)
	{
		TradeItemConfig.ItemCount = LxItemQuantity::Clamp(TradeItemConfig.ItemCount);
	}
	GoldItemIDTag = InConfig.GoldItemIDTag;
	// 客户端价格由服务器同步，延迟到达的资产配置不可覆盖当前运行时倍率。
	if (!GetOwner() || GetOwner()->HasAuthority())
	{
		TradeItemValueRate = FMath::Max(0.0f, InConfig.SellValueRate);
		PurchaseValueRate = FMath::Max(0.0f, InConfig.PurchaseValueRate);
	}
}

void ULxTradeContainerInteractionComponent::OnInitializeInteractionFeature_Implementation()
{
	Super::OnInitializeInteractionFeature_Implementation();
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->SetReplicates(true);
		if (!OwnerActor->HasAuthority())
		{
			// 复制库存可能早于交互节点绑定，避免客户端按初始商品重新补货。
			ApplyReplicatedTradeSlots();
			return;
		}
	}
	InitializeTradeSlots();
}

void ULxTradeContainerInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULxTradeContainerInteractionComponent, ReplicatedTradeSlots);
	DOREPLIFETIME(ULxTradeContainerInteractionComponent, TradeItemValueRate);
	DOREPLIFETIME(ULxTradeContainerInteractionComponent, PurchaseValueRate);
}

void ULxTradeContainerInteractionComponent::OnRep_ValueRates()
{
	RefreshTradeSlots();
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
	FAtomicTradePlan Plan;
	return BuildPurchasePlan(const_cast<ULxTradeContainerInteractionComponent*>(this),
		GetTradeBackpack(DataTransferComponent), TradeSlot, nullptr, GoldItemIDTag, Plan, true);
}

ULxItemTransferTicket* ULxTradeContainerInteractionComponent::BeginSafeExtractTradeItem(int32 SlotIndex, FText& OutReason)
{
	ULxItemSlotData* Slot = GetTradeSlotAt(SlotIndex);
	if (LxSafeItemOperations::IsContainerBusy(this) || !Slot || !Slot->ItemIsLeave()
		|| !LxSafeItemOperations::IsSupportedItem(Slot->GetItem()))
	{
		OutReason = NSLOCTEXT("LxTradeOperations", "TradeSourceInvalid", "商品槽位已失效或不支持快速购买");
		return nullptr;
	}
	ULxItemTransferTicket* Ticket = NewObject<ULxItemTransferTicket>(this);
	Ticket->SourceSlot = Slot;
	Ticket->Item = Slot->GetItem();
	Ticket->Count = Ticket->Item->ItemCount();
	if (!LxItemSaveData::CaptureItem(Ticket->Item, Ticket->ExpectedItem))
	{
		OutReason = NSLOCTEXT("LxTradeOperations", "TradeRecordInvalid", "商品实例数据不完整");
		return nullptr;
	}
	return Ticket;
}

bool ULxTradeContainerInteractionComponent::CanBuyTradeItem(ULxCharacterBackpackModule* Backpack,
	int32 TradeSlotIndex, FText& OutReason) const
{
	FAtomicTradePlan Plan;
	if (!BuildPurchasePlan(const_cast<ULxTradeContainerInteractionComponent*>(this), Backpack,
		GetTradeSlotAt(TradeSlotIndex), nullptr, GoldItemIDTag, Plan, true))
	{
		OutReason = NSLOCTEXT("LxTradeOperations", "CannotBuyWholeStack", "金币不足、背包空间不足或商品已经变化，无法购买整组商品");
		return false;
	}
	return true;
}

bool ULxTradeContainerInteractionComponent::CanSellBackpackSlot(ULxItemSlotData* BackpackSlot,
	ULxCharacterDataTransferComponent* DataTransferComponent) const
{
	FAtomicTradePlan Plan;
	return BuildSalePlan(const_cast<ULxTradeContainerInteractionComponent*>(this),
		GetTradeBackpack(DataTransferComponent), BackpackSlot, GoldItemIDTag, Plan);
}

bool ULxTradeContainerInteractionComponent::ExecuteAtomicPurchase(ULxItemSlotData* TradeSlot,
	ULxItemSlotData* TargetSlot, ULxCharacterDataTransferComponent* DataTransferComponent)
{
	ULxCharacterBackpackModule* Backpack = GetTradeBackpack(DataTransferComponent);
	if (!Backpack || (GetOwner() && !GetOwner()->HasAuthority())
		|| (Backpack->GetOwner() && !Backpack->GetOwner()->HasAuthority())) return false;
	FAtomicTradePlan Plan;
	return BuildPurchasePlan(this, Backpack, TradeSlot, TargetSlot, GoldItemIDTag, Plan) && Plan.Commit();
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

		PlayerController->ServerBuyTradeSlot(OwnerActor, GetRuntimeNodeIndex(), GetInteractionTreeRevision(), TradeSlot->GetSlotIndex());
		return true;
	}

	return ExecuteAtomicPurchase(TradeSlot, nullptr, DataTransferComponent);
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
			OwnerActor, GetRuntimeNodeIndex(), GetInteractionTreeRevision(), TradeSlot->GetSlotIndex(), TargetBackpackSlot->GetSlotIndex());
		return true;
	}

	return TargetBackpackSlot && ExecuteAtomicPurchase(TradeSlot, TargetBackpackSlot, DataTransferComponent);
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
			OwnerActor, GetRuntimeNodeIndex(), GetInteractionTreeRevision(), BackpackSlot->GetSlotIndex());
		return true;
	}

	ULxCharacterBackpackModule* Backpack = GetTradeBackpack(DataTransferComponent);
	if (!Backpack || (Backpack->GetOwner() && !Backpack->GetOwner()->HasAuthority())) return false;
	FAtomicTradePlan Plan;
	return BuildSalePlan(this, Backpack, BackpackSlot, GoldItemIDTag, Plan) && Plan.Commit();
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
	// 在转换为整数前处理非有限值和超大价格，避免浮点转整数溢出影响金币计划。
	if (!FMath::IsFinite(AdjustedValue) || AdjustedValue >= static_cast<double>(MAX_int32)) return MAX_int32;
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
	if (LxSafeItemOperations::IsContainerBusy(this)) return;
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
	if (LxSafeItemOperations::IsContainerBusy(this)) return;
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
	if (LxSafeItemOperations::IsContainerBusy(this)) return;
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
		// 零数量配置直接创建空槽，避免不可堆叠物品把零数量强制归一化为一个。
		ULxItemBase* NewItem = TradeItemConfigs.IsValidIndex(Index) && TradeItemConfigs[Index].ItemCount > 0
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

	// 完整采集成功后再替换网络快照，避免失败时向客户端发送部分容器。
	TArray<FLxItemSaveRecord> Snapshot;
	Snapshot.Reserve(TradeItemSlotList.Num());
	for (ULxItemSlotData* SlotData : TradeItemSlotList)
	{
		FLxItemSaveRecord Record;
        if (!LxItemSaveData::CaptureItem(SlotData ? SlotData->GetItem() : nullptr, Record)) return;
        Snapshot.Add(MoveTemp(Record));
	}

	// 库存变化后立即推动所属可交互对象复制，及时刷新客户端商城数量。
	GetOwner()->ForceNetUpdate();
    ReplicatedTradeSlots = MoveTemp(Snapshot);
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

		ULxItemBase* NewItem = LxItemSaveData::CreateItem(this, ReplicatedTradeSlots[Index]);
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
		BoundDataTransferComponent->OnBackpackItemChanged.AddUObject(this, &ULxTradeContainerInteractionComponent::HandlePlayerBackpackItemChanged);
	}
}

void ULxTradeContainerInteractionComponent::UnbindPlayerDataTransfer()
{
	if (BoundDataTransferComponent)
	{
		BoundDataTransferComponent->OnBackpackItemChanged.RemoveAll(this);
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
	// 原子交易结束后主动刷新一次，背包批次通知中的回调不重复发布商城状态。
	if (!LxSafeItemOperations::IsContainerBusy(this)) RefreshTradeSlots();
}

void ULxTradeContainerInteractionComponent::OnRep_TradeSlots()
{
	ApplyReplicatedTradeSlots();
}
