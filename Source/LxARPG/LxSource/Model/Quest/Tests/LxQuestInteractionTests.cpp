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
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNodeFunctionLibrary.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxQuestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxCharacterQuestModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "UObject/UnrealType.h"

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
	InteractableComponent->RegisterComponentWithWorld(World);
	FBoolProperty* EnableQuestProperty = FindFProperty<FBoolProperty>(
		InteractableComponent->GetClass(), TEXT("bEnableQuestInteraction"));
	if (!TestNotNull(TEXT("NPC可交互组件包含任务功能开关"), EnableQuestProperty)) return false;
	EnableQuestProperty->SetPropertyValue_InContainer(InteractableComponent, true);

	ULxInteractionNode* QuestNode = ULxInteractionNodeFunctionLibrary::CreateQuestInteractionNode(
		InteractableComponent, FText::FromString(TEXT("想离开新手村")), QuestSeriesId, QuestId,
		FLxInteractionRequirement(), {}, false);
	ULxInteractionNode* NextQuestNode = ULxInteractionNodeFunctionLibrary::CreateQuestInteractionNode(
		InteractableComponent, FText::FromString(TEXT("后续任务")), QuestSeriesId, NextQuestId,
		FLxInteractionRequirement(), {}, false);
	FLxInteractionRequirement CompletedRequirement;
	FLxInteractionQuestRequirement QuestRequirement;
	QuestRequirement.QuestSeriesId = QuestSeriesId;
	QuestRequirement.QuestId = QuestId;
	CompletedRequirement.RequiredQuests.Add(QuestRequirement);
	ULxInteractionNode* CompletedDialogue = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		InteractableComponent, FText::FromString(TEXT("完成后对话")), FText(),
		ELxInteractionActionType::Dialogue, CompletedRequirement, {});
	ULxInteractionNode* DialogueRootNode = ULxInteractionNodeFunctionLibrary::CreateInteractionNode(
		InteractableComponent, FText::FromString(TEXT("与村长交谈")), FText::FromString(TEXT("年轻人，有什么事？")),
		ELxInteractionActionType::Entrance, FLxInteractionRequirement(), {QuestNode, NextQuestNode, CompletedDialogue});
	InteractableComponent->BuildInteractionTree({DialogueRootNode});
	TestTrue(TEXT("任务节点被识别为功能节点"), QuestNode->IsFunctionNode());
	TestNotNull(TEXT("任务节点创建独立任务交互模块"),
		Cast<ULxQuestInteractionComponent>(QuestNode->GetInteractionFeature()));

	ULxDialogueInteractionWidget* DialogueWidget = NewObject<ULxDialogueInteractionWidget>();
	DialogueWidget->SetPlayerInteractionComponent(PlayerInteractionModule);
	InteractableComponent->HandleInteractionRangeBeginOverlap(PlayerCharacter);

	// 第一轮对话选择任务后接取，并因为没有实际目标而直接进入可提交状态。
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("未接取时显示任务选项"), DialogueWidget->GetDialogueOptionCount(), 1);
	TestEqual(TEXT("关闭任务可视化文本时显示自定义选项"),
		DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("想离开新手村")));
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestEqual(TEXT("接取任务后关闭对话窗口"), DialogueWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("首次选择后任务立即可提交"), QuestModule->GetQuestState(QuestSeriesId, QuestId),
		ELxQuestRuntimeState::ReadyToSubmit);
	TestFalse(TEXT("前置仅可提交时后续任务仍不可接取"), QuestModule->CanAcceptQuest(QuestSeriesId, NextQuestId));
	TestFalse(TEXT("可提交不满足已完成节点限制"), CompletedDialogue->IsNodeInteractable(PlayerInteractionModule));
	TestFalse(TEXT("不满足任务状态时拒绝直接执行节点"), CompletedDialogue->CanProcessActiveInteractionRequest(PlayerInteractionModule));

	// 第二轮对话再次选择同一任务选项，执行任务提交并标记完成。
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("可提交时仍显示任务选项"), DialogueWidget->GetDialogueOptionCount(), 1);
	DialogueWidget->SubmitDialogueOptionIndex(0);
	TestTrue(TEXT("第二次选择后任务已完成"), QuestModule->IsQuestCompleted(QuestSeriesId, QuestId));
	TestEqual(TEXT("提交任务后关闭对话窗口"), DialogueWidget->GetVisibility(), ESlateVisibility::Collapsed);

	// 第三轮对话隐藏已完成的前置任务，并显示同级后续任务。
	PlayerInteractionModule->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("前置完成后显示后续任务及受限对话"), DialogueWidget->GetDialogueOptionCount(), 2);
	TestEqual(TEXT("任务完成后显示受限对话"), DialogueWidget->GetDialogueOptionPromptText(1).ToString(), FString(TEXT("完成后对话")));
	TestEqual(TEXT("解锁的选项为后续任务"), DialogueWidget->GetDialogueOptionPromptText(0).ToString(), FString(TEXT("后续任务")));
	TestTrue(TEXT("前置完成后允许接取后续任务"), QuestModule->CanAcceptQuest(QuestSeriesId, NextQuestId));

	InteractableComponent->HandleInteractionRangeEndOverlap(PlayerCharacter);
	DialogueWidget->SetPlayerInteractionComponent(nullptr);
	return true;
}

#endif
