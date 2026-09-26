#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
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

/** 自动化测试专用标签，测试适配器只操作内存。 */
UE_DEFINE_GAMEPLAY_TAG_STATIC(LxTag_Test_InteractionSave, "测试.存档.交互对象");

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

/** 验证空槽、售罄库存、商店价格与仓库位置的完整恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionSaveContainersTest,
	"LxARPG.Save.InteractionContainers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 在独立物品配置下测试容器往返，并确保损坏存档不会破坏现有物品。 */
bool FLxInteractionSaveContainersTest::RunTest(const FString& Parameters)
{
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
	Original->FeatureConfig.TradeContainerConfig.TradeItems = {TradeItem};
	if (!TestTrue(TEXT("构建容器交互树"), Original->SetInteractionTreeAsset(Asset))) return false;
	ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(FindInteractionSaveTestFeature(Original, ChestNode->NodeId));
	ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(FindInteractionSaveTestFeature(Original, WarehouseNode->NodeId));
	ULxTradeContainerInteractionComponent* Trade = Cast<ULxTradeContainerInteractionComponent>(FindInteractionSaveTestFeature(Original, TradeNode->NodeId));
	if (!TestNotNull(TEXT("原宝箱"), Chest) || !TestNotNull(TEXT("原仓库"), Warehouse) || !TestNotNull(TEXT("原商店"), Trade)) return false;
	Chest->GetTreasureChestSlotAt(0)->ClearItem();
	Warehouse->GetWarehouseSlotAt(3)->SetItem(ULxItemBase::CreateItemObject(Warehouse, FLxItemQuote(Gold.ItemIDTag, 11)));
	Trade->GetTradeSlotAt(0)->ClearItem();
	Trade->SetTradeItemValueRate(1.75f);
	Trade->SetPurchaseValueRate(0.25f);
	ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
	if (!TestTrue(TEXT("采集全部容器"), CreateInteractionSaveTestAdapter(Original)->CaptureSaveData(Save))) return false;
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
	TestEqual(TEXT("商品倍率恢复"), Trade->GetTradeItemValueRate(), 1.75f);
	TestEqual(TEXT("收购倍率恢复"), Trade->GetPurchaseValueRate(), 0.25f);
	TestNull(TEXT("仓库空槽保留"), Warehouse->GetWarehouseSlotAt(0)->GetItem());
	if (!TestNotNull(TEXT("仓库原位置物品"), Warehouse->GetWarehouseSlotAt(3)->GetItem())) return false;
	TestEqual(TEXT("仓库数量恢复"), static_cast<int32>(Warehouse->GetWarehouseSlotAt(3)->GetItem()->ItemCount()), 11);
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
	const FLxItemSlotSaveRecord DuplicateSlot = InvalidRecord.Slots[0];
	InvalidRecord.Slots.Add(DuplicateSlot);
	TestFalse(TEXT("拒绝重复槽位"), Warehouse->RestorePersistentData(InvalidRecord));
	TestEqual(TEXT("无效存档不会破坏现有物品"), static_cast<int32>(Warehouse->GetWarehouseSlotAt(3)->GetItem()->ItemCount()), 11);
	Chest->GetTreasureChestSlotAt(1)->ClearItem();
	FLxInteractionFeatureSaveRecord Completed;
	TestTrue(TEXT("采集已完成宝箱"), Chest->CapturePersistentData(Completed));
	TestTrue(TEXT("完成通知已经记录"), Completed.bCompletionBroadcasted);
	TestTrue(TEXT("恢复已完成宝箱"), Chest->RestorePersistentData(Completed));
	TestEqual(TEXT("已完成宝箱仍然结束"), Chest->GetInteractionState(), ELxInteractionDataState::Finished);
	return true;
}

#endif
