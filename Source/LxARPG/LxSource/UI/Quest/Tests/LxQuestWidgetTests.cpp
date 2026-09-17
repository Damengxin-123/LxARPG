#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Quest/Logic/LxCharacterQuestModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/UI/Option/LxOptionViewData.h"
#include "LxARPG/LxSource/UI/Option/LxOptionListItemWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestDetailWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestSummaryWidget.h"
#include "LxARPG/LxSource/UI/Quest/LxQuestSummaryViewData.h"

/** 使用真实角色数据中转链验证任务界面的筛选、选项回调及状态刷新。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxQuestWidgetTest, "LxARPG.Quest.UI.RefreshAndSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过反射注入进度快照作为测试输入；提交和更新均经过真实任务模块。 */
bool FLxQuestWidgetTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("创建测试世界"), World)) return false;
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	World->SetGameInstance(GameInstance);
	GameInstance->Init();
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };
	ALxPlayerCharacter* Character = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("创建真实玩家角色"), Character)) return false;
	Character->GetCharacterContentComponent()->BaseComponentInitialize();
	ULxCharacterDataTransferComponent* Transfer = Character->GetCharacterDataTransferComponent();
	Transfer->BaseComponentInitialize();
	ULxCharacterQuestModule* Module = Character->GetCharacterContentComponent()->GetQuestModule();
	if (!TestNotNull(TEXT("取得任务模块"), Module)) return false;
	// 此测试的临时 GameInstance 没有引擎 WorldContext，使用真实 World 作为控件外部对象。
	ULxQuestDetailWidget* Detail = NewObject<ULxQuestDetailWidget>(World);
	ULxQuestSummaryWidget* Summary = NewObject<ULxQuestSummaryWidget>(World);
	if (!TestNotNull(TEXT("创建详细界面"), Detail) || !TestNotNull(TEXT("创建简要界面"), Summary)) return false;
	Detail->Initialize();
	Summary->Initialize();
	// 保持真实 Slate 实例存活，使可见性和 NativeConstruct 生命周期实际生效。
	const TSharedRef<SWidget> DetailSlate = Detail->TakeWidget();
	const TSharedRef<SWidget> SummarySlate = Summary->TakeWidget();
	TestEqual(TEXT("详细界面初始化后隐藏"), Detail->GetVisibility(), ESlateVisibility::Collapsed);
	ON_SCOPE_EXIT { Detail->NativeDestruct(); Summary->NativeDestruct(); };
	Detail->UpdateUIComponents(Transfer);
	Summary->UpdateUIComponents(Transfer);
	FArrayProperty* RecordProperty = FindFProperty<FArrayProperty>(Module->GetClass(), TEXT("QuestRecords"));
	FArrayProperty* OptionProperty = FindFProperty<FArrayProperty>(Detail->GetClass(), TEXT("Options"));
	FArrayProperty* DetailProperty = FindFProperty<FArrayProperty>(Detail->GetClass(), TEXT("Details"));
	FArrayProperty* SummaryProperty = FindFProperty<FArrayProperty>(Summary->GetClass(), TEXT("SummaryItems"));
	if (!TestNotNull(TEXT("进度属性"), RecordProperty) || !TestNotNull(TEXT("选项属性"), OptionProperty)
		|| !TestNotNull(TEXT("详情属性"), DetailProperty) || !TestNotNull(TEXT("简要属性"), SummaryProperty)) return false;
	auto& Records = *RecordProperty->ContainerPtrToValuePtr<TArray<FLxQuestRuntimeRecord>>(Module);
	auto& Options = *OptionProperty->ContainerPtrToValuePtr<TArray<ULxOptionViewData*>>(Detail);
	auto& Details = *DetailProperty->ContainerPtrToValuePtr<TArray<FLxQuestDetailViewData>>(Detail);
	auto& Items = *SummaryProperty->ContainerPtrToValuePtr<TArray<ULxQuestSummaryViewData*>>(Summary);
	TestTrue(TEXT("无任务时两个列表为空"), Options.IsEmpty() && Items.IsEmpty());
	const FGameplayTag SeriesId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务"));
	const FGameplayTag FirstId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务.想离开新手村"));
	const FGameplayTag SecondId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务.练练手"));
	FLxQuestRuntimeRecord First;
	First.QuestSeriesId = SeriesId;
	First.QuestId = FirstId;
	First.State = ELxQuestRuntimeState::InProgress;
	FLxQuestRuntimeRecord Second = First;
	Second.QuestId = SecondId;
	Second.State = ELxQuestRuntimeState::ReadyToSubmit;
	Records = {First, Second};
	Module->OnQuestProgressChanged.Broadcast();
	Detail->SetVisibility(ESlateVisibility::Collapsed);
	Detail->SetVisibility(ESlateVisibility::Visible);
	if (!TestEqual(TEXT("详情包含两项"), Options.Num(), 2) || !TestEqual(TEXT("简要包含两项"), Items.Num(), 2)) return false;
	TestEqual(TEXT("可提交任务优先"), Details[0].Progress.QuestId, SecondId);
	TestTrue(TEXT("打开默认选中可提交首项"), Options[0]->bSelected && !Options[1]->bSelected);
	TestTrue(TEXT("通过实际静态模块获取任务配置"), Details[0].bHasDefinition && Details[1].bHasDefinition);
	TestEqual(TEXT("选项名称来自任务可视化名称"), Options[0]->OptionText.ToString(), Details[0].Definition.DisplayName.ToString());
	TestEqual(TEXT("简要首项状态为可提交"), Items[0]->State, ELxQuestRuntimeState::ReadyToSubmit);
	ULxOptionViewData* StaleOption = Options[0];
	ULxOptionListItemWidget* Entry = NewObject<ULxOptionListItemWidget>();
	Entry->NativeOnListItemObjectSet(Options[1]);
	Entry->TriggerOption();
	TestTrue(TEXT("真实选项条目回调选择进行中任务"), Options[1]->bSelected && !Options[0]->bSelected);
	TestFalse(TEXT("重建后旧选项失效"), StaleOption->OnOptionTriggered.IsBound());
	TestTrue(TEXT("真实提交入口成功"), Transfer->SubmitQuest(SeriesId, SecondId));
	TestEqual(TEXT("提交后简要列表移除已完成任务"), Items.Num(), 1);
	TestEqual(TEXT("提交后已完成排在进行中之后"), Details[1].Progress.State, ELxQuestRuntimeState::Completed);
	TestTrue(TEXT("刷新保留按任务标识选择"), Options[0]->bSelected && Details[0].Progress.QuestId == FirstId);
	Entry->NativeOnEntryReleased();
	Detail->UpdateUIComponents(nullptr);
	Summary->UpdateUIComponents(nullptr);
	Module->OnQuestProgressChanged.Broadcast();
	TestTrue(TEXT("解除角色后旧事件不恢复列表"), Options.IsEmpty() && Items.IsEmpty());
	return true;
}

#endif
