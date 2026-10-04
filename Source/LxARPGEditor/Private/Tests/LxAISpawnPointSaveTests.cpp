#include "LxAISpawnPointSaveTestComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "LxSaveSystemTestComponent.h"
#include "Misc/AutomationTest.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxAISpawnPointSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/World/AISpawn/LxAISpawnPointActor.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	/** 独占临时存档和世界，测试退出时仅清理本用例创建的数据。 */
	struct FLxSpawnSaveTestContext
	{
		/** 生成独立测试槽位与无需关卡资产的运行世界。 */
		FLxSpawnSaveTestContext()
			: SlotName(TEXT("LxARPG_Automation_SpawnSave_") + FGuid::NewGuid().ToString(EGuidFormats::Digits))
			, World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			World->InitializeActorsForPlay(FURL());
			World->SetBegunPlay(true);
		}

		/** 即使测试提前退出，也清理临时世界和测试存档。 */
		~FLxSpawnSaveTestContext()
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
			UGameplayStatics::DeleteGameInSlot(SlotName, 0);
		}

		/** 创建具有独立所属对象的测试存档组件。 */
		template<typename TComponent>
		TComponent* AddComponent()
		{
			AActor* Owner = World->SpawnActor<AActor>();
			if (!Owner) return nullptr;
			TComponent* Component = NewObject<TComponent>(Owner);
			Owner->AddInstanceComponent(Component);
			Component->RegisterComponentWithWorld(World);
			return Component;
		}

		/** 本用例唯一的磁盘槽名称。 */
		FString SlotName;

		/** 本用例拥有的临时世界。 */
		UWorld* World;
	};

	/** 创建包含普通角色类和首领软类路径的数量快照，不加载或创建角色实例。 */
	FLxAISpawnPointSaveRecord MakePopulationRecord(const FGameplayTag& ID)
	{
		FLxAISpawnPointSaveRecord Record;
		Record.SaveID = ID;
		FLxAISpawnPopulationEntry& Normal = Record.Population.AddDefaulted_GetRef();
		Normal.CharacterClass = ALxAICharacter::StaticClass();
		Normal.Count = 7;
		FLxAISpawnPopulationEntry& Boss = Record.Population.AddDefaulted_GetRef();
		// 此测试仅验证软类路径往返，不需要存在对应测试蓝图资产。
		Boss.CharacterClass = TSoftClassPtr<ALxAICharacter>(FSoftObjectPath(TEXT("/Game/Automation/测试首领.测试首领_C")));
		Boss.Count = 2;
		return Record;
	}
}

/** 验证三个索引空间可以使用同一标签，刷怪点只提交自己记录且类型数量可完整往返磁盘。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSpawnSaveDiskRoundTripTest,
	"LxARPG.Save.SpawnPoint.DiskRoundTripAndNamespaces",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过真实管理器写盘再重新加载，验证普通类型、首领类型、空群体与既有记录互不干扰。 */
bool FLxSpawnSaveDiskRoundTripTest::RunTest(const FString& Parameters)
{
	FLxSpawnSaveTestContext Context;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag OtherID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TStrongObjectPtr<ULxSaveManager> Writer(NewObject<ULxSaveManager>());
	Writer->Initialize(Context.SlotName);
	if (!TestTrue(TEXT("首次运行创建存档缓存"), Writer->LoadSave())) return false;
	TestEqual(TEXT("旧业务默认缓存具有空刷怪点索引"), Writer->GetSaveData()->SpawnPoints.Num(), 0);
	ULxSaveSystemTestComponent* Player = Context.AddComponent<ULxSaveSystemTestComponent>();
	ULxSaveSystemTestComponent* Interaction = Context.AddComponent<ULxSaveSystemTestComponent>();
	ULxAISpawnPointSaveTestComponent* SpawnPoint = Context.AddComponent<ULxAISpawnPointSaveTestComponent>();
	if (!Player || !Interaction || !SpawnPoint) return false;
	Player->ConfigureIdentity(ID, true);
	Player->PlayerRecord.SaveID = ID;
	Player->PlayerRecord.BackpackSlotCount = 24;
	Interaction->ConfigureIdentity(ID, false);
	Interaction->InteractionRecord.InteractionIDTag = ID;
	SpawnPoint->ConfigureIdentity(ID);
	SpawnPoint->Record = MakePopulationRecord(ID);
	SpawnPoint->UnrelatedID = OtherID;
	SpawnPoint->bWriteUnrelatedRecords = true;
	TestTrue(TEXT("注册同标签玩家"), Writer->RegisterComponent(Player));
	TestTrue(TEXT("注册同标签交互对象"), Writer->RegisterComponent(Interaction));
	TestTrue(TEXT("注册同标签刷怪点"), Writer->RegisterComponent(SpawnPoint));
	if (!TestTrue(TEXT("三个索引空间统一保存成功"), Writer->SaveAll())) return false;
	TestEqual(TEXT("刷怪点不能改写同标签玩家"), Writer->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 24);
	TestEqual(TEXT("刷怪点不能改写同标签交互记录"), Writer->GetSaveData()->Interactions.FindChecked(ID).InteractionIDTag, ID);
	TestFalse(TEXT("刷怪点不能提交其他点的记录"), Writer->GetSaveData()->SpawnPoints.Contains(OtherID));
	TStrongObjectPtr<ULxSaveManager> Reader(NewObject<ULxSaveManager>());
	Reader->Initialize(Context.SlotName);
	if (!TestTrue(TEXT("从磁盘重新加载三类记录"), Reader->LoadSave())) return false;
	TestEqual(TEXT("玩家记录仍存在"), Reader->GetSaveData()->Players.Num(), 1);
	TestEqual(TEXT("交互记录仍存在"), Reader->GetSaveData()->Interactions.Num(), 1);
	TestEqual(TEXT("刷怪点记录仍存在"), Reader->GetSaveData()->SpawnPoints.Num(), 1);
	const FLxAISpawnPointSaveRecord* SavedRecord = Reader->GetSaveData()->SpawnPoints.Find(ID);
	if (!TestNotNull(TEXT("按标签找到刷怪点"), SavedRecord)) return false;
	TestTrue(TEXT("实际角色类和各类数量完整往返"),
		FLxAISpawnPointSaveRecord::StaticStruct()->CompareScriptStruct(&SpawnPoint->Record, SavedRecord, 0));
	ULxAISpawnPointSaveTestComponent* Restored = Context.AddComponent<ULxAISpawnPointSaveTestComponent>();
	if (!Restored) return false;
	Restored->ConfigureIdentity(ID);
	TestTrue(TEXT("注册时恢复刷怪点数量记录"), Reader->RegisterComponent(Restored));
	TestEqual(TEXT("恢复普通类型和首领类型"), Restored->Record.Population.Num(), 2);
	Reader->RegisterComponent(Restored);
	TestEqual(TEXT("重复注册不会重新恢复旧数量"), Restored->RestoreCount, 1);
	Restored->Record.Population.Reset();
	if (!TestTrue(TEXT("清空怪物后可以保存有效空群体"), Reader->SaveAll())) return false;
	TStrongObjectPtr<ULxSaveManager> EmptyReader(NewObject<ULxSaveManager>());
	EmptyReader->Initialize(Context.SlotName);
	if (!TestTrue(TEXT("重新读取空群体"), EmptyReader->LoadSave())) return false;
	TestTrue(TEXT("空群体保留刷怪点身份"), EmptyReader->GetSaveData()->SpawnPoints.Contains(ID));
	TestEqual(TEXT("空群体数量为零"), EmptyReader->GetSaveData()->SpawnPoints.FindChecked(ID).Population.Num(), 0);
	return true;
}

/** 验证刷怪点重复标签、身份变化和采集失败不会覆盖既有快照或磁盘进度。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSpawnSaveAtomicCaptureTest,
	"LxARPG.Save.SpawnPoint.AtomicCaptureAndIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用失败后重试和清理前采集覆盖真实保存流程，而不是直接操作最终数据映射。 */
bool FLxSpawnSaveAtomicCaptureTest::RunTest(const FString& Parameters)
{
	FLxSpawnSaveTestContext Context;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag OtherID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	Manager->Initialize(Context.SlotName);
	if (!Manager->LoadSave()) return false;
	ULxAISpawnPointSaveTestComponent* Original = Context.AddComponent<ULxAISpawnPointSaveTestComponent>();
	ULxAISpawnPointSaveTestComponent* Duplicate = Context.AddComponent<ULxAISpawnPointSaveTestComponent>();
	if (!Original || !Duplicate) return false;
	Original->ConfigureIdentity(ID);
	Original->Record = MakePopulationRecord(ID);
	Duplicate->ConfigureIdentity(ID);
	if (!Manager->RegisterComponent(Original) || !Manager->SaveAll()) return false;
	AddExpectedError(TEXT("存档ID冲突"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("重复刷怪点标签拒绝注册"), Manager->RegisterComponent(Duplicate));
	TArray<uint8> OriginalBytes;
	if (!UGameplayStatics::LoadDataFromSlot(OriginalBytes, Context.SlotName, 0)) return false;
	Original->Record.Population[0].Count = 99;
	Original->bRejectCapture = true;
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("写入快照后返回失败会使整体保存失败"), Manager->SaveAll());
	TestEqual(TEXT("失败记录不污染内存中的旧数量"), Manager->GetSaveData()->SpawnPoints.FindChecked(ID).Population[0].Count, 7);
	AddExpectedError(TEXT("存档保存失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("采集错误禁止缓存落盘"), Manager->SaveCachedData());
	TArray<uint8> AfterFailure;
	UGameplayStatics::LoadDataFromSlot(AfterFailure, Context.SlotName, 0);
	TestTrue(TEXT("采集失败保留磁盘原始字节"), AfterFailure == OriginalBytes);
	Original->bRejectCapture = false;
	TestTrue(TEXT("重试成功允许提交最新数量"), Manager->SaveAll());
	TestEqual(TEXT("重试后内存数量更新"), Manager->GetSaveData()->SpawnPoints.FindChecked(ID).Population[0].Count, 99);
	Original->ConfigureIdentity(OtherID);
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("注册后改标签不能改写另一刷怪点"), Manager->CacheComponent(Original));
	TestFalse(TEXT("篡改标签不创建新记录"), Manager->GetSaveData()->SpawnPoints.Contains(OtherID));
	Original->ConfigureIdentity(ID, ELxSaveRecordType::Player);
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("注册后改索引类型不能写入玩家空间"), Manager->CacheComponent(Original));
	TestEqual(TEXT("玩家空间保持为空"), Manager->GetSaveData()->Players.Num(), 0);
	Original->ConfigureIdentity(ID);
	TestTrue(TEXT("恢复正确身份可以重试采集"), Manager->CacheComponent(Original));
	Manager->UnregisterComponent(Original);
	Duplicate->bRejectRestore = true;
	AddExpectedError(TEXT("存档对象恢复失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("恢复失败不能注册并覆盖数量"), Manager->RegisterComponent(Duplicate));
	Duplicate->bRejectRestore = false;
	if (!TestTrue(TEXT("修正恢复条件后可正常绑定"), Manager->RegisterComponent(Duplicate))) return false;
	Duplicate->Record.Population[0].Count = 5;
	TestTrue(TEXT("世界清理前成功缓存数量"), Manager->CacheWorldBeforeCleanup(Context.World));
	Duplicate->Record.Population.Reset();
	TestTrue(TEXT("注销后的空业务状态不覆盖快照"), Manager->SaveAll());
	TestEqual(TEXT("保存的是世界清理前数量"), Manager->GetSaveData()->SpawnPoints.FindChecked(ID).Population[0].Count, 5);
	return true;
}

/** 验证真实刷怪点存档组件始终读取所属对象标签，拒绝错误归属与不一致的记录身份。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSpawnSaveOwnerIdentityTest,
	"LxARPG.Save.SpawnPoint.OwnerIdentityAndEmptyPopulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用真实刷怪点和空群体检查存档接入，无需加载角色蓝图或导航资产。 */
bool FLxSpawnSaveOwnerIdentityTest::RunTest(const FString& Parameters)
{
	FLxSpawnSaveTestContext Context;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag OtherID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	ALxAISpawnPointActor* SpawnPoint = Context.World->SpawnActor<ALxAISpawnPointActor>();
	if (!TestNotNull(TEXT("创建真实刷怪点"), SpawnPoint)
		|| !TestNotNull(TEXT("刷怪点自动挂载存档组件"), SpawnPoint->SaveComponent.Get())) return false;
	ULxAISpawnPointSaveComponent* Component = SpawnPoint->SaveComponent;
	SpawnPoint->SpawnPointId = ID;
	FStructProperty* ExplicitID = FindFProperty<FStructProperty>(ULxSaveComponentBase::StaticClass(), TEXT("SaveID"));
	if (!TestNotNull(TEXT("查找基类显式存档标签"), ExplicitID)) return false;
	*ExplicitID->ContainerPtrToValuePtr<FGameplayTag>(Component) = OtherID;
	TestEqual(TEXT("组件忽略手动标签并跟随所属刷怪点标签"), Component->GetSaveID(), ID);
	TestEqual(TEXT("真实组件使用刷怪点索引空间"), Component->GetSaveRecordType(), ELxSaveRecordType::AISpawnPoint);
	TStrongObjectPtr<ULxGameSaveData> Snapshot(NewObject<ULxGameSaveData>());
	TestTrue(TEXT("空刷怪点可以采集有效记录"), Component->CaptureSaveData(Snapshot.Get()));
	TestFalse(TEXT("基类标签不会创建第二条记录"), Snapshot->SpawnPoints.Contains(OtherID));
	const FLxAISpawnPointSaveRecord* Record = Snapshot->SpawnPoints.Find(ID);
	if (!TestNotNull(TEXT("按所属对象标签保存空群体"), Record)) return false;
	TestEqual(TEXT("记录内部身份与索引一致"), Record->SaveID, ID);
	TestEqual(TEXT("没有怪物时保存空列表"), Record->Population.Num(), 0);
	TestTrue(TEXT("有效空群体恢复成功"), Component->RestoreSaveData(Snapshot.Get()));
	Snapshot->SpawnPoints.FindChecked(ID).SaveID = OtherID;
	TestFalse(TEXT("索引与内部身份不一致时拒绝恢复"), Component->RestoreSaveData(Snapshot.Get()));
	SpawnPoint->SpawnPointId = FGameplayTag();
	TestFalse(TEXT("所属刷怪点空标签不能回退到基类标签"), Component->GetSaveID().IsValid());
	TestFalse(TEXT("没有所属标签时拒绝采集"), Component->CaptureSaveData(Snapshot.Get()));
	ULxAISpawnPointSaveComponent* WrongOwner = Context.AddComponent<ULxAISpawnPointSaveComponent>();
	if (!WrongOwner) return false;
	TestFalse(TEXT("普通Actor上的刷怪点组件不具有有效标签"), WrongOwner->GetSaveID().IsValid());
	TestFalse(TEXT("普通Actor不能被当作刷怪点采集"), WrongOwner->CaptureSaveData(Snapshot.Get()));
	TestFalse(TEXT("普通Actor不能被当作刷怪点恢复"), WrongOwner->RestoreSaveData(Snapshot.Get()));
	return true;
}

#endif
