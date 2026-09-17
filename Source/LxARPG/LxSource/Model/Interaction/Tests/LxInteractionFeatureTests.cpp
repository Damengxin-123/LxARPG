#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxWarehouseInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxItemTransferInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxQuestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"

/** 验证交互树自动创建 UObject 功能模块，并在清空树时解除持有及绑定。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionFeatureLifecycleTest,
	"LxARPG.Interaction.FeatureLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 检查功能模块类型、初始配置、节点归属和清理行为。 */
bool FLxInteractionFeatureLifecycleTest::RunTest(const FString& Parameters)
{
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>();
	Provider->InitializeInteractionFeatures();
	TestTrue(TEXT("未配置资产时不创建交互节点"), Provider->GetRootInteractionNodes().IsEmpty());
	TestTrue(TEXT("未配置资产时不创建功能模块"), Provider->GetInteractionFeatures().IsEmpty());
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableTriggerMechanism = true;
	Provider->FeatureConfig.bEnableTriggerMechanism = true;
	Provider->FeatureConfig.TriggerMechanismConfig.InitialState = ELxMechanismState::Opened;
	FLxMechanismStatePromptText StateText;
	StateText.MechanismState = ELxMechanismState::Opened;
	StateText.PromptText = FText::FromString(TEXT("关闭机关"));
	Provider->FeatureConfig.TriggerMechanismConfig.StatePromptTexts.Add(StateText);
	ULxInteractionTreeNodeData* NodeData = NewObject<ULxInteractionTreeNodeData>(Asset);
	NodeData->NodeId = FGuid::NewGuid();
	NodeData->Type = ELxInteractionActionType::TriggerMechanism;
	NodeData->PromptText = FText::FromString(TEXT("打开机关"));
	ULxInteractionTreeNodeData* RootData = NewObject<ULxInteractionTreeNodeData>(Asset);
	RootData->NodeId = FGuid::NewGuid();
	RootData->Type = ELxInteractionActionType::Entrance;
	RootData->PromptText = FText::FromString(TEXT("交互"));
	RootData->Children.Add(NodeData->NodeId);
	Asset->Nodes = {RootData, NodeData};
	Asset->Roots = {RootData->NodeId};
	if (!TestTrue(TEXT("资产立即实例化交互树"), Provider->SetInteractionTreeAsset(Asset))) return false;
	if (!TestEqual(TEXT("资产创建唯一入口"), Provider->GetRootInteractionNodes().Num(), 1)) return false;
	ULxInteractionNode* RootNode = Provider->GetRootInteractionNodes()[0];
	if (!TestEqual(TEXT("入口创建机关子节点"), RootNode->GetChildNodes().Num(), 1)) return false;
	ULxInteractionNode* Node = RootNode->GetChildNodes()[0];
	ULxTriggerMechanismInteractionComponent* Feature = Cast<ULxTriggerMechanismInteractionComponent>(Node->GetInteractionFeature());
	if (!TestNotNull(TEXT("自动创建机关功能模块"), Feature)) return false;
	TestTrue(TEXT("资产按枚举识别功能节点"), Node->IsFunctionNode());
	TestFalse(TEXT("功能模块不再继承 ActorComponent"), Feature->IsA<UActorComponent>());
	TestTrue(TEXT("功能模块由交互提供组件持有"), Feature->GetOuter() == Provider);
	TestTrue(TEXT("节点初始配置生效"), Feature->GetMechanismState() == ELxMechanismState::Opened);
	TestTrue(TEXT("资产实例保存子节点关系"), RootNode->GetChildNodes().Contains(Node));
	TestTrue(TEXT("资产实例设置父节点关系"), Node->GetParentNode() == RootNode);
	TestEqual(TEXT("节点保留直接配置的文本"), Node->GetConfiguredPromptText().ToString(), FString(TEXT("打开机关")));
	TestEqual(TEXT("机关状态文本传递到交互节点"), Node->GetPromptText().ToString(), FString(TEXT("关闭机关")));
	Node->SetNpcDialogueText(FText::FromString(TEXT("你好，旅行者。")));
	TestEqual(TEXT("NPC发言直接保存文本"), Node->GetNpcDialogueText().ToString(), FString(TEXT("你好，旅行者。")));
	TestTrue(TEXT("功能节点通过类型校验"), Node->IsNodeValid());
	TestTrue(TEXT("运行时序号可查回原节点"), Provider->FindInteractionNodeByRuntimeIndex(Node->GetRuntimeNodeIndex()) == Node);
	TestFalse(TEXT("动态模块不使用静态网络路径"), Feature->IsNameStableForNetworking());
	Feature->SetInteractionState(ELxInteractionDataState::Interacting);
	TestFalse(TEXT("交互中的功能不会再次显示为可选入口"), Node->IsNodeInteractable(nullptr));
	TestFalse(TEXT("缺少交互发起方时不能继续处理功能请求"), Node->CanProcessActiveInteractionRequest(nullptr));
	Provider->InitializeInteractionFeatures();
	TestTrue(TEXT("重复初始化保留原功能实例"), Node->GetInteractionFeature() == Feature);
	TestTrue(TEXT("重复初始化保留运行状态"), Feature->GetInteractionState() == ELxInteractionDataState::Interacting);
	TestTrue(TEXT("再次指定同一资产成功"), Provider->SetInteractionTreeAsset(Asset));
	TestTrue(TEXT("再次指定同一资产保留原节点"), Provider->GetRootInteractionNodes()[0] == RootNode);

	// 商城即使没有商品也保留十个空槽位；商品超过十个时继续按整十个补齐。
	FLxTradeContainerInteractionConfig EmptyTradeConfig;
	const FGameplayTag DefaultGoldTag = LxTag_Item_Material_Currency_Gold;
	TestEqual(TEXT("商城配置默认使用金币标签"), EmptyTradeConfig.GoldItemIDTag, DefaultGoldTag);
	ULxInteractionTreeAsset* TradeAsset = NewObject<ULxInteractionTreeAsset>();
	TradeAsset->Features.bEnableTradeContainer = true;
	Provider->FeatureConfig.bEnableTradeContainer = true;
	Provider->FeatureConfig.TradeContainerConfig = EmptyTradeConfig;
	ULxInteractionTreeNodeData* TradeData = NewObject<ULxInteractionTreeNodeData>(TradeAsset);
	TradeData->NodeId = FGuid::NewGuid();
	TradeData->Type = ELxInteractionActionType::TradeContainer;
	TradeData->PromptText = FText::FromString(TEXT("交易"));
	TradeAsset->Nodes.Add(TradeData);
	TradeAsset->Roots.Add(TradeData->NodeId);
	if (!TestTrue(TEXT("切换为商城资产"), Provider->SetInteractionTreeAsset(TradeAsset))) return false;
	TestFalse(TEXT("资产切换后旧节点不再归组件所有"), Provider->OwnsInteractionNode(Node));
	TestNull(TEXT("资产切换后解除旧节点功能绑定"), Node->GetInteractionFeature());
	TestNull(TEXT("资产切换后解除旧功能节点绑定"), Feature->GetOwnerInteractionNode());
	ULxInteractionNode* EmptyTradeNode = Provider->GetRootInteractionNodes()[0];
	ULxTradeContainerInteractionComponent* EmptyTradeFeature = Cast<ULxTradeContainerInteractionComponent>(EmptyTradeNode->GetInteractionFeature());
	if (!TestNotNull(TEXT("商城资产创建交易模块"), EmptyTradeFeature)) return false;
	TArray<ULxItemSlotData*> EmptyTradeSlots;
	EmptyTradeFeature->GetTradeItemSlotList(EmptyTradeSlots);
	TestEqual(TEXT("空商城保留十个出售目标槽位"), EmptyTradeSlots.Num(), 10);
	TestTrue(TEXT("补齐的商城空槽位允许接收出售拖放"), EmptyTradeSlots[0]->ItemIsEnter());

	FLxTradeContainerInteractionConfig ElevenItemTradeConfig;
	ElevenItemTradeConfig.TradeItems.SetNum(11);
	ElevenItemTradeConfig.TradeItems[0].bLimitedStock = false;
	Provider->FeatureConfig.TradeContainerConfig = ElevenItemTradeConfig;
	if (!TestTrue(TEXT("显式重载资产应用修改后的配置"), Provider->LoadInteractionTreeAsset())) return false;
	ULxInteractionNode* ElevenItemTradeNode = Provider->GetRootInteractionNodes()[0];
	ULxTradeContainerInteractionComponent* ElevenItemTradeFeature = Cast<ULxTradeContainerInteractionComponent>(ElevenItemTradeNode->GetInteractionFeature());
	if (!TestNotNull(TEXT("重载后创建新的商城模块"), ElevenItemTradeFeature)) return false;
	TestNotEqual(TEXT("显式重载替换运行时节点"), ElevenItemTradeNode, EmptyTradeNode);
	TestNull(TEXT("显式重载解除原节点绑定"), EmptyTradeNode->GetInteractionFeature());
	TestNull(TEXT("显式重载解除原功能绑定"), EmptyTradeFeature->GetOwnerInteractionNode());
	TArray<ULxItemSlotData*> ElevenItemTradeSlots;
	ElevenItemTradeFeature->GetTradeItemSlotList(ElevenItemTradeSlots);
	TestEqual(TEXT("十一件商品补齐为二十个槽位"), ElevenItemTradeSlots.Num(), 20);
	TestFalse(TEXT("商品可以分别配置为无限库存"), ElevenItemTradeFeature->IsTradeSlotLimitedStock(ElevenItemTradeSlots[0]));
	TestTrue(TEXT("商品默认使用有限库存"), ElevenItemTradeFeature->IsTradeSlotLimitedStock(ElevenItemTradeSlots[1]));
	TestFalse(TEXT("补齐的空槽位不属于有限库存商品"), ElevenItemTradeFeature->IsTradeSlotLimitedStock(ElevenItemTradeSlots[19]));
	ElevenItemTradeSlots[0]->SetCanTrade(false);
	TestTrue(TEXT("购买资格不足仍允许拖动商品并在放下时校验"), ElevenItemTradeSlots[0]->ItemIsLeave());
	TestEqual(TEXT("出售堆叠物品按整组数量计算总价"),
		ULxTradeContainerInteractionComponent::CalculateStackTotalPrice(7, 3, 0.5f), 11);

	Provider->SetInteractionTreeAsset(nullptr);
	TestTrue(TEXT("卸载资产后入口列表为空"), Provider->GetRootInteractionNodes().IsEmpty());
	TestEqual(TEXT("卸载资产后模块列表为空"), Provider->GetInteractionFeatures().Num(), 0);
	TestNull(TEXT("卸载资产后解除节点的功能模块绑定"), ElevenItemTradeNode->GetInteractionFeature());
	TestNull(TEXT("卸载资产后解除模块的节点绑定"), ElevenItemTradeFeature->GetOwnerInteractionNode());
	TestNull(TEXT("卸载资产后清理运行时索引"), Provider->FindInteractionNodeByRuntimeIndex(0));
	Provider->InitializeInteractionFeatures();
	TestTrue(TEXT("卸载后初始化不会恢复旧资产"), Provider->GetRootInteractionNodes().IsEmpty());
	if (!TestTrue(TEXT("卸载后可重新配置有效资产"), Provider->SetInteractionTreeAsset(TradeAsset))) return false;
	ULxInteractionNode* BeforeInvalidNode = Provider->GetRootInteractionNodes()[0];
	ULxInteractionTreeAsset* InvalidAsset = DuplicateObject<ULxInteractionTreeAsset>(TradeAsset, GetTransientPackage());
	InvalidAsset->Features.bEnableTradeContainer = false;
	AddExpectedError(TEXT("无法加载"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("拒绝加载尚未启用其功能的资产"), Provider->SetInteractionTreeAsset(InvalidAsset));
	TestTrue(TEXT("无效资产不会残留旧树入口"), Provider->GetRootInteractionNodes().IsEmpty());
	TestTrue(TEXT("无效资产不会残留旧功能模块"), Provider->GetInteractionFeatures().IsEmpty());
	TestFalse(TEXT("加载失败后旧节点不再属于组件"), Provider->OwnsInteractionNode(BeforeInvalidNode));
	TestNull(TEXT("加载失败后旧节点解除功能绑定"), BeforeInvalidNode->GetInteractionFeature());
	Provider->SetInteractionTreeAsset(nullptr);
	return true;
}

/** 验证交互树与组件共同创建功能，并为不同NPC保存独立内容和状态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionAllFeatureAssetsTest,
	"LxARPG.Interaction.AllFeatureAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 将全部功能挂接至同一入口，覆盖统一配置入口及功能模块分发。 */
bool FLxInteractionAllFeatureAssetsTest::RunTest(const FString& Parameters)
{
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableTreasureChest = true;
	Asset->Features.bEnableWarehouse = true;
	Asset->Features.bEnableTradeContainer = true;
	Asset->Features.bEnableTriggerMechanism = true;
	Asset->Features.bEnableItemTransfer = true;
	Asset->Features.bEnableFunctionPage = true;
	Asset->Features.bEnableQuest = true;
	FLxInteractableFeatureConfig Config;
	Config.bEnableTreasureChest = true;
	Config.bEnableWarehouse = true;
	Config.bEnableTradeContainer = true;
	Config.bEnableTriggerMechanism = true;
	Config.bEnableItemTransfer = true;
	Config.bEnableFunctionPage = true;
	Config.bEnableQuest = true;
	Config.WarehouseConfig.SlotCount = 7;
	Config.TriggerMechanismConfig.InitialState = ELxMechanismState::CannotOpen;
	Config.ItemTransferConfig.Direction = ELxItemTransferDirection::RemoveFromPlayer;
	Config.ItemTransferConfig.ItemList.Add(FLxItemQuote(LxTag_Item_Material_Currency_Gold, 5));
	Config.FunctionPageConfig.FunctionPageID = ELxFunctionPageID::ItemCrafting;
	ULxInteractionTreeNodeData* Root = NewObject<ULxInteractionTreeNodeData>(Asset);
	Root->NodeId = FGuid::NewGuid();
	Root->Type = ELxInteractionActionType::Entrance;
	Root->PromptText = FText::FromString(TEXT("交互功能"));
	Asset->Nodes.Add(Root);
	Asset->Roots.Add(Root->NodeId);
	const TArray<ELxInteractionActionType> Types = {
		ELxInteractionActionType::TreasureChest, ELxInteractionActionType::Warehouse,
		ELxInteractionActionType::TradeContainer, ELxInteractionActionType::TriggerMechanism,
		ELxInteractionActionType::ItemTransfer, ELxInteractionActionType::FunctionPage,
		ELxInteractionActionType::Quest};
	for (ELxInteractionActionType Type : Types)
	{
		ULxInteractionTreeNodeData* Data = NewObject<ULxInteractionTreeNodeData>(Asset);
		Data->NodeId = FGuid::NewGuid();
		Data->Type = Type;
		Data->PromptText = ULxInteractionTreeAsset::GetTypeLabel(Type);
		Data->OptionOrder = Asset->Nodes.Num();
		if (Type == ELxInteractionActionType::Quest)
		{
			Data->QuestConfig.QuestSeriesId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务")));
			Data->QuestConfig.QuestId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务.想离开新手村")));
		}
		Root->Children.Add(Data->NodeId);
		Asset->Nodes.Add(Data);
	}
	ULxInteractableComponent* First = NewObject<ULxInteractableComponent>();
	ULxInteractableComponent* Second = NewObject<ULxInteractableComponent>();
	First->FeatureConfig = Config;
	Second->FeatureConfig = Config;
	Second->FeatureConfig.WarehouseConfig.SlotCount = 13;
	if (!TestTrue(TEXT("全部功能通过首个组件加载"), First->SetInteractionTreeAsset(Asset))
		|| !TestTrue(TEXT("同一配置通过另一个组件加载"), Second->SetInteractionTreeAsset(Asset))) return false;
	const TArray<ULxInteractionActionComponentBase*> FirstFeatures = First->GetInteractionFeatures();
	const TArray<ULxInteractionActionComponentBase*> SecondFeatures = Second->GetInteractionFeatures();
	if (!TestEqual(TEXT("全部七种功能已实例化"), FirstFeatures.Num(), Types.Num())
		|| !TestEqual(TEXT("第二个组件也创建全部功能"), SecondFeatures.Num(), Types.Num())) return false;
	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		ULxInteractionActionComponentBase* Feature = FirstFeatures[Index];
		TestEqual(TEXT("模块类型来自节点配置"), Feature->GetInteractionActionType(), Types[Index]);
		TestTrue(TEXT("功能实例由当前组件独立持有"), Feature->GetOuter() == First);
		TestNotEqual(TEXT("每种功能在不同NPC中均为独立实例"), Feature, SecondFeatures[Index]);
		TestTrue(TEXT("功能节点归属当前组件"), First->OwnsInteractionNode(Feature->GetOwnerInteractionNode()));
		TestTrue(TEXT("功能模块及节点类型匹配"), Feature->GetOwnerInteractionNode()->ValidateInteractionFeatureType());
	}
	ULxTreasureChestInteractionComponent* Chest = Cast<ULxTreasureChestInteractionComponent>(FirstFeatures[0]);
	ULxWarehouseInteractionComponent* Warehouse = Cast<ULxWarehouseInteractionComponent>(FirstFeatures[1]);
	ULxTriggerMechanismInteractionComponent* Mechanism = Cast<ULxTriggerMechanismInteractionComponent>(FirstFeatures[3]);
	ULxItemTransferInteractionComponent* Transfer = Cast<ULxItemTransferInteractionComponent>(FirstFeatures[4]);
	ULxFunctionPageInteractionComponent* Page = Cast<ULxFunctionPageInteractionComponent>(FirstFeatures[5]);
	if (!TestNotNull(TEXT("宝箱功能类型有效"), Chest)
		|| !TestNotNull(TEXT("仓库功能类型有效"), Warehouse)
		|| !TestNotNull(TEXT("机关功能类型有效"), Mechanism)
		|| !TestNotNull(TEXT("物品传递功能类型有效"), Transfer)
		|| !TestNotNull(TEXT("功能界面类型有效"), Page)
		|| !TestNotNull(TEXT("任务功能类型有效"), Cast<ULxQuestInteractionComponent>(FirstFeatures[6]))) return false;
	TArray<ULxItemSlotData*> Slots;
	Warehouse->GetWarehouseItemSlotList(Slots);
	TestEqual(TEXT("仓库使用组件指定容量"), Slots.Num(), 7);
	ULxWarehouseInteractionComponent* OtherWarehouse = Cast<ULxWarehouseInteractionComponent>(SecondFeatures[1]);
	if (!TestNotNull(TEXT("第二个NPC的仓库有效"), OtherWarehouse)) return false;
	OtherWarehouse->GetWarehouseItemSlotList(Slots);
	TestEqual(TEXT("同一棵树的不同NPC使用各自配置"), Slots.Num(), 13);
	TestEqual(TEXT("机关使用组件指定初始状态"), Mechanism->GetMechanismState(), ELxMechanismState::CannotOpen);
	TestEqual(TEXT("传递使用组件指定方向"), Transfer->GetItemTransferDirection(), ELxItemTransferDirection::RemoveFromPlayer);
	TArray<FLxItemQuote> Items;
	Transfer->GetItemTransferList(Items);
	TestEqual(TEXT("传递使用组件指定物品清单"), Items.Num(), 1);
	TestEqual(TEXT("功能界面使用组件指定页面"), Page->GetFunctionPageID(), ELxFunctionPageID::ItemCrafting);
	ULxPlayerInteractionModule* ReceiverWithoutUI = NewObject<ULxPlayerInteractionModule>();
	TestFalse(TEXT("未配置页面类时功能节点不可交互"), Page->CheckInteractionRequirement(ReceiverWithoutUI));
	Second->FeatureConfig.FunctionPageConfig.PageWidgetClass = ULxDialogueInteractionWidget::StaticClass();
	if (!TestTrue(TEXT("重载应用组件指定页面类"), Second->LoadInteractionTreeAsset())) return false;
	ULxFunctionPageInteractionComponent* ConfiguredPage = Cast<ULxFunctionPageInteractionComponent>(Second->GetInteractionFeatures()[5]);
	if (!TestNotNull(TEXT("重载后保留功能界面模块"), ConfiguredPage)) return false;
	TestTrue(TEXT("配置具体页面类后满足功能类型条件"), ConfiguredPage->CheckInteractionRequirement(ReceiverWithoutUI));
	TestFalse(TEXT("缺少本地UI上下文时打开页面失败"), ConfiguredPage->ExecuteInteraction(ReceiverWithoutUI));
	TestEqual(TEXT("页面打开失败不锁定交互状态"), ConfiguredPage->GetInteractionState(), ELxInteractionDataState::Interactable);
	ULxInteractionActionComponentBase* OtherChest = Second->GetInteractionFeatures()[0];
	const ELxInteractionDataState OtherChestState = OtherChest->GetInteractionState();
	Chest->SetTreasureChestState(ELxInteractionDataState::Occupied);
	First->InitializeInteractionFeatures();
	TestEqual(TEXT("重复初始化保留宝箱状态"), Chest->GetInteractionState(), ELxInteractionDataState::Occupied);
	TestEqual(TEXT("改变一个NPC宝箱不会影响另一个NPC"), OtherChest->GetInteractionState(), OtherChestState);
	First->SetInteractionTreeAsset(nullptr);
	for (ULxInteractionActionComponentBase* Feature : FirstFeatures)
	{
		TestNull(TEXT("卸载全部类型时均解除所属节点"), Feature->GetOwnerInteractionNode());
	}
	TestEqual(TEXT("一个NPC卸载不影响其他NPC功能"), Second->GetInteractionFeatures().Num(), Types.Num());
	return true;
}

#endif
