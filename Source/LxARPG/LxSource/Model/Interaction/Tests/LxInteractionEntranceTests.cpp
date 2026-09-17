#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "InputCoreTypes.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxInteractionEntranceWidget.h"
#include "LxARPG/LxSource/UI/Option/LxOptionListItemWidget.h"
#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"
#include "UObject/UnrealType.h"

/** 验证入口编号、列表显示数据、滚轮及触发输入共同驱动真实交互节点。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionEntranceSelectionTest,
	"LxARPG.Interaction.EntranceSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实交互树和玩家模块验证入口控件，不替代业务激活逻辑。 */
bool FLxInteractionEntranceSelectionTest::RunTest(const FString& Parameters)
{
	UDataTable* InputTable = LoadObject<UDataTable>(nullptr,
		TEXT("/Game/项目内容/数据资产/数据表格/输入控制/角色操控输入信息表.角色操控输入信息表"));
	if (!TestNotNull(TEXT("加载实际角色输入配置"), InputTable)) return false;
	TArray<FLxInputActionInfo*> InputRows;
	InputTable->GetAllRows<FLxInputActionInfo>(TEXT("交互入口按键验证"), InputRows);
	const FLxInputActionInfo* const* InteractRow = InputRows.FindByPredicate([](const FLxInputActionInfo* Row)
	{
		return Row && Row->InputActionID == ELxInputActionID::InteractionInteract;
	});
	if (!TestNotNull(TEXT("配置中存在交互按键"), InteractRow)) return false;
	TestTrue(TEXT("交互动作默认绑定F键"), (*InteractRow)->DefaultKey == EKeys::F);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建入口测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	AActor* Npc = World->SpawnActor<AActor>();
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建NPC"), Npc) || !TestNotNull(TEXT("创建玩家"), Player)) return false;
	ULxPlayerInteractionModule* Receiver = Player->GetPlayerInteractionComponent();
	Receiver->InitializeModule(Player->GetPlayerControlComponent());
	ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Npc);
	Provider->bCreateAssetInteractionRange = false;
	Provider->RegisterComponentWithWorld(World);
	ULxInteractionEntranceWidget* Entrance = NewObject<ULxInteractionEntranceWidget>();
	Entrance->SetPlayerInteractionComponent(Receiver);
	ON_SCOPE_EXIT { Entrance->SetPlayerInteractionComponent(nullptr); };
	TestEqual(TEXT("空列表没有选中编号"), Entrance->GetCurrentEntranceOptionIndex(), INDEX_NONE);
	Entrance->ScrollEntrancePromptTexts(-1.0f);
	TestEqual(TEXT("空列表滚动保持无选项"), Entrance->GetEntranceOptionCount(), 0);

	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	for (int32 Index = 0; Index < 2; ++Index)
	{
		ULxInteractionTreeNodeData* Root = NewObject<ULxInteractionTreeNodeData>(Asset);
		Root->NodeId = FGuid::NewGuid();
		Root->Type = ELxInteractionActionType::Entrance;
		Root->PromptText = FText::FromString(Index == 0 ? TEXT("交谈") : TEXT("询问道路"));
		Asset->Nodes.Add(Root);
		Asset->Roots.Add(Root->NodeId);
	}
	if (!TestTrue(TEXT("加载两个真实入口"), Provider->SetInteractionTreeAsset(Asset))) return false;
	Provider->HandleInteractionRangeBeginOverlap(Player);
	TArray<ULxOptionViewData*> Data = Entrance->GetEntranceOptionViewData();
	if (!TestEqual(TEXT("打包两个列表项"), Data.Num(), 2)) return false;
	TestEqual(TEXT("首次显示选中第一项"), Entrance->GetCurrentEntranceOptionIndex(), 0);
	TestTrue(TEXT("第一项显示选中"), Data[0]->bSelected);
	TestFalse(TEXT("第二项显示未选中"), Data[1]->bSelected);
	TestEqual(TEXT("第二项编号与业务顺序一致"), Data[1]->OptionIndex, 1);
	TestTrue(TEXT("文本来自交互节点"), Data[1]->OptionText.EqualTo(Asset->Nodes[1]->PromptText));
	ULxOptionListItemWidget* Row = NewObject<ULxOptionListItemWidget>();
	Row->NativeOnListItemObjectSet(Data[0]);
	Entrance->ScrollEntrancePromptTexts(1.0f);
	TestEqual(TEXT("首项不能继续上滚"), Entrance->GetCurrentEntranceOptionIndex(), 0);
	Entrance->ScrollEntrancePromptTexts(-1.0f);
	TestEqual(TEXT("下滚选择第二项"), Entrance->GetCurrentEntranceOptionIndex(), 1);
	TArray<ULxOptionViewData*> ScrolledData = Entrance->GetEntranceOptionViewData();
	TestTrue(TEXT("滚动发送新的列表数据对象"), ScrolledData[0] != Data[0]);
	TestFalse(TEXT("更新后第一项取消选中"), ScrolledData[0]->bSelected);
	TestTrue(TEXT("更新后第二项显示选中"), ScrolledData[1]->bSelected);
	Row->TriggerOption();
	TestEqual(TEXT("旧条目不触发新列表中的交互"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	Entrance->ScrollEntrancePromptTexts(-1.0f);
	TestEqual(TEXT("末项不能继续下滚"), Entrance->GetCurrentEntranceOptionIndex(), 1);
	Entrance->ScrollEntrancePromptTexts(0.0f);
	TestEqual(TEXT("零滚轮值不改变选择"), Entrance->GetCurrentEntranceOptionIndex(), 1);
	Entrance->ScrollEntrancePromptTexts(1.0f);
	TestEqual(TEXT("上滚返回第一项"), Entrance->GetCurrentEntranceOptionIndex(), 0);
	Entrance->ScrollEntrancePromptTexts(-1.0f);

	FLxInputValue Input;
	Input.m_blValue = false;
	TestFalse(TEXT("松开交互键不触发"), Entrance->HandleInputEvent(ELxInputActionID::InteractionInteract, Input));
	Input.m_blValue = true;
	TestTrue(TEXT("按下交互键提交当前编号"), Entrance->HandleInputEvent(ELxInputActionID::InteractionInteract, Input));
	FObjectPropertyBase* CurrentNode = FindFProperty<FObjectPropertyBase>(Receiver->GetClass(), TEXT("CurrentInteractionNode"));
	if (!TestNotNull(TEXT("读取实际激活节点"), CurrentNode)) return false;
	TestTrue(TEXT("实际进入第二个交互节点"),
		CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == Provider->GetRootInteractionNodes()[1]);
	TestFalse(TEXT("交互期间入口不重复消费按键"), Entrance->HandleInputEvent(ELxInputActionID::InteractionInteract, Input));
	Receiver->CancelInteraction();
	TestEqual(TEXT("恢复候选列表时重新选择第一项"), Entrance->GetCurrentEntranceOptionIndex(), 0);
	Row->NativeOnListItemObjectSet(Entrance->GetEntranceOptionViewData()[1]);
	Row->TriggerOption();
	TestEqual(TEXT("条目按钮回调记录自身编号"), Entrance->GetCurrentEntranceOptionIndex(), 1);
	TestTrue(TEXT("条目按钮回调进入对应的第二个节点"),
		CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == Provider->GetRootInteractionNodes()[1]);
	Receiver->CancelInteraction();
	Row->NativeOnListItemObjectSet(Entrance->GetEntranceOptionViewData()[0]);
	Row->NativeOnEntryReleased();
	Row->TriggerOption();
	TestEqual(TEXT("释放后的条目不能触发交互"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	Provider->HandleInteractionRangeEndOverlap(Player);
	TestEqual(TEXT("离开范围清空选项"), Entrance->GetEntranceOptionCount(), 0);
	TestEqual(TEXT("离开范围清空选择编号"), Entrance->GetCurrentEntranceOptionIndex(), INDEX_NONE);
	TestFalse(TEXT("空列表不消费交互键"), Entrance->HandleInputEvent(ELxInputActionID::InteractionInteract, Input));
	return true;
}

#endif
