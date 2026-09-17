#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxQuestStaticDataModule.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/StaticDataSystem/LxGlobalStaticDataManager.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"

/** 通过真实角色状态及对话选项验证必要条件与可选条件的组合。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionRequirementModesTest,
	"LxARPG.Interaction.RequirementModes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxInteractionRequirementModesTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建限制条件测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	World->SetGameInstance(GameInstance);
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建限制条件的实际玩家"), Player)) return false;
	Player->GetCharacterContentComponent()->BaseComponentInitialize();
	Player->GetCharacterAttributeComponent()->BaseComponentInitialize();
	ULxCharacterDataTransferComponent* Transfer = Player->GetCharacterDataTransferComponent();
	Transfer->BaseComponentInitialize();
	ULxPlayerInteractionModule* Receiver = Player->GetPlayerInteractionComponent();
	Receiver->InitializeModule(Player->GetPlayerControlComponent());

	const FGameplayTag SeriesId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务"));
	const FGameplayTag FirstId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务.想离开新手村"));
	const FGameplayTag SecondId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务.练练手"));
	ULxGameInstanceSubsystem* Subsystem = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!TestNotNull(TEXT("取得静态数据服务"), Subsystem)) return false;
	ULxQuestStaticDataModule* QuestData = Subsystem->GetGlobalStaticDataManager()->GetQuestStaticDataModule();
	ULxQuestSeriesAsset* Series = NewObject<ULxQuestSeriesAsset>();
#if WITH_EDITOR
	Series->SetQuestSeriesId(SeriesId);
#endif
	FLxQuestNodeDefinition First;
	First.QuestId = FirstId;
	FLxQuestNodeDefinition Second;
	Second.QuestId = SecondId;
	Series->EditQuestNodes() = {First, Second};
	UDataTable* Index = NewObject<UDataTable>();
	Index->RowStruct = FLxQuestSeriesRegistryRow::StaticStruct();
	FLxQuestSeriesRegistryRow Registry;
	Registry.QuestSeriesId = SeriesId;
	Registry.QuestSeriesAsset = Series;
	Index->AddRow(TEXT("限制条件任务系列"), Registry);
	QuestData->Initialize(Index);

	FLxInteractionQuestRequirement FirstCondition;
	FirstCondition.QuestSeriesId = SeriesId;
	FirstCondition.QuestId = FirstId;
	FirstCondition.AllowedStates = {ELxQuestRuntimeState::ReadyToSubmit};
	FirstCondition.Mode = ELxInteractionRequirementMode::Optional;
	FLxInteractionQuestRequirement SecondCondition = FirstCondition;
	SecondCondition.QuestId = SecondId;
	FLxInteractionRequirement Requirement;
	Requirement.RequiredQuests = {FirstCondition, SecondCondition};

	AActor* Npc = World->SpawnActor<AActor>();
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Npc);
	Provider->RegisterComponentWithWorld(World);
	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	ULxInteractionTreeNodeData* Root = NewObject<ULxInteractionTreeNodeData>(Asset);
	Root->NodeId = FGuid::NewGuid();
	Root->Type = ELxInteractionActionType::Entrance;
	ULxInteractionTreeNodeData* Gate = NewObject<ULxInteractionTreeNodeData>(Asset);
	Gate->NodeId = FGuid::NewGuid();
	Gate->Type = ELxInteractionActionType::Dialogue;
	Gate->PromptText = FText::FromString(TEXT("提交任意任务"));
	Gate->Requirement = Requirement;
	Root->Children.Add(Gate->NodeId);
	Asset->Nodes = {Root, Gate};
	Asset->Roots.Add(Root->NodeId);
	if (!TestTrue(TEXT("从实际资产实例化条件节点"), Provider->SetInteractionTreeAsset(Asset))) return false;
	ULxInteractionNode* GateNode = Provider->GetRootInteractionNodes()[0]->GetChildNodes()[0];
	ULxDialogueInteractionWidget* Dialogue = NewObject<ULxDialogueInteractionWidget>();
	Dialogue->SetPlayerInteractionComponent(Receiver);
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	TestEqual(TEXT("两个任务均不可提交时隐藏对话选项"), Dialogue->GetDialogueOptionCount(), 0);
	TestFalse(TEXT("隐藏条件同时禁止直接请求"), GateNode->CanProcessActiveInteractionRequest(Receiver));
	if (!TestTrue(TEXT("接取第一个任务"), Transfer->AcceptDialogueQuest(SeriesId, FirstId))) return false;
	Receiver->RefreshCurrentInteractionOptions();
	TestEqual(TEXT("仅第一个任务可提交时显示对话"), Dialogue->GetDialogueOptionCount(), 1);
	TestTrue(TEXT("可见条件同时允许直接请求"), GateNode->CanProcessActiveInteractionRequest(Receiver));
	if (!TestTrue(TEXT("接取第二个独立任务"), Transfer->AcceptDialogueQuest(SeriesId, SecondId))) return false;
	Receiver->RefreshCurrentInteractionOptions();
	TestEqual(TEXT("两个任务都可提交时仍显示对话"), Dialogue->GetDialogueOptionCount(), 1);
	if (!TestTrue(TEXT("完成第一个任务"), Transfer->SubmitQuest(SeriesId, FirstId))) return false;
	Receiver->RefreshCurrentInteractionOptions();
	TestEqual(TEXT("第一个已完成、第二个可提交时仍显示对话"), Dialogue->GetDialogueOptionCount(), 1);
	if (!TestTrue(TEXT("完成第二个任务"), Transfer->SubmitQuest(SeriesId, SecondId))) return false;
	Receiver->RefreshCurrentInteractionOptions();
	TestEqual(TEXT("两个任务都完成后隐藏对话"), Dialogue->GetDialogueOptionCount(), 0);
	TestFalse(TEXT("两个任务都完成后禁止直接请求"), GateNode->CanProcessActiveInteractionRequest(Receiver));

	ULxInteractionNode* Probe = NewObject<ULxInteractionNode>();
	/** 使用同一正式通用判断入口检查不同配置，不替换角色数据查询。 */
	const auto Check = [Probe, Receiver](const FLxInteractionRequirement& Conditions)
	{
		Probe->InitializeInteractionNode(FText(), ELxInteractionActionType::Dialogue, {}, Conditions, FText());
		return Probe->IsNodeInteractable(Receiver);
	};
	TestTrue(TEXT("空条件不限制节点"), Check(FLxInteractionRequirement()));
	const FGameplayTag Gold = LxTag_Item_Material_Currency_Gold;
	if (!TestTrue(TEXT("通过实际背包添加五个金币"), Transfer->AddItemListToBackpack({FLxItemQuote(Gold, 5)}))) return false;
	FLxInteractionItemRequirement Item;
	Item.ItemIDTag = Gold;
	Item.ItemCount = 3;
	FLxInteractionRequirement Mixed;
	Mixed.ItemRequirements = {Item};
	TestTrue(TEXT("仅必要条件且满足时允许节点"), Check(Mixed));
	Mixed.ItemRequirements.Add(Item);
	TestFalse(TEXT("同类必要物品数量保持旧版累加语义"), Check(Mixed));
	Mixed.ItemRequirements.Pop();
	Mixed.RequiredQuests = Requirement.RequiredQuests;
	TestFalse(TEXT("必要项满足但全部可选项失败时拒绝"), Check(Mixed));
	Mixed.RequiredQuests[1].AllowedStates = {ELxQuestRuntimeState::Completed};
	TestTrue(TEXT("必要物品与匹配的可选任务组合生效"), Check(Mixed));
	Mixed.ItemRequirements[0].ItemCount = 6;
	TestFalse(TEXT("可选任务匹配不能绕过必要物品"), Check(Mixed));
	Mixed.ItemRequirements[0].ItemCount = 3;
	Mixed.ItemRequirements[0].Mode = ELxInteractionRequirementMode::Optional;
	Mixed.RequiredQuests = Requirement.RequiredQuests;
	TestTrue(TEXT("不同类别可选条件属于同一个或集合"), Check(Mixed));
	Mixed.RequiredQuests[0].QuestId = SeriesId;
	Mixed.RequiredQuests[0].AllowedStates = {ELxQuestRuntimeState::NotAccepted};
	TestTrue(TEXT("无效可选任务不会阻止其他可选项满足"), Check(Mixed));
	Mixed.ItemRequirements.Reset();
	TestFalse(TEXT("无效任务不会被默认未接取状态误判匹配"), Check(Mixed));
	Mixed.ItemRequirements = {Item};
	Mixed.ItemRequirements[0].Mode = ELxInteractionRequirementMode::Optional;
	Mixed.RequiredQuests.Reset();
	FLxInteractionAttributeRequirement Attribute;
	Attribute.Mode = ELxInteractionRequirementMode::Optional;
	Mixed.RequiredAttributes.Add(Attribute);
	FLxInteractionStateRequirement State;
	State.Mode = ELxInteractionRequirementMode::Optional;
	Mixed.StateRequirements.Add(State);
	TestTrue(TEXT("不匹配的可选属性和状态不阻止匹配物品"), Check(Mixed));
	Mixed.RequiredAttributes[0].Mode = ELxInteractionRequirementMode::Required;
	TestFalse(TEXT("必要属性不满足时拒绝"), Check(Mixed));
	Mixed.RequiredAttributes.Reset();
	Mixed.StateRequirements[0].Mode = ELxInteractionRequirementMode::Required;
	TestFalse(TEXT("必要状态不满足时拒绝"), Check(Mixed));
	Mixed.StateRequirements.Reset();
	Mixed.ItemRequirements[0].Mode = static_cast<ELxInteractionRequirementMode>(255);
	TestFalse(TEXT("未知生效方式不会放行节点"), Check(Mixed));

	float Strength = 0.0f;
	if (!TestTrue(TEXT("读取真实角色力量属性"), Transfer->QueryCharacterAttributeValue(LxTag_Attribute_Ability_Strength, Strength))) return false;
	Transfer->ClearCharacterStateTagsByCategory(LxTag_CharacterState_Movement);
	if (!TestTrue(TEXT("通过正式状态入口添加静止状态"), Transfer->AddCharacterStateTag(
		LxTag_CharacterState_Movement, LxTag_CharacterState_Movement_Idle))) return false;
	Mixed = FLxInteractionRequirement();
	Attribute.AttributeIDTag = LxTag_Attribute_Ability_Strength;
	Attribute.MinValue = Strength;
	Attribute.Mode = ELxInteractionRequirementMode::Required;
	State.StateTag = LxTag_CharacterState_Movement;
	Mixed.RequiredAttributes = {Attribute};
	Mixed.StateRequirements = {State};
	TestTrue(TEXT("必要属性与可选状态满足，状态保持父标签匹配"), Check(Mixed));
	Mixed.StateRequirements[0].StateTag = LxTag_CharacterState_Movement_Running;
	TestFalse(TEXT("属性必要项满足仍需要可选状态匹配"), Check(Mixed));
	Mixed.RequiredAttributes[0].Mode = ELxInteractionRequirementMode::Optional;
	TestTrue(TEXT("匹配的可选属性可以独立满足可选集合"), Check(Mixed));
	Mixed.RequiredAttributes[0].MinValue = Strength + 1.0f;
	TestFalse(TEXT("全部可选属性与状态都不匹配时拒绝"), Check(Mixed));
	Mixed.StateRequirements[0].StateTag = LxTag_CharacterState_Movement_Idle;
	TestTrue(TEXT("匹配的可选状态可以独立满足可选集合"), Check(Mixed));

	Provider->HandleInteractionRangeEndOverlap(Player);
	Dialogue->SetPlayerInteractionComponent(nullptr);
	return true;
}

/** 验证旧字段的标签序列化加载、必要模式默认值和新格式二次保存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionRequirementMigrationTest,
	"LxARPG.Interaction.RequirementMigration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxInteractionRequirementMigrationTest::RunTest(const FString& Parameters)
{
	UScriptStruct* Type = FLxInteractionRequirement::StaticStruct();
	FArrayProperty* OldItems = FindFProperty<FArrayProperty>(Type, TEXT("RequiredItems"));
	FStructProperty* OldStates = FindFProperty<FStructProperty>(Type, TEXT("RequiredStateTags"));
	if (!TestNotNull(TEXT("保留旧物品字段的序列化名称"), OldItems)
		|| !TestNotNull(TEXT("保留旧状态字段的序列化名称"), OldStates)) return false;
	FLxInteractionRequirement Source;
	const FGameplayTag Tag = LxTag_Item_Material_Currency_Gold;
	*OldItems->ContainerPtrToValuePtr<TArray<FLxItemQuote>>(&Source) = {FLxItemQuote(Tag, 2), FLxItemQuote(Tag, 3)};
	OldStates->ContainerPtrToValuePtr<FGameplayTagContainer>(&Source)->AddTag(Tag);
	TArray<uint8> Bytes;
	{
		FMemoryWriter Memory(Bytes, true);
		FObjectAndNameAsStringProxyArchive Writer(Memory, false);
		// 明确写出已弃用的旧字段，复现旧资产中的字段名和字段类型。
		Writer.SetPortFlags(PPF_UseDeprecatedProperties);
		Type->SerializeItem(Writer, &Source, nullptr);
	}
	FLxInteractionRequirement Loaded;
	{
		FMemoryReader Memory(Bytes, true);
		FObjectAndNameAsStringProxyArchive Reader(Memory, true);
		Type->SerializeItem(Reader, &Loaded, nullptr);
		TestFalse(TEXT("旧结构标签流成功加载"), Reader.IsError());
	}
	if (!TestEqual(TEXT("迁移完整物品列表"), Loaded.ItemRequirements.Num(), 2)
		|| !TestEqual(TEXT("迁移状态标签列表"), Loaded.StateRequirements.Num(), 1)) return false;
	TestEqual(TEXT("迁移保持物品数量"), Loaded.ItemRequirements[1].ItemCount, 3);
	TestEqual(TEXT("迁移保持物品标签"), Loaded.ItemRequirements[0].ItemIDTag, Tag);
	TestEqual(TEXT("旧物品默认为必要"), Loaded.ItemRequirements[0].Mode, ELxInteractionRequirementMode::Required);
	TestEqual(TEXT("旧状态默认为必要"), Loaded.StateRequirements[0].Mode, ELxInteractionRequirementMode::Required);
	TestTrue(TEXT("旧物品字段退出运行时数据"), OldItems->ContainerPtrToValuePtr<TArray<FLxItemQuote>>(&Loaded)->IsEmpty());
	TestTrue(TEXT("旧状态字段退出运行时数据"), OldStates->ContainerPtrToValuePtr<FGameplayTagContainer>(&Loaded)->IsEmpty());
	Loaded.ItemRequirements[0].Mode = ELxInteractionRequirementMode::Optional;
	Bytes.Reset();
	{
		FMemoryWriter Memory(Bytes, true);
		FObjectAndNameAsStringProxyArchive Writer(Memory, false);
		Type->SerializeItem(Writer, &Loaded, nullptr);
	}
	FLxInteractionRequirement Reloaded;
	{
		FMemoryReader Memory(Bytes, true);
		FObjectAndNameAsStringProxyArchive Reader(Memory, true);
		Type->SerializeItem(Reader, &Reloaded, nullptr);
		TestFalse(TEXT("新结构标签流成功加载"), Reader.IsError());
	}
	if (!TestEqual(TEXT("再次保存加载不重复迁移"), Reloaded.ItemRequirements.Num(), 2)) return false;
	TestEqual(TEXT("保存保留可选方式"), Reloaded.ItemRequirements[0].Mode, ELxInteractionRequirementMode::Optional);
	return true;
}

#endif
