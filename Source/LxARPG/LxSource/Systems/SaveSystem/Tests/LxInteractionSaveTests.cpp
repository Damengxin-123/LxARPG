#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveComponent.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"

/** 自动化测试专用宽字符标签，确保中文标识经过存档序列化后仍可正确解析。 */
UE_DEFINE_GAMEPLAY_TAG_STATIC(LxTag_Test_InteractionSave, TEXT("测试.存档.交互对象"));
/** 仅用于验证商人更换商品后不会继承原商品库存。 */
UE_DEFINE_GAMEPLAY_TAG_STATIC(LxTag_Test_InteractionReplacementItem, TEXT("物品.材料.测试交互替换商品"));

namespace
{
	/** 创建具有稳定标识的测试节点。 */
	ULxInteractionTreeNodeData* AddInteractionSaveTestNode(ULxInteractionTreeAsset* Asset, ELxInteractionActionType Type)
	{
		ULxInteractionTreeNodeData* Node = NewObject<ULxInteractionTreeNodeData>(Asset);
		Node->NodeId = FGuid::NewGuid();
		Node->Type = Type;
		Asset->Nodes.Add(Node);
		return Node;
	}

	/** 按静态标识查找功能，与运行时遍历次序无关。 */
	ULxInteractionActionComponentBase* FindInteractionSaveTestFeature(ULxInteractableComponent* Provider, const FGuid& ID)
	{
		for (ULxInteractionActionComponentBase* Feature : Provider->GetInteractionFeatures())
			if (Feature && Feature->GetOwnerInteractionNode()->GetPersistentNodeID() == ID) return Feature;
		return nullptr;
	}

	/** 创建不连接游戏实例和磁盘的内存测试适配器。 */
	ULxInteractionSaveComponent* CreateInteractionSaveTestAdapter(ULxInteractableComponent* Provider)
	{
		Provider->InteractionIDTag = LxTag_Test_InteractionSave;
		ULxInteractionSaveComponent* Adapter = NewObject<ULxInteractionSaveComponent>();
		Adapter->SetInteractableComponent(Provider);
		return Adapter;
	}

	/** 选取实际配置中的有效词条，供测试验证物品按预设重新创建。 */
	bool FindInteractionSaveTestEntry(FLxItemEntryConfig& OutEntry)
	{
		for (const auto& Pair : LxItemConfig::GetEquipmentItemMap())
		{
			for (const FLxItemEntryConfig& Entry : Pair.Value.ItemEntryConfigs)
			{
				if (LxEntryConfig::GetEntryData(Entry.EntryQuote.EntryID))
				{
					OutEntry = Entry;
					return true;
				}
			}
		}
		return false;
	}

	/** 将新版物品槽位转成旧记录，并加入无效实例词条以验证兼容读取只使用标识和数量。 */
	FLxInteractionFeatureSaveRecord MakeLegacyInteractionRecord(const FLxInteractionFeatureSaveRecord& Record)
	{
		FLxInteractionFeatureSaveRecord Legacy = Record;
		Legacy.DataVersion = 0;
		Legacy.Slots.Reset();
		for (const FLxInteractionItemSaveRecord& ItemSlot : Record.ItemSlots)
		{
			FLxItemSlotSaveRecord& Slot = Legacy.Slots.AddDefaulted_GetRef();
			Slot.SlotIndex = ItemSlot.SlotIndex;
			Slot.SlotTag = LxTag_Item;
			Slot.Item.ItemIDTag = ItemSlot.ItemIDTag;
			Slot.Item.ItemCount = ItemSlot.ItemCount;
			if (ItemSlot.ItemCount > 0) Slot.Item.Entries.AddDefaulted();
		}
		Legacy.ItemSlots.Reset();
		return Legacy;
	}
}

/** 防止同类功能调序后因运行时序号变化而互相串档。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionSaveStableIdentityTest,
	"LxARPG.Save.InteractionStableIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 保存两个不同机关状态，交换树中顺序后恢复。 */
bool FLxInteractionSaveStableIdentityTest::RunTest(const FString& Parameters)
{
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableTriggerMechanism = true;
	Asset->Features.MultipleNodeTypes.Add(ELxInteractionActionType::TriggerMechanism);
	ULxInteractionTreeNodeData* Root = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::Entrance);
	ULxInteractionTreeNodeData* First = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::TriggerMechanism);
	ULxInteractionTreeNodeData* Second = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::TriggerMechanism);
	Root->Children = {First->NodeId, Second->NodeId};
	Asset->Roots = {Root->NodeId};
	ULxInteractableComponent* Original = NewObject<ULxInteractableComponent>();
	Original->FeatureConfig.bEnableTriggerMechanism = true;
	if (!TestTrue(TEXT("构建机关交互树"), Original->SetInteractionTreeAsset(Asset))) return false;
	ULxTriggerMechanismInteractionComponent* OriginalFirst = Cast<ULxTriggerMechanismInteractionComponent>(FindInteractionSaveTestFeature(Original, First->NodeId));
	ULxTriggerMechanismInteractionComponent* OriginalSecond = Cast<ULxTriggerMechanismInteractionComponent>(FindInteractionSaveTestFeature(Original, Second->NodeId));
	if (!TestNotNull(TEXT("第一机关"), OriginalFirst) || !TestNotNull(TEXT("第二机关"), OriginalSecond)) return false;
	OriginalFirst->SetMechanismState(ELxMechanismState::Opened);
	OriginalSecond->SetMechanismState(ELxMechanismState::CannotOpen);
	OriginalFirst->SetInteractionState(ELxInteractionDataState::Occupied);
	ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
	if (!TestTrue(TEXT("按对象标签采集"), CreateInteractionSaveTestAdapter(Original)->CaptureSaveData(Save))) return false;
	Root->Children = {Second->NodeId, First->NodeId};
	ULxInteractableComponent* Restored = NewObject<ULxInteractableComponent>();
	Restored->FeatureConfig = Original->FeatureConfig;
	if (!TestTrue(TEXT("交换顺序后重建"), Restored->SetInteractionTreeAsset(Asset))) return false;
	ULxTriggerMechanismInteractionComponent* RestoredFirst = Cast<ULxTriggerMechanismInteractionComponent>(FindInteractionSaveTestFeature(Restored, First->NodeId));
	ULxTriggerMechanismInteractionComponent* RestoredSecond = Cast<ULxTriggerMechanismInteractionComponent>(FindInteractionSaveTestFeature(Restored, Second->NodeId));
	if (!TestNotNull(TEXT("重建第一机关"), RestoredFirst) || !TestNotNull(TEXT("重建第二机关"), RestoredSecond)) return false;
	TestNotEqual(TEXT("运行时序号已经改变"), OriginalFirst->GetRuntimeNodeIndex(), RestoredFirst->GetRuntimeNodeIndex());
	TestTrue(TEXT("按稳定GUID恢复"), CreateInteractionSaveTestAdapter(Restored)->RestoreSaveData(Save));
	TestEqual(TEXT("第一机关仍为开启"), RestoredFirst->GetMechanismState(), ELxMechanismState::Opened);
	TestEqual(TEXT("第二机关仍不可开启"), RestoredSecond->GetMechanismState(), ELxMechanismState::CannotOpen);
	TestEqual(TEXT("不恢复上次运行的占用"), RestoredFirst->GetInteractionState(), ELxInteractionDataState::Interactable);
	FLxInteractionFeatureSaveRecord InvalidRecord = Save->Interactions.FindChecked(LxTag_Test_InteractionSave).Features.FindChecked(First->NodeId);
	InvalidRecord.InteractionType = ELxInteractionActionType::Warehouse;
	TestFalse(TEXT("功能类型不同的旧档被拒绝"), RestoredFirst->RestorePersistentData(InvalidRecord));
	return true;
}

/** 验证容器只持久化预设物品数量、有限库存与槽位位置。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionSaveContainersTest,
	"LxARPG.Save.InteractionContainers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 在独立物品配置下测试容器往返，并确保损坏存档不会破坏现有物品。 */
bool FLxInteractionSaveContainersTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); };
	FLxItemEntryConfig DefaultEntry;
	if (!TestTrue(TEXT("存在用于预设物品的有效词条"), FindInteractionSaveTestEntry(DefaultEntry))) return false;
	const auto EquipmentBackup = LxItemConfig::GetEquipmentItemMap();
	const auto ConsumableBackup = LxItemConfig::GetConsumableItemMap();
	const auto MaterialBackup = LxItemConfig::GetMaterialItemMap();
	const auto BuffBackup = LxItemConfig::GetBuffItemMap();
	const auto SkillBackup = LxItemConfig::GetSkillItemMap();
	ON_SCOPE_EXIT
	{
		LxItemConfig::ClearItemConfig();
		for (const auto& Pair : EquipmentBackup) LxItemConfig::SetEquipmentItemData(Pair.Value);
		for (const auto& Pair : ConsumableBackup) LxItemConfig::SetConsumableItemData(Pair.Value);
		for (const auto& Pair : MaterialBackup) LxItemConfig::SetMaterialItemData(Pair.Value);
		for (const auto& Pair : BuffBackup) LxItemConfig::SetBuffItemData(Pair.Value);
		for (const auto& Pair : SkillBackup) LxItemConfig::SetSkillItemData(Pair.Value);
	};
	FLxMaterialInformation Gold;
	Gold.ItemIDTag = LxTag_Item_Material_Currency_Gold;
	Gold.ItemType = ELxItemType::Material;
	Gold.ItemCountMax = 99;
	Gold.ItemEntryConfigs = {DefaultEntry};
	LxItemConfig::SetMaterialItemData(Gold);
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableTreasureChest = true;
	Asset->Features.bEnableWarehouse = true;
	Asset->Features.bEnableTradeContainer = true;
	ULxInteractionTreeNodeData* Root = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::Entrance);
	ULxInteractionTreeNodeData* ChestNode = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::TreasureChest);
	ULxInteractionTreeNodeData* WarehouseNode = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::Warehouse);
	ULxInteractionTreeNodeData* TradeNode = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::TradeContainer);
	Root->Children = {ChestNode->NodeId, WarehouseNode->NodeId, TradeNode->NodeId};
	Asset->Roots = {Root->NodeId};
	ULxInteractableComponent* Original = NewObject<ULxInteractableComponent>();
	Original->FeatureConfig.bEnableTreasureChest = true;
	Original->FeatureConfig.bEnableWarehouse = true;
	Original->FeatureConfig.bEnableTradeContainer = true;
	Original->FeatureConfig.TreasureChestConfig.ItemList = {FLxItemQuote(Gold.ItemIDTag, 5), FLxItemQuote(Gold.ItemIDTag, 7)};
	Original->FeatureConfig.WarehouseConfig.SlotCount = 4;
	FLxTradeItemConfig TradeItem;
	TradeItem.ItemIDTag = Gold.ItemIDTag;
	TradeItem.ItemCount = 9;
	FLxTradeItemConfig RemainingTradeItem = TradeItem;
	RemainingTradeItem.ItemCount = 11;
	FLxTradeItemConfig UnlimitedTradeItem = TradeItem;
	UnlimitedTradeItem.ItemCount = 19;
	UnlimitedTradeItem.bLimitedStock = false;
	Original->FeatureConfig.TradeContainerConfig.TradeItems = {TradeItem, RemainingTradeItem, UnlimitedTradeItem};
	Original->FeatureConfig.TradeContainerConfig.SellValueRate = 1.2f;
	Original->FeatureConfig.TradeContainerConfig.PurchaseValueRate = 0.8f;
	if (!TestTrue(TEXT("构建容器交互树"), Original->SetInteractionTreeAsset(Asset))) return false;
	ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(FindInteractionSaveTestFeature(Original, ChestNode->NodeId));
	ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(FindInteractionSaveTestFeature(Original, WarehouseNode->NodeId));
	ULxTradeContainerInteractionComponent* Trade = Cast<ULxTradeContainerInteractionComponent>(FindInteractionSaveTestFeature(Original, TradeNode->NodeId));
	if (!TestNotNull(TEXT("原宝箱"), Chest) || !TestNotNull(TEXT("原仓库"), Warehouse) || !TestNotNull(TEXT("原商店"), Trade)) return false;
	Chest->GetTreasureChestSlotAt(0)->ClearItem();
	Warehouse->GetWarehouseSlotAt(3)->SetItem(ULxItemBase::CreateItemObject(Warehouse, FLxItemQuote(Gold.ItemIDTag, 11)));
	// 运行时词条缓存不完整也不影响地图物品的数量采集，读档后由静态配置重建。
	Warehouse->GetWarehouseSlotAt(3)->GetItem()->GetItemEntryList().Reset();
	Trade->GetTradeSlotAt(0)->ClearItem();
	Trade->GetTradeSlotAt(1)->SetItem(ULxItemBase::CreateItemObject(Trade, FLxItemQuote(Gold.ItemIDTag, 3)));
	Trade->GetTradeSlotAt(2)->ClearItem();
	Trade->SetTradeItemValueRate(1.75f);
	Trade->SetPurchaseValueRate(0.25f);
	ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
	if (!TestTrue(TEXT("采集全部容器"), CreateInteractionSaveTestAdapter(Original)->CaptureSaveData(Save))) return false;
	const auto& SavedFeatures = Save->Interactions.FindChecked(LxTag_Test_InteractionSave).Features;
	const FLxInteractionFeatureSaveRecord& SavedTrade = SavedFeatures.FindChecked(TradeNode->NodeId);
	TestEqual(TEXT("新存档标记精简物品格式"), SavedTrade.DataVersion, 1);
	if (!TestEqual(TEXT("商店仅保存两项有限库存"), SavedTrade.ItemSlots.Num(), 2)) return false;
	TestEqual(TEXT("售罄商品保留配置标识"), SavedTrade.ItemSlots[0].ItemIDTag, Gold.ItemIDTag);
	TestEqual(TEXT("售罄商品保存零数量"), SavedTrade.ItemSlots[0].ItemCount, 0);
	TestEqual(TEXT("有限库存保存剩余数量"), SavedTrade.ItemSlots[1].ItemCount, 3);
	TestTrue(TEXT("商店不写入旧实例记录"), SavedTrade.Slots.IsEmpty());
	TestTrue(TEXT("仓库不写入旧实例记录"), SavedFeatures.FindChecked(WarehouseNode->NodeId).Slots.IsEmpty());
	TestTrue(TEXT("宝箱不写入旧实例记录"), SavedFeatures.FindChecked(ChestNode->NodeId).Slots.IsEmpty());
	TestEqual(TEXT("不采集运行时售卖倍率"), SavedTrade.TradeItemValueRate, 1.0f);
	TestEqual(TEXT("不采集运行时收购倍率"), SavedTrade.PurchaseValueRate, 1.0f);
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("交互快照可真实序列化"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
	Save = Cast<ULxGameSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
	if (!TestNotNull(TEXT("读取序列化后的交互快照"), Save)) return false;
	if (!TestTrue(TEXT("序列化后仍能按原中文交互标签索引"), Save->Interactions.Contains(LxTag_Test_InteractionSave))) return false;
	TestEqual(TEXT("序列化后记录内部标识保持一致"), Save->Interactions.FindChecked(LxTag_Test_InteractionSave).InteractionIDTag,
		LxTag_Test_InteractionSave.GetTag());
	ULxInteractableComponent* Restored = NewObject<ULxInteractableComponent>();
	Restored->FeatureConfig = Original->FeatureConfig;
	if (!TestTrue(TEXT("重建默认容器"), Restored->SetInteractionTreeAsset(Asset))) return false;
	Chest = Cast<ULxTreasureChestInteractionComponent>(FindInteractionSaveTestFeature(Restored, ChestNode->NodeId));
	Warehouse = Cast<ULxWarehouseInteractionComponent>(FindInteractionSaveTestFeature(Restored, WarehouseNode->NodeId));
	Trade = Cast<ULxTradeContainerInteractionComponent>(FindInteractionSaveTestFeature(Restored, TradeNode->NodeId));
	if (!TestNotNull(TEXT("新宝箱"), Chest) || !TestNotNull(TEXT("新仓库"), Warehouse) || !TestNotNull(TEXT("新商店"), Trade)) return false;
	if (!TestTrue(TEXT("恢复全部容器"), CreateInteractionSaveTestAdapter(Restored)->RestoreSaveData(Save))) return false;
	TestNull(TEXT("已经拿走的宝箱槽保持为空"), Chest->GetTreasureChestSlotAt(0)->GetItem());
	if (!TestNotNull(TEXT("未拿走的宝箱物品"), Chest->GetTreasureChestSlotAt(1)->GetItem())) return false;
	TestEqual(TEXT("宝箱剩余数量"), static_cast<int32>(Chest->GetTreasureChestSlotAt(1)->GetItem()->ItemCount()), 7);
	TestNull(TEXT("商店售罄不会补货"), Trade->GetTradeSlotAt(0)->GetItem());
	if (!TestNotNull(TEXT("有限剩余库存"), Trade->GetTradeSlotAt(1)->GetItem())
		|| !TestNotNull(TEXT("无限库存按配置重新创建"), Trade->GetTradeSlotAt(2)->GetItem())) return false;
	TestEqual(TEXT("有限库存数量恢复"), static_cast<int32>(Trade->GetTradeSlotAt(1)->GetItem()->ItemCount()), 3);
	TestEqual(TEXT("无限库存恢复预设数量"), static_cast<int32>(Trade->GetTradeSlotAt(2)->GetItem()->ItemCount()), 19);
	TestEqual(TEXT("售卖倍率使用当前配置"), Trade->GetTradeItemValueRate(), 1.2f);
	TestEqual(TEXT("收购倍率使用当前配置"), Trade->GetPurchaseValueRate(), 0.8f);
	TestNull(TEXT("仓库空槽保留"), Warehouse->GetWarehouseSlotAt(0)->GetItem());
	if (!TestNotNull(TEXT("仓库原位置物品"), Warehouse->GetWarehouseSlotAt(3)->GetItem())) return false;
	TestEqual(TEXT("仓库数量恢复"), static_cast<int32>(Warehouse->GetWarehouseSlotAt(3)->GetItem()->ItemCount()), 11);
	if (!TestEqual(TEXT("仓库重新创建默认词条"), Warehouse->GetWarehouseSlotAt(3)->GetItem()->GetItemEntryList().Num(), 1)
		|| !TestEqual(TEXT("宝箱重新创建默认词条"), Chest->GetTreasureChestSlotAt(1)->GetItem()->GetItemEntryList().Num(), 1)) return false;
	TestEqual(TEXT("仓库词条采用预设比例"), Warehouse->GetWarehouseSlotAt(3)->GetItem()->GetItemEntryList()[0]->GetEntryQuote().EntryProportion,
		DefaultEntry.EntryQuote.EntryProportion);
	TestEqual(TEXT("恢复中途不误触宝箱完成"), Chest->GetInteractionState(), ELxInteractionDataState::Interactable);

	// 模拟槽位快照先于节点配置到达：先建两个空槽，再恢复剩余物品时不得提前完成。
	ULxInteractableComponent* ReplicatedProvider = NewObject<ULxInteractableComponent>();
	ReplicatedProvider->FeatureConfig = Original->FeatureConfig;
	ReplicatedProvider->FeatureConfig.TreasureChestConfig.ItemList.Reset();
	if (!TestTrue(TEXT("构建等待网络槽位的宝箱"), ReplicatedProvider->SetInteractionTreeAsset(Asset))) return false;
	ULxTreasureChestInteractionComponent* ReplicatedChest = Cast<ULxTreasureChestInteractionComponent>(FindInteractionSaveTestFeature(ReplicatedProvider, ChestNode->NodeId));
	FArrayProperty* SlotsProperty = FindFProperty<FArrayProperty>(ULxTreasureChestInteractionComponent::StaticClass(), TEXT("ReplicatedTreasureChestSlots"));
	if (!TestNotNull(TEXT("网络宝箱"), ReplicatedChest) || !TestNotNull(TEXT("复制槽位属性"), SlotsProperty)) return false;
	*SlotsProperty->ContainerPtrToValuePtr<TArray<FLxItemQuote>>(ReplicatedChest) = {FLxItemQuote(), FLxItemQuote(Gold.ItemIDTag, 7)};
	ReplicatedChest->ProcessEvent(ReplicatedChest->FindFunctionChecked(TEXT("OnRep_TreasureChestSlots")), nullptr);
	TestEqual(TEXT("网络整批恢复不误触完成"), ReplicatedChest->GetInteractionState(), ELxInteractionDataState::Interactable);
	TestNotNull(TEXT("网络宝箱保留未拿取物品"), ReplicatedChest->GetTreasureChestSlotAt(1)->GetItem());

	FLxInteractionFeatureSaveRecord InvalidRecord = Save->Interactions.FindChecked(LxTag_Test_InteractionSave).Features.FindChecked(WarehouseNode->NodeId);
	const FLxInteractionItemSaveRecord DuplicateSlot = InvalidRecord.ItemSlots[0];
	InvalidRecord.ItemSlots.Add(DuplicateSlot);
	ULxItemBase* OriginalWarehouseItem = Warehouse->GetWarehouseSlotAt(3)->GetItem();
	TestFalse(TEXT("拒绝重复槽位"), Warehouse->RestorePersistentData(InvalidRecord));
	TestTrue(TEXT("无效槽位不会替换原物品对象"), Warehouse->GetWarehouseSlotAt(3)->GetItem() == OriginalWarehouseItem);
	TestEqual(TEXT("无效存档不会破坏现有物品"), static_cast<int32>(Warehouse->GetWarehouseSlotAt(3)->GetItem()->ItemCount()), 11);
	InvalidRecord.ItemSlots.Pop();
	InvalidRecord.ItemSlots.Last().ItemIDTag = LxTag_Test_InteractionSave;
	TestFalse(TEXT("未知物品标识被拒绝"), Warehouse->RestorePersistentData(InvalidRecord));
	TestTrue(TEXT("未知物品不会部分清空原仓库"), Warehouse->GetWarehouseSlotAt(3)->GetItem() == OriginalWarehouseItem);
	InvalidRecord.ItemSlots.Last().ItemIDTag = Gold.ItemIDTag;
	for (const int32 InvalidCount : {-1, 256})
	{
		InvalidRecord.ItemSlots.Last().ItemCount = InvalidCount;
		TestFalse(TEXT("仓库拒绝负数或超出物品数量类型上限"), Warehouse->RestorePersistentData(InvalidRecord));
		TestTrue(TEXT("非法数量不会替换仓库物品"), Warehouse->GetWarehouseSlotAt(3)->GetItem() == OriginalWarehouseItem);
	}
	InvalidRecord.ItemSlots.Last().ItemCount = 11;
	InvalidRecord.DataVersion = 2;
	TestFalse(TEXT("未知交互数据版本不能覆盖当前仓库"), Warehouse->RestorePersistentData(InvalidRecord));
	TestTrue(TEXT("未知版本保留原物品对象"), Warehouse->GetWarehouseSlotAt(3)->GetItem() == OriginalWarehouseItem);
	FLxInteractionFeatureSaveRecord InvalidTrade = Save->Interactions.FindChecked(LxTag_Test_InteractionSave).Features.FindChecked(TradeNode->NodeId);
	InvalidTrade.ItemSlots[1].ItemCount = -1;
	TestFalse(TEXT("有限库存负数量被拒绝"), Trade->RestorePersistentData(InvalidTrade));
	InvalidTrade.ItemSlots[1].ItemCount = 256;
	TestFalse(TEXT("有限库存不能因数量截断而变成售罄"), Trade->RestorePersistentData(InvalidTrade));
	TestNull(TEXT("错误商店记录不会补充售罄商品"), Trade->GetTradeSlotAt(0)->GetItem());
	TestEqual(TEXT("错误商店记录保留其他库存"), static_cast<int32>(Trade->GetTradeSlotAt(1)->GetItem()->ItemCount()), 3);

	const auto& LoadedFeatures = Save->Interactions.FindChecked(LxTag_Test_InteractionSave).Features;
	FLxInteractionFeatureSaveRecord LegacyWarehouse = MakeLegacyInteractionRecord(LoadedFeatures.FindChecked(WarehouseNode->NodeId));
	FLxInteractionFeatureSaveRecord LegacyChest = MakeLegacyInteractionRecord(LoadedFeatures.FindChecked(ChestNode->NodeId));
	FLxInteractionFeatureSaveRecord LegacyTrade = MakeLegacyInteractionRecord(LoadedFeatures.FindChecked(TradeNode->NodeId));
	// 旧格式的售罄槽位不带物品标识，无限库存内容和价格均不作为恢复依据。
	LegacyTrade.Slots[0].Item.ItemIDTag = FGameplayTag();
	FLxItemSlotSaveRecord& LegacyUnlimited = LegacyTrade.Slots.AddDefaulted_GetRef();
	LegacyUnlimited.SlotIndex = 2;
	LegacyTrade.TradeItemValueRate = 9.0f;
	LegacyTrade.PurchaseValueRate = 8.0f;
	TestTrue(TEXT("旧仓库忽略实例词条并恢复数量"), Warehouse->RestorePersistentData(LegacyWarehouse));
	TestTrue(TEXT("旧宝箱忽略实例词条并恢复数量"), Chest->RestorePersistentData(LegacyChest));
	TestTrue(TEXT("旧商店恢复有限库存"), Trade->RestorePersistentData(LegacyTrade));
	TestEqual(TEXT("旧仓库重建默认词条"), Warehouse->GetWarehouseSlotAt(3)->GetItem()->GetItemEntryList().Num(), 1);
	TestNull(TEXT("旧档售罄状态保持"), Trade->GetTradeSlotAt(0)->GetItem());
	TestEqual(TEXT("旧档无限库存仍按预设数量创建"), static_cast<int32>(Trade->GetTradeSlotAt(2)->GetItem()->ItemCount()), 19);
	TestEqual(TEXT("旧档价格字段不覆盖当前配置"), Trade->GetTradeItemValueRate(), 1.2f);
	TestEqual(TEXT("旧档收购字段不覆盖当前配置"), Trade->GetPurchaseValueRate(), 0.8f);

	FLxMaterialInformation ReplacementItem = Gold;
	ReplacementItem.ItemIDTag = LxTag_Test_InteractionReplacementItem;
	LxItemConfig::SetMaterialItemData(ReplacementItem);
	ULxInteractableComponent* ChangedProvider = NewObject<ULxInteractableComponent>();
	ChangedProvider->FeatureConfig = Original->FeatureConfig;
	ChangedProvider->FeatureConfig.TradeContainerConfig.TradeItems[0].ItemIDTag = ReplacementItem.ItemIDTag;
	if (!TestTrue(TEXT("构建已替换商品的商人"), ChangedProvider->SetInteractionTreeAsset(Asset))) return false;
	ULxTradeContainerInteractionComponent* ChangedTrade = Cast<ULxTradeContainerInteractionComponent>(FindInteractionSaveTestFeature(ChangedProvider, TradeNode->NodeId));
	if (!TestNotNull(TEXT("更换商品后的商人"), ChangedTrade)
		|| !TestTrue(TEXT("旧库存按配置标识匹配"), ChangedTrade->RestorePersistentData(LoadedFeatures.FindChecked(TradeNode->NodeId)))) return false;
	if (!TestNotNull(TEXT("替换商品不继承旧商品售罄状态"), ChangedTrade->GetTradeSlotAt(0)->GetItem())) return false;
	TestEqual(TEXT("替换商品保持新预设标识"), ChangedTrade->GetTradeSlotAt(0)->GetItem()->ItemIDTag(), ReplacementItem.ItemIDTag);
	TestEqual(TEXT("替换商品使用新配置初始数量"), static_cast<int32>(ChangedTrade->GetTradeSlotAt(0)->GetItem()->ItemCount()), 9);
	TestEqual(TEXT("未替换的有限商品继续使用已存库存"), static_cast<int32>(ChangedTrade->GetTradeSlotAt(1)->GetItem()->ItemCount()), 3);
	Chest->GetTreasureChestSlotAt(1)->ClearItem();
	FLxInteractionFeatureSaveRecord Completed;
	TestTrue(TEXT("采集已完成宝箱"), Chest->CapturePersistentData(Completed));
	TestTrue(TEXT("完成通知已经记录"), Completed.bCompletionBroadcasted);
	TestTrue(TEXT("恢复已完成宝箱"), Chest->RestorePersistentData(Completed));
	TestEqual(TEXT("已完成宝箱仍然结束"), Chest->GetInteractionState(), ELxInteractionDataState::Finished);
	return true;
}

/** 验证四种持久交互功能在真实运行世界中自动挂载同一存档适配组件。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionSaveAutomaticAttachmentTest,
	"LxARPG.Save.InteractionAutomaticAttachment", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过真实组件注册和交互树重建检查自动接入，不依赖编辑器手动添加存档组件。 */
bool FLxInteractionSaveAutomaticAttachmentTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建独立运行世界"), World)) return false;
	ON_SCOPE_EXIT
	{
		World->EndPlay(EEndPlayReason::Quit);
		World->DestroyWorld(false);
	};
	World->InitializeActorsForPlay(FURL());
	World->SetBegunPlay(true);
	for (const ELxInteractionActionType Type : {ELxInteractionActionType::Warehouse, ELxInteractionActionType::TriggerMechanism,
		ELxInteractionActionType::TreasureChest, ELxInteractionActionType::TradeContainer})
	{
		AActor* Owner = World->SpawnActor<AActor>();
		if (!TestNotNull(TEXT("创建交互对象"), Owner)) return false;
		ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Owner);
		Provider->InteractionIDTag = LxTag_Test_InteractionSave;
		Provider->FeatureConfig.bEnableWarehouse = Type == ELxInteractionActionType::Warehouse;
		Provider->FeatureConfig.bEnableTriggerMechanism = Type == ELxInteractionActionType::TriggerMechanism;
		Provider->FeatureConfig.bEnableTreasureChest = Type == ELxInteractionActionType::TreasureChest;
		Provider->FeatureConfig.bEnableTradeContainer = Type == ELxInteractionActionType::TradeContainer;
		ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
		Asset->Features.bEnableWarehouse = Provider->FeatureConfig.bEnableWarehouse;
		Asset->Features.bEnableTriggerMechanism = Provider->FeatureConfig.bEnableTriggerMechanism;
		Asset->Features.bEnableTreasureChest = Provider->FeatureConfig.bEnableTreasureChest;
		Asset->Features.bEnableTradeContainer = Provider->FeatureConfig.bEnableTradeContainer;
		ULxInteractionTreeNodeData* Root = AddInteractionSaveTestNode(Asset, ELxInteractionActionType::Entrance);
		ULxInteractionTreeNodeData* Feature = AddInteractionSaveTestNode(Asset, Type);
		Root->Children = {Feature->NodeId};
		Asset->Roots = {Root->NodeId};
		if (!TestTrue(TEXT("配置待注册的交互树"), Provider->SetInteractionTreeAsset(Asset))) return false;
		Owner->AddInstanceComponent(Provider);
		Provider->RegisterComponentWithWorld(World);
		ULxInteractionSaveComponent* Adapter = Owner->FindComponentByClass<ULxInteractionSaveComponent>();
		if (!TestNotNull(TEXT("运行时自动挂载交互存档组件"), Adapter)) return false;
		TestEqual(TEXT("存档组件跟随交互对象标识"), Adapter->GetSaveID(), Provider->InteractionIDTag);
		TestEqual(TEXT("交互组件进入地图交互索引"), Adapter->GetSaveRecordType(), ELxSaveRecordType::Interaction);
		ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
		if (!TestTrue(TEXT("自动挂载组件可以采集对应功能"), Adapter->CaptureSaveData(Save))) return false;
		TestEqual(TEXT("仅持久化当前功能节点"), Save->Interactions.FindChecked(Provider->InteractionIDTag).Features.Num(), 1);
		TestTrue(TEXT("重建交互树成功"), Provider->LoadInteractionTreeAsset());
		TArray<ULxInteractionSaveComponent*> Adapters;
		Owner->GetComponents<ULxInteractionSaveComponent>(Adapters);
		TestEqual(TEXT("交互树重建不会重复挂载存档组件"), Adapters.Num(), 1);
		// 临时对象随独立世界统一销毁，避免缺少引擎世界上下文时单独销毁对象产生警告。
	}
	return true;
}

#endif
