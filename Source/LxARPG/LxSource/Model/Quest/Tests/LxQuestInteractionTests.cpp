#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxQuestInteractionComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxCharacterQuestModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"

/** 验证对话型任务通过NPC交互节点完成接取、提交及完成后隐藏的基础闭环。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxQuestInteractionFlowTest,
	"LxARPG.Quest.InteractionFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 创建实际玩家与交互对象，连续执行三轮对话检查任务状态和选项可见性。 */
bool FLxQuestInteractionFlowTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建任务交互测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	World->SetGameInstance(GameInstance);
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };

	AActor* NpcActor = World->SpawnActor<AActor>();
	ALxPlayerCharacter* PlayerCharacter = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建NPC交互对象"), NpcActor)
		|| !TestNotNull(TEXT("创建玩家角色"), PlayerCharacter)) return false;

	ULxCharacterContentComponent* ContentComponent = PlayerCharacter->GetCharacterContentComponent();
	ULxCharacterDataTransferComponent* DataTransferComponent = PlayerCharacter->GetCharacterDataTransferComponent();
	ULxPlayerInteractionModule* PlayerInteractionModule = PlayerCharacter->GetPlayerInteractionComponent();
	if (!TestNotNull(TEXT("玩家持有角色内容组件"), ContentComponent)
		|| !TestNotNull(TEXT("玩家持有数据中转组件"), DataTransferComponent)
		|| !TestNotNull(TEXT("玩家持有交互模块"), PlayerInteractionModule)) return false;

	ContentComponent->BaseComponentInitialize();
	DataTransferComponent->BaseComponentInitialize();
	PlayerInteractionModule->InitializeModule(PlayerCharacter->GetPlayerControlComponent());
	ULxCharacterQuestModule* QuestModule = ContentComponent->GetQuestModule();
	if (!TestNotNull(TEXT("角色内容组件持有任务模块"), QuestModule)) return false;

	const FGameplayTag QuestSeriesId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务")));
	const FGameplayTag QuestId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务.想离开新手村")));
	const FGameplayTag NextQuestId = FGameplayTag::RequestGameplayTag(FName(TEXT("任务.新手任务.练练手")));
	ULxGameInstanceSubsystem* Subsystem = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!TestNotNull(TEXT("创建任务静态数据服务"), Subsystem)) return false;
	ULxGlobalStaticDataManager* Manager = Subsystem->GetGlobalStaticDataManager();
	if (!TestNotNull(TEXT("创建静态数据管理器"), Manager)) return false;
	ULxQuestStaticDataModule* QuestData = Manager->GetQuestStaticDataModule();
	if (!TestNotNull(TEXT("创建任务静态模块"), QuestData)) return false;
	ULxQuestSeriesAsset* Series = NewObject<ULxQuestSeriesAsset>();
#if WITH_EDITOR
	Series->SetQuestSeriesId(QuestSeriesId);
#endif
	FLxQuestNodeDefinition FirstDefinition;
	FirstDefinition.QuestId = QuestId;
	const FGameplayTag RewardItemId = LxTag_Item_Material_Currency_Gold;
	FirstDefinition.RewardItemList = {FLxItemQuote(RewardItemId, 3), FLxItemQuote(RewardItemId, 4)};
	if (!TestNotNull(TEXT("通过真实静态数据入口加载奖励物品"), LxItemConfig::GetItemData(RewardItemId))) return false;
	Series->EditQuestNodes().Add(FirstDefinition);
	FLxQuestNodeDefinition NextDefinition;
	NextDefinition.QuestId = NextQuestId;
	Series->EditQuestNodes().Add(NextDefinition);
	FLxQuestNodeLink Link;
	Link.FromQuestId = QuestId;
	Link.ToQuestId = NextQuestId;
	Series->EditQuestLinks().Add(Link);
	UDataTable* IndexTable = NewObject<UDataTable>();
	IndexTable->RowStruct = FLxQuestSeriesRegistryRow::StaticStruct();
	FLxQuestSeriesRegistryRow Row;
	Row.QuestSeriesId = QuestSeriesId;
	Row.QuestSeriesAsset = Series;
	IndexTable->AddRow(TEXT("测试任务系列"), Row);
	QuestData->Initialize(IndexTable);
	TestFalse(TEXT("前置未完成时禁止直接接取后续任务"), QuestModule->AcceptDialogueQuest(QuestSeriesId, NextQuestId));
	TestEqual(TEXT("任务初始状态为未接取"), QuestModule->GetQuestState(QuestSeriesId, QuestId),
		ELxQuestRuntimeState::NotAccepted);

	ULxInteractableComponent* InteractableComponent = NewObject<ULxInteractableComponent>(NpcActor);
	InteractableComponent->FeatureConfig.bEnableQuest = true;
	InteractableComponent->RegisterComponentWithWorld(World);
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	Asset->Features.bEnableQuest = true;
	Asset->Features.MultipleNodeTypes.Add(ELxInteractionActionType::Quest);
	/** 为同一资产添加静态节点，保持提示文本和选项排序独立。 */
	auto AddNode = [Asset](ELxInteractionActionType Type, const TCHAR* Prompt, int32 Order)
	{
		ULxInteractionTreeNodeData* Data = NewObject<ULxInteractionTreeNodeData>(Asset);
		Data->NodeId = FGuid::NewGuid();
		Data->Type = Type;
		Data->PromptText = FText::FromString(Prompt);
		Data->OptionOrder = Order;
		Asset->Nodes.Add(Data);
		return Data;
	};
	ULxInteractionTreeNodeData* QuestDataNode = AddNode(ELxInteractionActionType::Quest, TEXT("想离开新手村"), 0);
	QuestDataNode->QuestConfig.QuestSeriesId = QuestSeriesId;
	QuestDataNode->QuestConfig.QuestId = QuestId;
	QuestDataNode->QuestConfig.bUseQuestDisplayText = false;
	ULxInteractionTreeNodeData* QuestChildData = AddNode(ELxInteractionActionType::Dialogue, TEXT("任务后的对话"), 0);
	ULxInteractionTreeNodeData* CloseDialogueData = AddNode(ELxInteractionActionType::Dialogue, TEXT("结束本次对话"), 0);
	CloseDialogueData->bCloseInteractionDialogue = true;
	ULxInteractionTreeNodeData* IgnoredChildData = AddNode(ELxInteractionActionType::Dialogue, TEXT("关闭后不应显示"), 0);
	QuestDataNode->Children = {QuestChildData->NodeId};
	QuestChildData->Children = {CloseDialogueData->NodeId};
	CloseDialogueData->Children = {IgnoredChildData->NodeId};
	ULxInteractionTreeNodeData* NextQuestDataNode = AddNode(ELxInteractionActionType::Quest, TEXT("后续任务"), 1);
	NextQuestDataNode->QuestConfig.QuestSeriesId = QuestSeriesId;
	NextQuestDataNode->QuestConfig.QuestId = NextQuestId;
	NextQuestDataNode->QuestConfig.bUseQuestDisplayText = false;
	FLxInteractionRequirement CompletedRequirement;
	FLxInteractionQuestRequirement QuestRequirement;
	QuestRequirement.QuestSeriesId = QuestSeriesId;
	QuestRequirement.QuestId = QuestId;
	CompletedRequirement.RequiredQuests.Add(QuestRequirement);
	ULxInteractionTreeNodeData* CompletedDialogueData = AddNode(ELxInteractionActionType::Dialogue, TEXT("完成后对话"), 2);
	CompletedDialogueData->Requirement = CompletedRequirement;
	ULxInteractionTreeNodeData* DialogueRootData = AddNode(ELxInteractionActionType::Entrance, TEXT("与村长交谈"), 0);
	DialogueRootData->NpcDialogueText = FText::FromString(TEXT("年轻人，有什么事？"));
	DialogueRootData->Children = {QuestDataNode->NodeId, NextQuestDataNode->NodeId, CompletedDialogueData->NodeId};
	Asset->Roots.Add(DialogueRootData->NodeId);
	if (!TestTrue(TEXT("加载任务交互资产"), InteractableComponent->SetInteractionTreeAsset(Asset))) return false;
	if (!TestEqual(TEXT("任务资产创建唯一入口"), InteractableComponent->GetRootInteractionNodes().Num(), 1)) return false;
	ULxInteractionNode* DialogueRootNode = InteractableComponent->GetRootInteractionNodes()[0];
	if (!TestEqual(TEXT("入口实例保留三个任务相关选项"), DialogueRootNode->GetChildNodes().Num(), 3)) return false;
	ULxInteractionNode* QuestNode = DialogueRootNode->GetChildNodes()[0];
	ULxInteractionNode* CompletedDialogue = DialogueRootNode->GetChildNodes()[2];
	TestTrue(TEXT("任务节点被识别为功能节点"), QuestNode->IsFunctionNode());
	TestNotNull(TEXT("任务节点创建独立任务交互模块"),
		Cast<ULxQuestInteractionComponent>(QuestNode->GetInteractionFeature()));

	ULxDialogueInteractionWidget* DialogueWidget = NewObject<ULxDialogueInteractionWidget>();
	DialogueWidget->SetPlayerInteractionComponent(PlayerInteractionModule);
	ULxInteractionEntranceWidget* EntranceWidget = NewObject<ULxInteractionEntranceWidget>();
	EntranceWidget->SetPlayerInteractionComponent(PlayerInteractionModule);
	InteractableComponent->HandleInteractionRangeBeginOverlap(PlayerCharacter);
	TestEqual(TEXT("进入范围后显示入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Visible);
	FLxInputValue InteractionPressed;
	InteractionPressed.m_blValue = true;

	// 第一轮对话选择任务后接取，并因为没有实际目标而直接进入可提交状态。
	TestTrue(TEXT("交互键从入口进入对话"), EntranceWidget->HandleInputEvent(ELxInputActionID::InteractionInteract, InteractionPressed));
	TestEqual(TEXT("进入对话立即隐藏入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Collapsed);
	PlayerInteractionModule->CancelInteraction();
	TestEqual(TEXT("取消交互恢复入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Visible);
	TestTrue(TEXT("取消后交互键可以再次进入对话"), EntranceWidget->HandleInputEvent(ELxInputActionID::InteractionInteract, InteractionPressed));
	TestEqual(TEXT("未接取时显示任务选项"), DialogueWidget->GetDialogueOptionCount(), 1);
	TestEqual(TEXT("关闭任务可视化文本时显示自定义选项"),
		DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("想离开新手村")));
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestTrue(TEXT("未勾选关闭时接取任务后对话保持显示"), DialogueWidget->GetVisibility() != ESlateVisibility::Collapsed);
	TestEqual(TEXT("接取任务后继续子项导航"), PlayerInteractionModule->GetInteractionPhase(), ELxPlayerInteractionPhase::Navigation);
	TestEqual(TEXT("任务执行后显示其子项"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("任务后的对话")));
	TestEqual(TEXT("任务执行后的入口刷新不会重显提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Collapsed);
	PlayerInteractionModule->RefreshEntranceOptions();
	TestEqual(TEXT("对话期间再次刷新入口仍隐藏提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestFalse(TEXT("对话期间入口不处理交互快捷键"), EntranceWidget->HandleInputEvent(ELxInputActionID::InteractionInteract, InteractionPressed));
	EntranceWidget->SubmitCurrentEntranceOption();
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	FLxInteractionOption CachedRootOption;
	CachedRootOption.SourceInteractionComponent = InteractableComponent;
	CachedRootOption.InteractionNode = DialogueRootNode;
	TestFalse(TEXT("直接提交缓存入口也不能打断当前对话"), PlayerInteractionModule->ActivateInteractionOption(CachedRootOption));
	TestEqual(TEXT("交互键及入口调用未改变任务子项"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("任务后的对话")));
	TestEqual(TEXT("首次选择后任务立即可提交"), QuestModule->GetQuestState(QuestSeriesId, QuestId),
		ELxQuestRuntimeState::ReadyToSubmit);
	TestFalse(TEXT("接取任务不发放奖励"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 1)}));
	TestFalse(TEXT("前置仅可提交时后续任务仍不可接取"), QuestModule->CanAcceptQuest(QuestSeriesId, NextQuestId));
	TestFalse(TEXT("可提交不满足已完成节点限制"), CompletedDialogue->IsNodeInteractable(PlayerInteractionModule));
	TestFalse(TEXT("不满足任务状态时拒绝直接执行节点"), CompletedDialogue->CanProcessActiveInteractionRequest(PlayerInteractionModule));
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestEqual(TEXT("普通对话未勾选关闭时继续其子项"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("结束本次对话")));
	PlayerInteractionModule->BackToParentInteractionNode();
	TestEqual(TEXT("返回任务节点不重复提交任务"), QuestModule->GetQuestState(QuestSeriesId, QuestId), ELxQuestRuntimeState::ReadyToSubmit);
	TestEqual(TEXT("返回任务节点恢复其子项"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("任务后的对话")));
	TestEqual(TEXT("返回上级仍隐藏入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Collapsed);
	DialogueWidget->SubmitDialogueOptionIndex(0);
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestEqual(TEXT("普通对话勾选关闭时即使有子项也关闭窗口"), DialogueWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("关闭对话后不保留子项"), DialogueWidget->GetDialogueOptionCount(), 0);
	TestEqual(TEXT("关闭节点结束交互阶段"), PlayerInteractionModule->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	TestEqual(TEXT("关闭对话恢复入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Visible);

	// 第二轮对话再次选择同一任务选项，执行任务提交并标记完成。
	ULxQuestInteractionComponent* QuestFeature = Cast<ULxQuestInteractionComponent>(QuestNode->GetInteractionFeature());
	if (!TestNotNull(TEXT("取得任务奖励发放入口"), QuestFeature)) return false;
	ULxCharacterBackpackModule* Backpack = PlayerCharacter->GetCharacterBackpackComponent();
	if (!TestNotNull(TEXT("取得提交者背包"), Backpack)) return false;
	const int32 FullBackpackCount = LxItemConfig::GetItemData(RewardItemId)->ItemCountMax * Backpack->GetAllItems().Num();
	const TArray<FLxItemQuote> FullBackpackItems = {FLxItemQuote(RewardItemId, FullBackpackCount)};
	if (!TestTrue(TEXT("填满提交者背包"), DataTransferComponent->AddItemListToBackpack(FullBackpackItems))) return false;
	TestFalse(TEXT("背包满时拒绝提交"), QuestFeature->ExecuteInteraction(PlayerInteractionModule));
	TestEqual(TEXT("容量不足保留可提交状态"), QuestModule->GetQuestState(QuestSeriesId, QuestId), ELxQuestRuntimeState::ReadyToSubmit);
	if (!TestTrue(TEXT("腾出奖励空间"), DataTransferComponent->RemoveItemListFromBackpack(FullBackpackItems))) return false;
	Series->EditQuestNodes()[0].RewardItemList.Add(FLxItemQuote(RewardItemId, 0));
	TestFalse(TEXT("无效奖励数量阻止提交"), QuestFeature->ExecuteInteraction(PlayerInteractionModule));
	TestEqual(TEXT("奖励无效保留可提交状态"), QuestModule->GetQuestState(QuestSeriesId, QuestId), ELxQuestRuntimeState::ReadyToSubmit);
	TestFalse(TEXT("无效列表不会部分发奖"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 1)}));
	Series->EditQuestNodes()[0].RewardItemList.Pop();
	QuestNode->SetCloseInteractionDialogue(true);
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("可提交时仍显示任务选项"), DialogueWidget->GetDialogueOptionCount(), 1);
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestTrue(TEXT("第二次选择后任务已完成"), QuestModule->IsQuestCompleted(QuestSeriesId, QuestId));
	TestTrue(TEXT("提交者收到列表内全部奖励"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 7)}));
	TestFalse(TEXT("奖励数量准确"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 8)}));
	TestFalse(TEXT("已完成任务不能重复提交发奖"), QuestFeature->ExecuteInteraction(PlayerInteractionModule));
	TestFalse(TEXT("重复提交未增加奖励"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 8)}));
	TestEqual(TEXT("提交任务后关闭对话窗口"), DialogueWidget->GetVisibility(), ESlateVisibility::Collapsed);

	// 第三轮对话隐藏已完成的前置任务，并显示同级后续任务。
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("前置完成后显示后续任务及受限对话"), DialogueWidget->GetDialogueOptionCount(), 2);
	TestEqual(TEXT("任务完成后显示受限对话"), DialogueWidget->GetDialogueOptionPromptText(1).ToString(), FString(TEXT("完成后对话")));
	TestEqual(TEXT("解锁的选项为后续任务"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("后续任务")));
	TestTrue(TEXT("前置完成后允许接取后续任务"), QuestModule->CanAcceptQuest(QuestSeriesId, NextQuestId));
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestEqual(TEXT("无奖励任务正常接取"), QuestModule->GetQuestState(QuestSeriesId, NextQuestId), ELxQuestRuntimeState::ReadyToSubmit);
	ULxQuestInteractionComponent* NextQuestFeature = Cast<ULxQuestInteractionComponent>(DialogueRootNode->GetChildNodes()[1]->GetInteractionFeature());
	if (!TestNotNull(TEXT("取得无奖励任务交互入口"), NextQuestFeature)) return false;
	TestTrue(TEXT("无奖励任务正常提交"), NextQuestFeature->ExecuteInteraction(PlayerInteractionModule));
	TestTrue(TEXT("无奖励任务提交后完成"), QuestModule->IsQuestCompleted(QuestSeriesId, NextQuestId));
	TestFalse(TEXT("无奖励任务未增加物品"), DataTransferComponent->CheckHaveBackpackItemList({FLxItemQuote(RewardItemId, 8)}));

	InteractableComponent->HandleInteractionRangeEndOverlap(PlayerCharacter);
	TestEqual(TEXT("离开交互范围后隐藏入口提示"), EntranceWidget->GetVisibility(), ESlateVisibility::Collapsed);
	EntranceWidget->SetPlayerInteractionComponent(nullptr);
	DialogueWidget->SetPlayerInteractionComponent(nullptr);
	return true;
}

#endif
