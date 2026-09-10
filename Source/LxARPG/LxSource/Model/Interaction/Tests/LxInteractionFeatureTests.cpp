#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNodeFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTriggerMechanismInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "UObject/UnrealType.h"

/** 验证交互树自动创建 UObject 功能模块，并在清空树时解除持有及绑定。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionFeatureLifecycleTest,
	"LxARPG.Interaction.FeatureLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 检查功能模块类型、初始配置、节点归属和清理行为。 */
bool FLxInteractionFeatureLifecycleTest::RunTest(const FString& Parameters)
{
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>();
	FBoolProperty* EnableProperty = FindFProperty<FBoolProperty>(Provider->GetClass(), TEXT("bEnableTriggerMechanism"));
	FStructProperty* ConfigProperty = FindFProperty<FStructProperty>(Provider->GetClass(), TEXT("TriggerMechanismConfig"));
	if (!TestNotNull(TEXT("细节面板包含机关启用项"), EnableProperty)
		|| !TestNotNull(TEXT("细节面板包含机关配置项"), ConfigProperty)) return false;
	EnableProperty->SetPropertyValue_InContainer(Provider, true);
	FLxTriggerMechanismInteractionConfig* Config = ConfigProperty->ContainerPtrToValuePtr<FLxTriggerMechanismInteractionConfig>(Provider);
	Config->InitialState = ELxMechanismState::Opened;
	FLxMechanismStatePromptText StateText;
	StateText.MechanismState = ELxMechanismState::Opened;
	StateText.PromptText = FText::FromString(TEXT("关闭机关"));
	Config->StatePromptTexts.Add(StateText);
	ULxInteractionNode* Node = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		Provider, FText::FromString(TEXT("打开机关")), FText(),
		ELxInteractionActionType::TriggerMechanism, FLxInteractionRequirement(), {});
	ULxInteractionNode* RootNode = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		Provider, FText::FromString(TEXT("交互")), FText(), ELxInteractionActionType::Entrance,
		FLxInteractionRequirement(), {Node});
	Provider->BuildInteractionTree({RootNode});
	ULxTriggerMechanismInteractionComponent* Feature = Cast<ULxTriggerMechanismInteractionComponent>(Node->GetInteractionFeature());
	if (!TestNotNull(TEXT("自动创建机关功能模块"), Feature)) return false;
	TestTrue(TEXT("通用创建函数按枚举识别功能节点"), Node->IsFunctionNode());
	TestFalse(TEXT("功能模块不再继承 ActorComponent"), Feature->IsA<UActorComponent>());
	TestTrue(TEXT("功能模块由交互提供组件持有"), Feature->GetOuter() == Provider);
	TestTrue(TEXT("节点初始配置生效"), Feature->GetMechanismState() == ELxMechanismState::Opened);
	TestTrue(TEXT("蓝图创建函数保存子节点关系"), RootNode->GetChildNodes().Contains(Node));
	TestTrue(TEXT("构建交互树设置父节点关系"), Node->GetParentNode() == RootNode);
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

	// 商城即使没有商品也保留十个空槽位；商品超过十个时继续按整十个补齐。
	FLxTradeContainerInteractionConfig EmptyTradeConfig;
	const FGameplayTag DefaultGoldTag = LxTag_Item_Material_Currency_Gold;
	TestEqual(TEXT("商城配置默认使用金币标签"), EmptyTradeConfig.GoldItemIDTag, DefaultGoldTag);
	ULxTradeContainerInteractionComponent* EmptyTradeFeature = NewObject<ULxTradeContainerInteractionComponent>(Provider);
	EmptyTradeFeature->ApplyConfig(EmptyTradeConfig);
	EmptyTradeFeature->InitializeInteractionFeature(Provider, nullptr, 100);
	TArray<ULxItemSlotData*> EmptyTradeSlots;
	EmptyTradeFeature->GetTradeItemSlotList(EmptyTradeSlots);
	TestEqual(TEXT("空商城保留十个出售目标槽位"), EmptyTradeSlots.Num(), 10);
	TestTrue(TEXT("补齐的商城空槽位允许接收出售拖放"), EmptyTradeSlots[0]->ItemIsEnter());

	FLxTradeContainerInteractionConfig ElevenItemTradeConfig;
	ElevenItemTradeConfig.TradeItems.SetNum(11);
	ElevenItemTradeConfig.TradeItems[0].bLimitedStock = false;
	ULxTradeContainerInteractionComponent* ElevenItemTradeFeature = NewObject<ULxTradeContainerInteractionComponent>(Provider);
	ElevenItemTradeFeature->ApplyConfig(ElevenItemTradeConfig);
	ElevenItemTradeFeature->InitializeInteractionFeature(Provider, nullptr, 101);
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

	Provider->SetRootInteractionNodes({});
	TestEqual(TEXT("清空树后模块列表为空"), Provider->GetInteractionFeatures().Num(), 0);
	TestNull(TEXT("清空树后解除节点的功能模块绑定"), Node->GetInteractionFeature());
	TestNull(TEXT("清空树后解除模块的节点绑定"), Feature->GetOwnerInteractionNode());
	return true;
}

#endif
