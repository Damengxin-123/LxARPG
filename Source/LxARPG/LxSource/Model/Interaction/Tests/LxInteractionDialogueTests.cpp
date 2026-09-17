#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Interaction/LxDialogueInteractionWidget.h"
#include "LxARPG/LxSource/UI/Option/LxOptionListItemWidget.h"
#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"
#include "UObject/UnrealType.h"

/** 验证通用选项条目驱动真实对话导航，过滤返回项后仍按正确编号触发。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxInteractionDialogueOptionsTest,
	"LxARPG.Interaction.DialogueOptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实玩家模块、交互树和条目控件验证列表数据及回调生命周期。 */
bool FLxInteractionDialogueOptionsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建对话框测试世界"), World)) return false;
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
	ULxDialogueInteractionWidget* Dialog = NewObject<ULxDialogueInteractionWidget>();
	Dialog->SetPlayerInteractionComponent(Receiver);
	ON_SCOPE_EXIT { Dialog->SetPlayerInteractionComponent(nullptr); };
	TestEqual(TEXT("未交互时没有选择编号"), Dialog->GetCurrentDialogueOptionIndex(), INDEX_NONE);

	ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxInteractionTreeNodeData* Node = NewObject<ULxInteractionTreeNodeData>(Asset);
		Node->NodeId = FGuid::NewGuid();
		Node->Type = Index == 0 ? ELxInteractionActionType::Entrance : ELxInteractionActionType::Dialogue;
		Node->PromptText = FText::FromString(FString::Printf(TEXT("对话选项%d"), Index));
		Asset->Nodes.Add(Node);
	}
	Asset->Roots.Add(Asset->Nodes[0]->NodeId);
	Asset->Nodes[0]->Children = {Asset->Nodes[1]->NodeId, Asset->Nodes[2]->NodeId};
	Asset->Nodes[2]->Children.Add(Asset->Nodes[3]->NodeId);
	if (!TestTrue(TEXT("加载多级对话树"), Provider->SetInteractionTreeAsset(Asset))) return false;
	Provider->HandleInteractionRangeBeginOverlap(Player);
	Receiver->SelectEntranceOptionByIndex(0);
	TArray<ULxOptionViewData*> Data = Dialog->GetDialogueOptionViewData();
	if (!TestEqual(TEXT("对话框打包两个子选项"), Data.Num(), 2)) return false;
	TestEqual(TEXT("新列表默认选择第一项"), Dialog->GetCurrentDialogueOptionIndex(), 0);
	TestTrue(TEXT("首项显示选中"), Data[0]->bSelected);
	TestFalse(TEXT("第二项显示未选中"), Data[1]->bSelected);
	TestEqual(TEXT("第二项显示编号为1"), Data[1]->OptionIndex, 1);
	TestTrue(TEXT("条目文本来自对应节点"), Data[1]->OptionText.EqualTo(Asset->Nodes[2]->PromptText));
	ULxOptionListItemWidget* Row = NewObject<ULxOptionListItemWidget>();
	Row->NativeOnListItemObjectSet(Data[1]);
	Row->TriggerOption();
	FObjectPropertyBase* CurrentNode = FindFProperty<FObjectPropertyBase>(Receiver->GetClass(), TEXT("CurrentInteractionNode"));
	if (!TestNotNull(TEXT("读取实际导航节点"), CurrentNode)) return false;
	ULxInteractionNode* SecondNode = Provider->GetRootInteractionNodes()[0]->GetChildNodes()[1];
	TestTrue(TEXT("通用条目回调进入第二个子节点"), CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == SecondNode);
	TArray<ULxOptionViewData*> ChildData = Dialog->GetDialogueOptionViewData();
	if (!TestEqual(TEXT("子层列表过滤返回上级项"), ChildData.Num(), 1)) return false;
	TestEqual(TEXT("过滤后的唯一选项编号为0"), ChildData[0]->OptionIndex, 0);
	TestTrue(TEXT("过滤后显示真正子选项的文本"), ChildData[0]->OptionText.EqualTo(Asset->Nodes[3]->PromptText));
	Row->TriggerOption();
	TestTrue(TEXT("旧条目回调不会误选新列表"), CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == SecondNode);
	Row->NativeOnListItemObjectSet(ChildData[0]);
	Row->TriggerOption();
	TestTrue(TEXT("显示编号0触发子节点而不是返回上级"),
		CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == SecondNode->GetChildNodes()[0]);
	TestEqual(TEXT("叶节点没有可显示选项"), Dialog->GetDialogueOptionCount(), 0);
	TestEqual(TEXT("叶节点选择编号为空"), Dialog->GetCurrentDialogueOptionIndex(), INDEX_NONE);
	Receiver->CancelInteraction();
	TestTrue(TEXT("取消后清空显示数据"), Dialog->GetDialogueOptionViewData().IsEmpty());
	Row->TriggerOption();
	TestEqual(TEXT("取消后旧条目不能恢复交互"), Receiver->GetInteractionPhase(), ELxPlayerInteractionPhase::None);
	Receiver->SelectEntranceOptionByIndex(0);
	Row->NativeOnListItemObjectSet(Dialog->GetDialogueOptionViewData()[0]);
	Dialog->SetPlayerInteractionComponent(nullptr);
	Row->TriggerOption();
	TestTrue(TEXT("解除模块绑定清空条目数据"), Dialog->GetDialogueOptionViewData().IsEmpty());
	TestTrue(TEXT("解除绑定后旧条目不能导航"),
		CurrentNode->GetObjectPropertyValue_InContainer(Receiver) == Provider->GetRootInteractionNodes()[0]);
	Provider->HandleInteractionRangeEndOverlap(Player);
	return true;
}

#endif
