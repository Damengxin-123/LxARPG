#if WITH_DEV_AUTOMATION_TESTS

#include "LxSaveSystemTestComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuGameMode.h"
#include "UObject/StrongObjectPtr.h"

/** 验证菜单只读状态同时阻止注册、缓存采集和磁盘提交。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMenuReadOnlyTest, "LxARPG.Menu.ReadOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxMenuReadOnlyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	AActor* Actor = World->SpawnActor<AActor>();
	ULxSaveSystemTestComponent* Component = NewObject<ULxSaveSystemTestComponent>(Actor);
	Component->ConfigureIdentity(FGameplayTag::RequestGameplayTag(TEXT("角色.默认角色")), true);
	Component->PlayerRecord.SaveID = Component->GetSaveID();
	Component->PlayerRecord.BackpackSlotCount = 7;
	TStrongObjectPtr<ULxGameSaveData> Data(NewObject<ULxGameSaveData>());
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	int32 Writes = 0;
	const auto Writer = FLxPersistSaveSession::CreateLambda([&Writes](const ULxGameSaveData*) { ++Writes; return true; });
	TestTrue(TEXT("新会话可初始化"), Manager->InitializeSession(Data.Get(), Writer));
	Manager->SetReadOnly(true);
	TestFalse(TEXT("预览不能注册存档对象"), Manager->RegisterComponent(Component));
	TestFalse(TEXT("预览不能保存"), Manager->SaveAll());
	TestEqual(TEXT("预览没有磁盘写入"), Writes, 0);
	Manager->SetReadOnly(false);
	TestTrue(TEXT("正式会话允许注册"), Manager->RegisterComponent(Component));
	TestTrue(TEXT("正式会话统一提交"), Manager->SaveAll());
	TestEqual(TEXT("提交一次完整快照"), Writes, 1);
	Manager->SetReadOnly(true);
	Component->PlayerRecord.BackpackSlotCount = 99;
	Manager->CacheComponent(Component);
	Manager->CacheWorldBeforeCleanup(World);
	TestFalse(TEXT("只读结束不落盘"), Manager->SaveCachedData());
	TestEqual(TEXT("卸载预览不污染缓存"), Manager->GetSaveData()->Players[Component->GetSaveID()].BackpackSlotCount, 7);
	TestEqual(TEXT("只读期间没有追加写入"), Writes, 1);
	TestFalse(TEXT("不能替换已经使用的会话"), Manager->InitializeSession(Data.Get(), Writer));
	return true;
}

/** 验证菜单开始不会向关卡分发正式玩法开始事件。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMenuWorldLifecycleTest, "LxARPG.Menu.WorldLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxMenuWorldLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ALxMainMenuGameMode* Mode = World->SpawnActor<ALxMainMenuGameMode>();
	if (!TestNotNull(TEXT("创建主菜单游戏模式"), Mode)) return false;
	Mode->StartPlay();
	TestFalse(TEXT("主菜单不启动关卡玩法"), World->HasBegunPlay());
	TestNull(TEXT("菜单没有默认玩家角色"), Mode->DefaultPawnClass.Get());
	return true;
}

/** 验证切换地图档会覆盖已有单位且为缺失节点恢复初始值，不重建世界或写盘。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxMenuPersistentWorldTest, "LxARPG.Menu.PersistentWorldData", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxMenuPersistentWorldTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	AActor* Actor = World->SpawnActor<AActor>();
	ULxSaveSystemTestComponent* Component = NewObject<ULxSaveSystemTestComponent>(Actor);
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.默认角色"));
	const FGuid Node = FGuid::NewGuid();
	Component->ConfigureIdentity(ID, false);
	Component->InteractionRecord.InteractionIDTag = ID;
	FLxInteractionFeatureSaveRecord& Feature = Component->InteractionRecord.Features.Add(Node);
	Feature.NodeID = Node;
	Feature.MechanismState = ELxMechanismState::Closed;
	TestTrue(TEXT("保留单位初始状态"), Component->CaptureInitialState());
	TStrongObjectPtr<ULxGameSaveData> First(NewObject<ULxGameSaveData>());
	First->Interactions.Add(ID, Component->InteractionRecord);
	First->Interactions[ID].Features[Node].MechanismState = ELxMechanismState::Opened;
	TStrongObjectPtr<ULxGameSaveData> Empty(NewObject<ULxGameSaveData>());
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	int32 Writes = 0;
	TestTrue(TEXT("安装背景地图档"), Manager->InitializeSession(First.Get(), FLxPersistSaveSession::CreateLambda(
		[&Writes](const ULxGameSaveData*) { ++Writes; return true; })));
	Manager->SetReadOnly(true);
	TestTrue(TEXT("只读背景可注册并恢复单位"), Manager->RegisterComponent(Component));
	TestTrue(TEXT("背景采用最后地图档的机关状态"), Component->InteractionRecord.Features[Node].MechanismState == ELxMechanismState::Opened);
	TestFalse(TEXT("菜单期间不采集单位"), Manager->CacheComponent(Component));
	TestTrue(TEXT("原地切换到新地图档"), Manager->ReplaceSession(Empty.Get(), true));
	TestTrue(TEXT("新地图档的缺失单位回到初始状态"), Component->InteractionRecord.Features[Node].MechanismState == ELxMechanismState::Closed);
	TestTrue(TEXT("原地切回旧地图档"), Manager->ReplaceSession(First.Get(), true));
	TestTrue(TEXT("旧地图档进度重新恢复"), Component->InteractionRecord.Features[Node].MechanismState == ELxMechanismState::Opened);
	const int32 Restores = Component->RestoreCount;
	TestTrue(TEXT("同地图切角色仅替换会话数据"), Manager->ReplaceSession(First.Get(), false));
	TestEqual(TEXT("同地图不再次恢复单位"), Component->RestoreCount, Restores);
	TestEqual(TEXT("所有菜单切换不写盘"), Writes, 0);
	Manager->SetReadOnly(false);
	TestFalse(TEXT("正式游戏不能直接覆盖会话"), Manager->ReplaceSession(Empty.Get(), true));
	TestTrue(TEXT("原单位仍参与正式会话保存"), Manager->SaveAll());
	TestEqual(TEXT("进入游戏后统一提交一次"), Writes, 1);
	return true;
}

#endif
