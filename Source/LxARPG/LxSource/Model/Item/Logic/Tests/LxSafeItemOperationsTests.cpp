#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/GarbageCollection.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

namespace
{
	/** 加载真实物品配置的隔离环境，禁止写入用户存档。 */
	struct FSafeItemFixture
	{
		/** 保持测试实例存活。 */
		TStrongObjectPtr<UGameInstance> Instance;
		/** 加载配置并禁止保存。 */
		FSafeItemFixture() : Instance(NewObject<UGameInstance>())
		{
			Instance->Init();
			if (ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>())
				if (System->GetSaveManager()) System->GetSaveManager()->SetReadOnly(true);
		}
		/** 关闭隔离实例。 */
		~FSafeItemFixture() { Instance->Shutdown(); }
		/** 创建普通材料或指定物品实例。 */
		ULxItemBase* MakeItem(int32 Count, FGameplayTag ID = LxTag_Item_Material_Currency_Gold, UObject* Outer = nullptr)
		{
			return ULxItemBase::CreateItemObject(Outer ? Outer : Instance.Get(), FLxItemQuote(ID, Count));
		}
		/** 创建隔离槽位，不接触角色存档。 */
		ULxItemSlotData* MakeSlot(ELxItemSlotType Type, ULxItemBase* Item = nullptr, int32 Index = 0, UObject* Outer = nullptr)
		{
			ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(Outer ? Outer : Instance.Get());
			Slot->InitItemSlot(Type, LxTag_Item, Item);
			Slot->SetSlotIndex(Index);
			return Slot;
		}
	};
}

/** 验证容量不足、取消和过期请求均不修改来源及目标。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemFailureTest, "LxARPG.Items.SafeOperations.FailureAndCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemFailureTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	ULxItemBase* Item = Fixture.MakeItem(6);
	if (!TestNotNull(TEXT("创建来源材料"), Item)) return false;
	const int32 Max = Item->ItemInformation().ItemCountMax;
	if (!TestTrue(TEXT("堆叠上限满足测试"), Max >= 6)) return false;
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Item);
	ULxItemBase* Existing = Fixture.MakeItem(Max - 3);
	ULxItemSlotData* Target = Fixture.MakeSlot(ELxItemSlotType::Warehouse, Existing);
	TArray<TObjectPtr<ULxItemSlotData>> Targets = {Target};
	FText Reason;
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("安全提取凭证"), Ticket)) return false;
	TestTrue(TEXT("待提交来源仍持有原实例"), Source->GetItem() == Item);
	TestEqual(TEXT("提取尚未修改数量"), Item->ItemCount(), 6);
	int32 Notifications = 0;
	Source->OnItemDataChanged.AddLambda([&](ULxItemBase*) { ++Notifications; });
	Target->OnItemDataChanged.AddLambda([&](ULxItemBase*) { ++Notifications; });
	TestFalse(TEXT("不足以容纳整堆"), LxSafeItemOperations::CanAccept(Ticket, Targets, Reason));
	TestFalse(TEXT("失败不能部分合并"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	TestEqual(TEXT("失败来源不变"), Item->ItemCount(), 6);
	TestEqual(TEXT("失败目标不变"), Existing->ItemCount(), Max - 3);
	TestEqual(TEXT("失败不通知数据变化"), Notifications, 0);
	Ticket->Cancel();
	TestTrue(TEXT("取消凭证终结"), Ticket->IsCompleted());
	Targets.Add(Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 1));
	TestFalse(TEXT("取消凭证不能重放"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	Item->SetItemCountSafely(5);
	TestFalse(TEXT("数量变化拒绝旧凭证"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	TestEqual(TEXT("拒绝保留当前数量"), Item->ItemCount(), 5);
	Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	ULxItemBase* Replacement = Fixture.MakeItem(5);
	Source->SetItem(Replacement);
	TestFalse(TEXT("相同属性不同实例仍拒绝旧凭证"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	TestTrue(TEXT("替换后的来源实例不变"), Source->GetItem() == Replacement);
	return true;
}

/** 验证整堆接收优先堆叠、余量保留原实例且任何通知读取最终状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemTransferTest, "LxARPG.Items.SafeOperations.AtomicTransfer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemTransferTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	ULxItemBase* Item = Fixture.MakeItem(6);
	if (!TestNotNull(TEXT("创建来源物品"), Item)) return false;
	const int32 Max = Item->ItemInformation().ItemCountMax;
	if (!TestTrue(TEXT("堆叠上限满足测试"), Max >= 6)) return false;
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Item);
	ULxItemBase* Existing = Fixture.MakeItem(Max - 3);
	ULxItemSlotData* Target = Fixture.MakeSlot(ELxItemSlotType::Warehouse, Existing);
	ULxItemSlotData* Empty = Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 1);
	TArray<TObjectPtr<ULxItemSlotData>> Targets = {Target, Empty};
	FText Reason;
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("建立整堆凭证"), Ticket)) return false;
	bool bAllNotificationsFinal = true;
	int32 UseEvents = 0;
	/** 任意通知到来时来源、目标和凭证都已经完成。 */
	auto Observe = [&](ULxItemBase*)
	{
		bAllNotificationsFinal &= Source->GetItem() == nullptr && Existing->ItemCount() == Max
			&& Empty->GetItem() == Item && Item->ItemCount() == 3 && Ticket->bCompleted;
	};
	Source->OnItemDataChanged.AddLambda(Observe);
	Target->OnItemDataChanged.AddLambda(Observe);
	Empty->OnItemDataChanged.AddLambda(Observe);
	Item->OnItemUseSucceeded.AddLambda([&](ULxItemBase*) { ++UseEvents; });
	TestTrue(TEXT("预检查不修改数据"), LxSafeItemOperations::CanAccept(Ticket, Targets, Reason));
	TestTrue(TEXT("整堆成功接收"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	TestTrue(TEXT("全部通知只见最终状态"), bAllNotificationsFinal);
	TestEqual(TEXT("已有堆叠补满"), Existing->ItemCount(), Max);
	TestTrue(TEXT("余量仍为原实例"), Empty->GetItem() == Item);
	TestEqual(TEXT("余量数量正确"), Item->ItemCount(), 3);
	TestFalse(TEXT("来源清空"), Source->IsValid());
	TestEqual(TEXT("转移不触发使用"), UseEvents, 0);
	TestFalse(TEXT("成功凭证不能重放"), LxSafeItemOperations::Accept(Ticket, Targets, Reason));
	return true;
}

/** 验证拆分只放独立空格，满堆可拆且后续仍可普通堆叠。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemSplitTest, "LxARPG.Items.SafeOperations.IndependentSplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemSplitTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	ULxItemBase* Item = Fixture.MakeItem(6);
	if (!TestNotNull(TEXT("创建可拆材料"), Item)) return false;
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Item);
	ULxItemBase* PeerItem = Fixture.MakeItem(2);
	ULxItemSlotData* Peer = Fixture.MakeSlot(ELxItemSlotType::Backpack, PeerItem, 1);
	ULxItemSlotData* Empty = Fixture.MakeSlot(ELxItemSlotType::Backpack, nullptr, 2);
	TArray<TObjectPtr<ULxItemSlotData>> Slots = {Source, Peer};
	FText Reason;
	TestTrue(TEXT("拆分窗口资格不依赖空格"), LxSafeItemOperations::CanSplitItem(Source, Reason));
	TestFalse(TEXT("没有独立格时不合并同类堆叠"), LxSafeItemOperations::Split(Source, Slots, 2, Reason));
	TestEqual(TEXT("失败来源不变"), Item->ItemCount(), 6);
	TestEqual(TEXT("失败其他堆叠不变"), PeerItem->ItemCount(), 2);
	Slots.Add(Empty);
	TestFalse(TEXT("零数量非法"), LxSafeItemOperations::Split(Source, Slots, 0, Reason));
	TestFalse(TEXT("不允许拆出全部"), LxSafeItemOperations::Split(Source, Slots, 6, Reason));
	TestTrue(TEXT("拆到独立格"), LxSafeItemOperations::Split(Source, Slots, 2, Reason));
	TestEqual(TEXT("来源保留4个"), Item->ItemCount(), 4);
	TestEqual(TEXT("其他堆叠未改变"), PeerItem->ItemCount(), 2);
	if (!TestNotNull(TEXT("新格有独立物品"), Empty->GetItem())) return false;
	TestTrue(TEXT("普通格不共用实例"), Empty->GetItem() != Item);
	TestEqual(TEXT("新格数量正确"), Empty->GetItem()->ItemCount(), 2);
	TestTrue(TEXT("后续正常拖放仍可堆叠"), Peer->ItemEnterToThis(Empty) == ELxItemSlotDropResult::StackedAll);
	TestEqual(TEXT("后续堆叠合并数量"), PeerItem->ItemCount(), 4);
	Item->SetItemCountSafely(Item->ItemInformation().ItemCountMax);
	TestFalse(TEXT("满堆不能继续接收"), Item->ItemIsStackable());
	TestTrue(TEXT("满堆仍允许拆分"), LxSafeItemOperations::Split(Source, Slots, 1, Reason));
	TestEqual(TEXT("满堆拆出的独立数量"), Empty->GetItem()->ItemCount(), 1);
	return true;
}

/** 验证同ID不同词条不会混合，拆分完整保留实例属性。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemAttributesTest, "LxARPG.Items.SafeOperations.InstanceAttributes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemAttributesTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	const TArray<FGameplayTag> IDs = LxEntryConfig::GetEquipmentForgeEntryIDs();
	if (!TestTrue(TEXT("存在有效词条配置"), !IDs.IsEmpty())) return false;
	ULxItemBase* Item = Fixture.MakeItem(6);
	if (!TestNotNull(TEXT("创建带词条物品"), Item)) return false;
	FLxEntryQuote Quote;
	Quote.EntryID = IDs[0]; Quote.EntryProportion = 1.25f; Quote.EntryCD = 0.5f;
	if (!TestTrue(TEXT("设置实例词条"), Item->RestoreItemEntries({FLxItemEntryConfig(Quote)}))) return false;
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Item);
	ULxItemSlotData* Plain = Fixture.MakeSlot(ELxItemSlotType::Warehouse, Fixture.MakeItem(1));
	ULxItemSlotData* Empty = Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 1);
	FText Reason;
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("提取实例属性"), Ticket)) return false;
	TestFalse(TEXT("不同词条不能仅按ID合并"), LxSafeItemOperations::CanAccept(Ticket, {Plain}, Reason));
	TestTrue(TEXT("不同词条可独立放空格"), LxSafeItemOperations::Accept(Ticket, {Plain, Empty}, Reason));
	TestTrue(TEXT("转移保留原实例"), Empty->GetItem() == Item);
	ULxItemSlotData* SplitEmpty = Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 2);
	TestTrue(TEXT("拆出新实例"), LxSafeItemOperations::Split(Empty, {Plain, Empty, SplitEmpty}, 2, Reason));
	FLxItemSaveRecord Record;
	if (!TestTrue(TEXT("新实例可采集"), LxItemSaveData::CaptureItem(SplitEmpty->GetItem(), Record))) return false;
	TestEqual(TEXT("保留完整词条数量"), Record.Entries.Num(), 1);
	if (Record.Entries.Num() != 1) return false;
	TestEqual(TEXT("保留生效比例"), Record.Entries[0].EntryQuote.EntryProportion, 1.25f);
	TestEqual(TEXT("保留冷却参数"), Record.Entries[0].EntryQuote.EntryCD, 0.5f);
	return true;
}

/** 验证完整装备身份与强化状态，并排除技能、Buff和展示引用槽。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemTypeTest, "LxARPG.Items.SafeOperations.TypeAndEquipmentRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemTypeTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	const auto& Configs = LxItemConfig::GetEquipmentItemMap();
	if (!TestTrue(TEXT("存在真实装备"), !Configs.IsEmpty())) return false;
	const FGameplayTag ID = Configs.CreateConstIterator().Key();
	ULxEquipment* Equipment = Cast<ULxEquipment>(Fixture.MakeItem(1, ID));
	if (!TestNotNull(TEXT("创建装备实例"), Equipment)) return false;
	if (!TestTrue(TEXT("修改强化状态"), Equipment->ApplyCraftingState(0.37f, Equipment->GetNormalEntries()))) return false;
	const FGuid OriginalID = Equipment->GetInstanceID();
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Equipment);
	ULxItemSlotData* Existing = Fixture.MakeSlot(ELxItemSlotType::Warehouse, Fixture.MakeItem(1, ID));
	ULxItemSlotData* Empty = Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 1);
	FText Reason;
	TestFalse(TEXT("装备不能拆分"), LxSafeItemOperations::CanSplitItem(Source, Reason));
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("装备允许完整发送"), Ticket)) return false;
	TestFalse(TEXT("同ID装备不能合并或交换"), LxSafeItemOperations::Accept(Ticket, {Existing}, Reason));
	TestTrue(TEXT("装备完整放入空格"), LxSafeItemOperations::Accept(Ticket, {Existing, Empty}, Reason));
	TestTrue(TEXT("保留原装备对象"), Empty->GetItem() == Equipment);
	TestEqual(TEXT("装备身份不变"), Equipment->GetInstanceID(), OriginalID);
	TestEqual(TEXT("强化状态不变"), Equipment->GetEnhancement(), 0.37f);
	for (ELxItemSlotType Type : {ELxItemSlotType::SkillDisplay, ELxItemSlotType::BuffDisplay,
		ELxItemSlotType::Shortcut, ELxItemSlotType::Transaction, ELxItemSlotType::Equipment})
		TestFalse(TEXT("展示及引用槽不支持"), LxSafeItemOperations::IsSupportedSlot(Fixture.MakeSlot(Type, Fixture.MakeItem(2))));
	if (!LxItemConfig::GetSkillItemMap().IsEmpty())
		TestFalse(TEXT("技能物品不支持"), LxSafeItemOperations::IsSupportedItem(Fixture.MakeItem(1, LxItemConfig::GetSkillItemMap().CreateConstIterator().Key())));
	if (!LxItemConfig::GetBuffItemMap().IsEmpty())
		TestFalse(TEXT("Buff物品不支持"), LxSafeItemOperations::IsSupportedItem(Fixture.MakeItem(1, LxItemConfig::GetBuffItemMap().CreateConstIterator().Key())));
	return true;
}

/** 验证数量变更不激活词条、最后一枚物品使用，以及移出后旧监听解除。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemUseNotificationTest, "LxARPG.Items.SafeOperations.UseAndBatchNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemUseNotificationTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	FGameplayTag ID;
	for (const auto& Pair : LxItemConfig::GetConsumableItemMap())
		if (Pair.Value.ItemCountMax > 1) { ID = Pair.Key; break; }
	if (!TestTrue(TEXT("存在可堆叠消耗品"), ID.IsValid())) return false;
	ULxItemBase* Item = Fixture.MakeItem(2, ID);
	if (!TestNotNull(TEXT("创建消耗品"), Item)) return false;
	FLxItemSlotSaveRecord Saved;
	Saved.SlotIndex = 0; Saved.SlotTag = LxTag_Item;
	if (!TestTrue(TEXT("采集实例"), LxItemSaveData::CaptureItem(Item, Saved.Item))) return false;
	ULxCharacterBackpackModule* Backpack = NewObject<ULxCharacterBackpackModule>(Fixture.Instance.Get());
	if (!TestTrue(TEXT("创建隔离背包"), Backpack->RestoreBackpackSaveData(2, {Saved}))) return false;
	int32 Uses = 0, Changes = 0;
	Backpack->OnItemUsed.AddLambda([&](ULxItemBase*) { ++Uses; });
	Backpack->OnDataChange.AddLambda([&]() { ++Changes; });
	FText Reason;
	TestTrue(TEXT("数据模块拆分"), Backpack->SplitItemStack(0, 1, Reason));
	TestEqual(TEXT("拆分不激活使用效果"), Uses, 0);
	TestEqual(TEXT("批量操作仅刷新一次"), Changes, 1);
	Backpack->GetBackpackSlotAt(0)->EndUseItem();
	TestEqual(TEXT("最后一枚消耗品仍激活一次"), Uses, 1);
	TestFalse(TEXT("使用清空原槽"), Backpack->GetBackpackSlotAt(0)->IsValid());
	ULxItemTransferTicket* Ticket = Backpack->BeginSafeExtractItem(1, Reason);
	if (!TestNotNull(TEXT("提取剩余实例"), Ticket)) return false;
	ULxItemSlotData* Warehouse = Fixture.MakeSlot(ELxItemSlotType::Warehouse);
	if (!TestTrue(TEXT("安全移出背包"), LxSafeItemOperations::Accept(Ticket, {Warehouse}, Reason))) return false;
	const int32 ChangesAfterMove = Changes;
	Warehouse->GetItem()->SetItemCountSafely(2);
	TestEqual(TEXT("目标变化不回调旧背包"), Changes, ChangesAfterMove);
	TestEqual(TEXT("数量调整不激活使用"), Uses, 1);
	return true;
}

/** 验证不同真实模块之间迁移实例归属，源模块回收后目标仍保留完整装备及词条。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemOwnershipTest, "LxARPG.Items.SafeOperations.OwnershipLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemOwnershipTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	const auto& Configs = LxItemConfig::GetEquipmentItemMap();
	if (!TestTrue(TEXT("存在装备配置"), !Configs.IsEmpty())) return false;
	ULxCharacterBackpackModule* SourceOwner = NewObject<ULxCharacterBackpackModule>(Fixture.Instance.Get());
	TWeakObjectPtr<ULxCharacterBackpackModule> PreviousOwner(SourceOwner);
	ULxWarehouseInteractionComponent* TargetOwner = NewObject<ULxWarehouseInteractionComponent>(Fixture.Instance.Get());
	ULxItemBase* NameCollision = Fixture.MakeItem(1, Configs.CreateConstIterator().Key(), TargetOwner);
	ULxEquipment* Equipment = Cast<ULxEquipment>(Fixture.MakeItem(1, Configs.CreateConstIterator().Key(), SourceOwner));
	if (!TestNotNull(TEXT("创建真实背包归属的装备"), Equipment)) return false;
	if (!TestNotNull(TEXT("创建目标同名冲突物品"), NameCollision)) return false;
	const FString CollisionName = Equipment->GetName();
	if (!TestTrue(TEXT("构造跨Outer的相同对象名"), NameCollision->Rename(*CollisionName, TargetOwner,
		REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty))) return false;
	if (!TestTrue(TEXT("准备强化状态"), Equipment->ApplyCraftingState(0.37f, Equipment->GetNormalEntries()))) return false;
	const FGuid Identity = Equipment->GetInstanceID();
	const TArray<TObjectPtr<ULxEntryObjectBase>> Entries = Equipment->GetItemEntryList();
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Equipment, 0, SourceOwner);
	TStrongObjectPtr<ULxItemSlotData> Target(Fixture.MakeSlot(ELxItemSlotType::Warehouse, nullptr, 0, TargetOwner));
	FText Reason;
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("建立装备凭证"), Ticket)) return false;
	if (!TestTrue(TEXT("跨模块完整接收装备"), LxSafeItemOperations::Accept(Ticket, {Target.Get()}, Reason))) return false;
	TestTrue(TEXT("实例归属迁移到仓库模块"), Equipment->GetOuter() == TargetOwner);
	TestTrue(TEXT("目标已有同名物品时自动生成不冲突的对象名"), Equipment->GetFName() != NameCollision->GetFName());
	TestTrue(TEXT("目标仍持有同一装备对象"), Target->GetItem() == Equipment);
	for (ULxEntryObjectBase* Entry : Entries)
		TestTrue(TEXT("词条仍以原装备对象为父级"), Entry && Entry->GetOuter() == Equipment);
	CollectGarbage(RF_NoFlags, true);
	TestFalse(TEXT("旧来源模块可以正常回收"), PreviousOwner.IsValid());
	if (!TestTrue(TEXT("目标装备在来源回收后仍有效"), IsValid(Target->GetItem()))) return false;
	TestEqual(TEXT("回收后装备身份不变"), Equipment->GetInstanceID(), Identity);
	TestEqual(TEXT("回收后强化度不变"), Equipment->GetEnhancement(), 0.37f);
	TestEqual(TEXT("回收后词条数量不变"), Equipment->GetItemEntryList().Num(), Entries.Num());
	return true;
}

/** 验证格子与模块最终通知中的安全、拖放、使用和计数写入均拒绝重入。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSafeItemReentrancyTest, "LxARPG.Items.SafeOperations.ReentrantNotifications",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSafeItemReentrancyTest::RunTest(const FString& Parameters)
{
	FSafeItemFixture Fixture;
	ULxItemBase* Initial = Fixture.MakeItem(6);
	if (!TestNotNull(TEXT("创建材料实例"), Initial)) return false;
	FLxItemSlotSaveRecord Saved;
	Saved.SlotIndex = 0; Saved.SlotTag = LxTag_Item;
	if (!TestTrue(TEXT("采集来源材料"), LxItemSaveData::CaptureItem(Initial, Saved.Item))) return false;
	ULxCharacterBackpackModule* Backpack = NewObject<ULxCharacterBackpackModule>(Fixture.Instance.Get());
	if (!TestTrue(TEXT("建立真实模块槽位"), Backpack->RestoreBackpackSaveData(3, {Saved}))) return false;
	bool bAttempted = false, bNestedAccepted = false, bDirectCountAccepted = false, bAddAccepted = false;
	int32 Changes = 0;
	Backpack->OnDataChange.AddLambda([&]()
	{
		++Changes;
		if (bAttempted) return;
		bAttempted = true;
		FText NestedReason;
		bNestedAccepted = Backpack->SplitItemStack(0, 1, NestedReason);
		bDirectCountAccepted = Backpack->GetBackpackSlotAt(0)->GetItem()->SetItemCountSafely(1);
		bAddAccepted = Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 1)});
	});
	FText Reason;
	TestTrue(TEXT("外层拆分成功"), Backpack->SplitItemStack(0, 2, Reason));
	TestTrue(TEXT("模块通知确实尝试重入"), bAttempted);
	TestFalse(TEXT("最终模块通知仍拒绝拆分重入"), bNestedAccepted);
	TestFalse(TEXT("最终通知拒绝直接计数覆盖"), bDirectCountAccepted);
	TestFalse(TEXT("最终通知拒绝常规添加重入"), bAddAccepted);
	TestEqual(TEXT("没有递归刷新或额外变更"), Changes, 1);
	TestEqual(TEXT("外层来源保留正确数量"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 4);
	TestTrue(TEXT("作用域退出后锁已释放"), Backpack->SplitItemStack(0, 1, Reason));

	ULxItemBase* Incoming = Fixture.MakeItem(6);
	const int32 Max = Incoming->ItemInformation().ItemCountMax;
	if (!TestTrue(TEXT("堆叠上限满足验证"), Max >= 6)) return false;
	ULxItemSlotData* Source = Fixture.MakeSlot(ELxItemSlotType::Backpack, Incoming);
	ULxItemSlotData* First = Fixture.MakeSlot(ELxItemSlotType::Warehouse, Fixture.MakeItem(Max - 3));
	ULxItemBase* SecondItem = Fixture.MakeItem(Max - 3);
	ULxItemSlotData* Second = Fixture.MakeSlot(ELxItemSlotType::Warehouse, SecondItem, 1);
	ULxItemSlotData* Other = Fixture.MakeSlot(ELxItemSlotType::Backpack, nullptr, 1);
	bool bClearBlocked = false, bExtractBlocked = false, bDropBlocked = false;
	First->OnItemDataChanged.AddLambda([&](ULxItemBase*)
	{
		Second->ClearItem();
		bClearBlocked = Second->GetItem() == SecondItem;
		FText NestedReason;
		bExtractBlocked = LxSafeItemOperations::BeginSafeExtract(Second, NestedReason) == nullptr;
		bDropBlocked = Other->ItemEnterToThis(Second) == ELxItemSlotDropResult::CannotEnter;
	});
	ULxItemTransferTicket* Ticket = LxSafeItemOperations::BeginSafeExtract(Source, Reason);
	if (!TestNotNull(TEXT("建立双目标堆叠凭证"), Ticket)) return false;
	TestTrue(TEXT("外层双堆叠完整接收"), LxSafeItemOperations::Accept(Ticket, {First, Second}, Reason));
	TestTrue(TEXT("前一个格子通知不能清空后一个计划格"), bClearBlocked);
	TestTrue(TEXT("格子通知拒绝再次提取"), bExtractBlocked);
	TestTrue(TEXT("格子通知拒绝普通拖放"), bDropBlocked);
	TestEqual(TEXT("后一个堆叠数量完整"), SecondItem->ItemCount(), Max);
	TestFalse(TEXT("重入失败不会把物品移入其他容器"), Other->IsValid());
	FGameplayTag ConsumableID;
	for (const auto& Pair : LxItemConfig::GetConsumableItemMap())
		if (Pair.Value.ItemCountMax > 1) { ConsumableID = Pair.Key; break; }
	if (!TestTrue(TEXT("存在可堆叠消耗品"), ConsumableID.IsValid())) return false;
	ULxItemBase* Consumable = Fixture.MakeItem(2, ConsumableID);
	if (!TestNotNull(TEXT("创建用于重入使用验证的消耗品"), Consumable)) return false;
	ULxItemSlotData* ConsumableSource = Fixture.MakeSlot(ELxItemSlotType::Backpack, Consumable);
	ULxItemSlotData* ConsumableTarget = Fixture.MakeSlot(ELxItemSlotType::Warehouse);
	ULxItemSlotData* Shortcut = Fixture.MakeSlot(ELxItemSlotType::Shortcut, Consumable, 0,
		NewObject<ULxItemTransferTicket>(Fixture.Instance.Get()));
	int32 Uses = 0;
	bool bShortcutBusyDuringNotification = false;
	ELxItemUseState DirectUseResult = ELxItemUseState::ActivateEntry;
	Consumable->OnItemUseSucceeded.AddLambda([&](ULxItemBase*) { ++Uses; });
	ConsumableTarget->OnItemDataChanged.AddLambda([&](ULxItemBase*)
	{
		bShortcutBusyDuringNotification = LxSafeItemOperations::IsSlotBusy(Shortcut);
		ConsumableTarget->EndUseItem();
		Shortcut->StartUseItem();
		Shortcut->EndUseItem();
		DirectUseResult = Consumable->ItemUse();
	});
	Ticket = LxSafeItemOperations::BeginSafeExtract(ConsumableSource, Reason);
	if (!TestNotNull(TEXT("建立消耗品转移凭证"), Ticket)) return false;
	TestTrue(TEXT("通知中尝试使用仍可完整完成转移"), LxSafeItemOperations::Accept(Ticket, {ConsumableTarget}, Reason));
	TestEqual(TEXT("通知回调不能消耗已提交的物品"), Consumable->ItemCount(), 2);
	TestEqual(TEXT("通知回调不能激活使用词条"), Uses, 0);
	TestTrue(TEXT("不同父级的快捷引用格仍遵守真实物品的容器锁"), bShortcutBusyDuringNotification);
	TestEqual(TEXT("直接调用消耗品对象也拒绝通知重入使用"), DirectUseResult, ELxItemUseState::Failed);
	TestFalse(TEXT("转移完成后快捷引用格解除事务阻挡"), LxSafeItemOperations::IsSlotBusy(Shortcut));
	return true;
}

#endif
