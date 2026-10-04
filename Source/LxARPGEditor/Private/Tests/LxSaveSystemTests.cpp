#include "LxSaveSystemTestComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	/** 每个用例独占临时槽位，退出时只删除自己生成的测试存档。 */
	struct FLxSaveTestSlot
	{
		/** 用随机标识隔离用户存档及并行编辑器实例。 */
		FLxSaveTestSlot()
			: Name(TEXT("LxARPG_Automation_Save_") + FGuid::NewGuid().ToString(EGuidFormats::Digits))
		{
		}

		/** 即使断言提前返回也清理本用例的临时文件。 */
		~FLxSaveTestSlot()
		{
			UGameplayStatics::DeleteGameInSlot(Name, 0);
		}

		/** 此用例唯一的磁盘槽名称。 */
		FString Name;
	};

	/** 为存档组件提供独立的真实对象生命周期，无需加载关卡资产。 */
	struct FLxSaveTestWorld
	{
		/** 创建允许自动分发对象开始与结束事件的临时游戏世界。 */
		FLxSaveTestWorld()
			: World(UWorld::CreateWorld(EWorldType::Game, false))
		{
			World->InitializeActorsForPlay(FURL());
			World->SetBegunPlay(true);
		}

		/** 销毁当前用例创建的全部对象。 */
		~FLxSaveTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
		}

		/** 生成具有独立所属对象的测试组件。 */
		ULxSaveSystemTestComponent* AddComponent(const FGameplayTag& ID, bool bPlayer = true)
		{
			AActor* Owner = World->SpawnActor<AActor>();
			if (!Owner) return nullptr;
			ULxSaveSystemTestComponent* Component = NewObject<ULxSaveSystemTestComponent>(Owner);
			Component->ConfigureIdentity(ID, bPlayer);
			Owner->AddInstanceComponent(Component);
			Component->RegisterComponentWithWorld(World);
			return Component;
		}

		/** 此用例拥有的临时世界。 */
		UWorld* World;
	};

	/** 生成具有非默认嵌套属性的玩家快照，验证槽位、词条、任务与职业均可往返。 */
	FLxCharacterSaveRecord MakePlayerRecord(const FGameplayTag& ID)
	{
		FLxCharacterSaveRecord Record;
		Record.SaveID = ID;
		Record.BackpackSlotCount = 24;
		FLxItemSlotSaveRecord& ItemSlot = Record.BackpackSlots.AddDefaulted_GetRef();
		ItemSlot.SlotIndex = 3;
		ItemSlot.SlotTag = FGameplayTag::RequestGameplayTag(TEXT("物品.材料.货币.金币"));
		ItemSlot.Item.ItemIDTag = ItemSlot.SlotTag;
		ItemSlot.Item.ItemCount = 37;
		FLxItemEntryConfig& Entry = ItemSlot.Item.Entries.AddDefaulted_GetRef();
		Entry.EntryQuote.EntryID = FGameplayTag::RequestGameplayTag(TEXT("属性.战斗.攻击力"));
		Entry.EntryQuote.EntryProportion = 1.75f;
		Entry.EntryQuote.EntryCD = 8.5f;
		Record.EquipmentSlots.Add(ItemSlot);
		Record.EquipmentSlots[0].SlotIndex = 0;
		Record.EquipmentSlots[0].Item.ItemCount = 1;
		Record.BackpackSlots.AddDefaulted_GetRef().SlotIndex = 4;
		FLxQuestRuntimeRecord& Quest = Record.QuestRecords.AddDefaulted_GetRef();
		Quest.QuestSeriesId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务"));
		Quest.QuestId = FGameplayTag::RequestGameplayTag(TEXT("任务.新手任务.想离开新手村"));
		Quest.State = ELxQuestRuntimeState::Completed;
		FLxProfessionSaveRecord& Profession = Record.Professions.AddDefaulted_GetRef();
		Profession.ProfessionIDTag = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.战士.见习战士"));
		Profession.Level = 7;
		Profession.Experience = 123.25f;
		Profession.bCanUpgrade = false;
		return Record;
	}

	/** 生成四类交互节点的持久属性，覆盖GUID映射及容器空槽位。 */
	FLxInteractionSaveRecord MakeInteractionRecord(const FGameplayTag& ID)
	{
		FLxInteractionSaveRecord Record;
		Record.InteractionIDTag = ID;
		const ELxInteractionActionType Types[] = {
			ELxInteractionActionType::TriggerMechanism, ELxInteractionActionType::TreasureChest,
			ELxInteractionActionType::TradeContainer, ELxInteractionActionType::Warehouse};
		for (const ELxInteractionActionType Type : Types)
		{
			FLxInteractionFeatureSaveRecord Feature;
			Feature.NodeID = FGuid::NewGuid();
			Feature.InteractionType = Type;
			Feature.InteractionState = ELxInteractionDataState::Finished;
			Feature.MechanismState = ELxMechanismState::Opened;
			Feature.bCompletionBroadcasted = true;
			Feature.TradeItemValueRate = 1.25f;
			Feature.PurchaseValueRate = .75f;
			Feature.Slots = MakePlayerRecord(ID).BackpackSlots;
			Record.Features.Add(Feature.NodeID, Feature);
		}
		return Record;
	}
}

/** 验证全部纯属性经管理器写盘、重新加载及组件恢复后仍然完整。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveDiskRoundTripTest,
	"LxARPG.Save.DiskRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 检查同名的玩家与交互对象在不同索引空间中不会互相覆盖。 */
bool FLxSaveDiskRoundTripTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestWorld TestWorld;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	TStrongObjectPtr<ULxSaveManager> Writer(NewObject<ULxSaveManager>());
	Writer->Initialize(Slot.Name);
	if (!TestTrue(TEXT("首次运行创建空存档缓存"), Writer->LoadSave())) return false;
	TestTrue(TEXT("空缓存保存成功"), Writer->SaveCachedData());
	TestFalse(TEXT("空缓存不创建磁盘存档"), UGameplayStatics::DoesSaveGameExist(Slot.Name, 0));
	ULxSaveSystemTestComponent* Player = TestWorld.AddComponent(ID);
	ULxSaveSystemTestComponent* Interaction = TestWorld.AddComponent(ID, false);
	if (!TestNotNull(TEXT("创建玩家适配组件"), Player)
		|| !TestNotNull(TEXT("创建交互适配组件"), Interaction)) return false;
	Player->PlayerRecord = MakePlayerRecord(ID);
	Interaction->InteractionRecord = MakeInteractionRecord(ID);
	TestTrue(TEXT("注册玩家"), Writer->RegisterComponent(Player));
	TestTrue(TEXT("同标签交互对象使用独立索引"), Writer->RegisterComponent(Interaction));
	if (!TestTrue(TEXT("统一采集并写盘"), Writer->SaveAll())) return false;
	TStrongObjectPtr<ULxSaveManager> Reader(NewObject<ULxSaveManager>());
	Reader->Initialize(Slot.Name);
	if (!TestTrue(TEXT("重新创建的管理器从磁盘读入记录"), Reader->LoadSave())) return false;
	TestEqual(TEXT("玩家索引只有一条记录"), Reader->GetSaveData()->Players.Num(), 1);
	TestEqual(TEXT("交互索引只有一条记录"), Reader->GetSaveData()->Interactions.Num(), 1);
	const FLxCharacterSaveRecord* SavedPlayer = Reader->GetSaveData()->Players.Find(ID);
	const FLxInteractionSaveRecord* SavedInteraction = Reader->GetSaveData()->Interactions.Find(ID);
	if (!TestNotNull(TEXT("通过精确标签找到玩家"), SavedPlayer)
		|| !TestNotNull(TEXT("通过精确标签找到交互对象"), SavedInteraction)) return false;
	TestTrue(TEXT("背包、装备、词条、任务与职业属性往返保持一致"),
		FLxCharacterSaveRecord::StaticStruct()->CompareScriptStruct(&Player->PlayerRecord, SavedPlayer, 0));
	TestTrue(TEXT("机关、宝箱、商店与仓库节点属性往返保持一致"),
		FLxInteractionSaveRecord::StaticStruct()->CompareScriptStruct(&Interaction->InteractionRecord, SavedInteraction, 0));
	ULxSaveSystemTestComponent* RestoredPlayer = TestWorld.AddComponent(ID);
	ULxSaveSystemTestComponent* RestoredInteraction = TestWorld.AddComponent(ID, false);
	if (!TestNotNull(TEXT("创建待恢复玩家"), RestoredPlayer)
		|| !TestNotNull(TEXT("创建待恢复交互对象"), RestoredInteraction)) return false;
	TestTrue(TEXT("注册时恢复玩家状态"), Reader->RegisterComponent(RestoredPlayer));
	TestTrue(TEXT("注册时恢复交互状态"), Reader->RegisterComponent(RestoredInteraction));
	if (TestEqual(TEXT("恢复一条任务记录"), RestoredPlayer->PlayerRecord.QuestRecords.Num(), 1))
	{
		TestEqual(TEXT("恢复已完成任务而不是重新领取任务"), RestoredPlayer->PlayerRecord.QuestRecords[0].State,
			ELxQuestRuntimeState::Completed);
	}
	TestEqual(TEXT("四类交互节点全部恢复"), RestoredInteraction->InteractionRecord.Features.Num(), 4);
	TestEqual(TEXT("首次注册只恢复一次"), RestoredPlayer->RestoreCount, 1);
	Reader->RegisterComponent(RestoredPlayer);
	TestEqual(TEXT("重复注册不再次覆盖运行时状态"), RestoredPlayer->RestoreCount, 1);
	return true;
}

/** 验证重复ID、运行中ID变更和恢复失败都不会污染已有记录。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveIdentityProtectionTest,
	"LxARPG.Save.IdentityProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用不同所属对象模拟关卡中的误配置，而不直接改管理器注册表。 */
bool FLxSaveIdentityProtectionTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestWorld TestWorld;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag OtherID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	Manager->Initialize(Slot.Name);
	if (!TestTrue(TEXT("管理器初始化成功"), Manager->LoadSave())) return false;
	ULxSaveSystemTestComponent* Original = TestWorld.AddComponent(ID);
	ULxSaveSystemTestComponent* Duplicate = TestWorld.AddComponent(ID);
	if (!Original || !Duplicate) return false;
	Original->PlayerRecord = MakePlayerRecord(ID);
	Duplicate->PlayerRecord.BackpackSlotCount = 999;
	TestTrue(TEXT("首次注册成功"), Manager->RegisterComponent(Original));
	AddExpectedError(TEXT("存档ID冲突"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("同类对象重复ID必须被拒绝"), Manager->RegisterComponent(Duplicate));
	Manager->CacheComponent(Original);
	Manager->CacheComponent(Duplicate);
	TestEqual(TEXT("被拒绝对象没有参与采集"), Duplicate->CaptureCount, 0);
	TestEqual(TEXT("重复对象不能覆盖原记录"), Manager->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 24);
	Original->ConfigureIdentity(OtherID, true);
	Original->PlayerRecord.BackpackSlotCount = 999;
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("注册后身份变化明确回报采集失败"), Manager->CacheComponent(Original));
	TestFalse(TEXT("注册后篡改ID不能写入新记录"), Manager->GetSaveData()->Players.Contains(OtherID));
	TestEqual(TEXT("原ID的旧记录仍完整"), Manager->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 24);
	Manager->UnregisterComponent(Original);
	Duplicate->bRejectRestore = true;
	AddExpectedError(TEXT("存档对象恢复失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("已有记录恢复失败不能继续注册"), Manager->RegisterComponent(Duplicate));
	Manager->CacheComponent(Duplicate);
	TestEqual(TEXT("恢复失败的对象仍不参与采集"), Duplicate->CaptureCount, 0);
	TestEqual(TEXT("恢复失败保留旧记录"), Manager->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 24);
	return true;
}

/** 验证采集错误向保存调用者传播，并阻止部分采集结果覆盖磁盘上的完整存档。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveCaptureFailureTest,
	"LxARPG.Save.CaptureFailureProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 模拟写入后失败、重试成功和世界结束前失败，检查返回值、内存快照与磁盘字节。 */
bool FLxSaveCaptureFailureTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestWorld TestWorld;
	const FGameplayTag FirstID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag SecondID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	Manager->Initialize(Slot.Name);
	if (!TestTrue(TEXT("创建采集失败测试缓存"), Manager->LoadSave())) return false;
	ULxSaveSystemTestComponent* First = TestWorld.AddComponent(FirstID);
	ULxSaveSystemTestComponent* Second = TestWorld.AddComponent(SecondID);
	if (!TestNotNull(TEXT("创建待失败组件"), First) || !TestNotNull(TEXT("创建正常组件"), Second)) return false;
	First->PlayerRecord = MakePlayerRecord(FirstID);
	Second->PlayerRecord = MakePlayerRecord(SecondID);
	Second->PlayerRecord.BackpackSlotCount = 48;
	if (!Manager->RegisterComponent(First) || !Manager->RegisterComponent(Second)
		|| !TestTrue(TEXT("先保存一份完整进度"), Manager->SaveAll())) return false;
	TArray<uint8> OriginalFile;
	if (!TestTrue(TEXT("读取原始存档字节"), UGameplayStatics::LoadDataFromSlot(OriginalFile, Slot.Name, 0))) return false;

	First->PlayerRecord.BackpackSlotCount = 99;
	First->bRejectCapture = true;
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("采集回调失败直接向调用者返回失败"), Manager->CacheComponent(First));
	AddExpectedError(TEXT("存档保存失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("没有脏数据也不能把采集失败报告为成功"), Manager->SaveCachedData());
	Second->PlayerRecord.BackpackSlotCount = 72;
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("任一组件采集失败使整体保存失败"), Manager->SaveAll());
	TestEqual(TEXT("写入后返回失败不污染原记录"), Manager->GetSaveData()->Players.FindChecked(FirstID).BackpackSlotCount, 24);
	TestEqual(TEXT("正常对象仍可完成内存采集"), Manager->GetSaveData()->Players.FindChecked(SecondID).BackpackSlotCount, 72);
	TArray<uint8> AfterFailure;
	TestTrue(TEXT("失败后磁盘原文件仍可读取"), UGameplayStatics::LoadDataFromSlot(AfterFailure, Slot.Name, 0));
	TestTrue(TEXT("整体保存失败完全保留原始文件"), AfterFailure == OriginalFile);

	First->bRejectCapture = false;
	if (!TestTrue(TEXT("失败对象成功重试后恢复正常保存"), Manager->SaveAll())) return false;
	TestEqual(TEXT("重试提交最新对象状态"), Manager->GetSaveData()->Players.FindChecked(FirstID).BackpackSlotCount, 99);
	TArray<uint8> RecoveredFile;
	if (!TestTrue(TEXT("读取重试成功的完整文件"), UGameplayStatics::LoadDataFromSlot(RecoveredFile, Slot.Name, 0))) return false;

	First->PlayerRecord.BackpackSlotCount = 111;
	First->bRejectCapture = true;
	AddExpectedError(TEXT("对象存档采集失败"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("世界清理前采集失败明确回报失败"), Manager->CacheWorldBeforeCleanup(TestWorld.World));
	TestWorld.World->EndPlay(EEndPlayReason::Quit);
	AddExpectedError(TEXT("存档保存失败"), EAutomationExpectedErrorFlags::Contains, 2);
	TestFalse(TEXT("组件注销后缓存保存仍保留失败状态"), Manager->SaveCachedData());
	TestFalse(TEXT("注销后没有待采集组件也不能报告保存成功"), Manager->SaveAll());
	TArray<uint8> AfterCleanupFailure;
	TestTrue(TEXT("退出失败后文件仍可读取"), UGameplayStatics::LoadDataFromSlot(AfterCleanupFailure, Slot.Name, 0));
	TestTrue(TEXT("退出采集失败不会覆盖最近一次完整存档"), AfterCleanupFailure == RecoveredFile);
	TestEqual(TEXT("退出采集失败仍保留最后成功快照"), Manager->GetSaveData()->Players.FindChecked(FirstID).BackpackSlotCount, 99);
	return true;
}

/** 验证世界清理前快照、清理后防回写以及跨地图重新绑定的行为。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveWorldCleanupTest,
	"LxARPG.Save.WorldCleanupAndTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 实际结束第一个世界的组件，检查另一个世界及跨地图缓存仍然有效。 */
bool FLxSaveWorldCleanupTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestWorld FirstWorld;
	FLxSaveTestWorld SecondWorld;
	const FGameplayTag FirstID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	const FGameplayTag SecondID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	Manager->Initialize(Slot.Name);
	if (!Manager->LoadSave()) return false;
	ULxSaveSystemTestComponent* First = FirstWorld.AddComponent(FirstID);
	ULxSaveSystemTestComponent* Second = SecondWorld.AddComponent(SecondID);
	if (!First || !Second) return false;
	First->PlayerRecord = MakePlayerRecord(FirstID);
	Second->PlayerRecord = MakePlayerRecord(SecondID);
	Second->PlayerRecord.BackpackSlotCount = 48;
	TestTrue(TEXT("第一个世界组件注册"), Manager->RegisterComponent(First));
	TestTrue(TEXT("第二个世界组件注册"), Manager->RegisterComponent(Second));
	Manager->CacheWorldBeforeCleanup(FirstWorld.World, SecondWorld.World->PersistentLevel);
	TestEqual(TEXT("流式关卡筛选不采集其他关卡对象"), First->CaptureCount, 0);
	Manager->CacheWorldBeforeCleanup(FirstWorld.World, FirstWorld.World->PersistentLevel);
	TestEqual(TEXT("结束前仅采集目标世界"), First->CaptureCount, 1);
	TestEqual(TEXT("其他世界此时尚未采集"), Second->CaptureCount, 0);
	FirstWorld.World->EndPlay(EEndPlayReason::LevelTransition);
	TestEqual(TEXT("真实结束事件已清空模拟业务属性"), First->PlayerRecord.BackpackSlotCount, 0);
	Manager->CacheComponent(First);
	TestTrue(TEXT("清理后仍可统一保存其他世界与已有缓存"), Manager->SaveAll());
	TestEqual(TEXT("已清理对象不会再被采集"), First->CaptureCount, 1);
	TestEqual(TEXT("结束前快照未被空数据覆盖"), Manager->GetSaveData()->Players.FindChecked(FirstID).BackpackSlotCount, 24);
	TestEqual(TEXT("其他世界仍正常采集"), Manager->GetSaveData()->Players.FindChecked(SecondID).BackpackSlotCount, 48);
	ULxSaveSystemTestComponent* Replacement = SecondWorld.AddComponent(FirstID);
	if (!Replacement) return false;
	TestTrue(TEXT("切图后相同ID可重新绑定"), Manager->RegisterComponent(Replacement));
	TestEqual(TEXT("切图后从内存缓存恢复状态"), Replacement->PlayerRecord.BackpackSlotCount, 24);
	Replacement->PlayerRecord.BackpackSlotCount = 72;
	Manager->CacheWorldBeforeCleanup(SecondWorld.World);
	SecondWorld.World->EndPlay(EEndPlayReason::Quit);
	TestTrue(TEXT("所有业务组件退出后只保存缓存成功"), Manager->SaveCachedData());
	TStrongObjectPtr<ULxSaveManager> Reader(NewObject<ULxSaveManager>());
	Reader->Initialize(Slot.Name);
	if (!TestTrue(TEXT("退出存档重新加载成功"), Reader->LoadSave())) return false;
	TestEqual(TEXT("最终落盘的是退出前最新状态"), Reader->GetSaveData()->Players.FindChecked(FirstID).BackpackSlotCount, 72);
	return true;
}

/** 验证游戏运行中重复请求加载不会回滚未落盘的进度。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveRepeatedLoadTest,
	"LxARPG.Save.RepeatedLoadPreservesCache",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 同时验证管理器在成功加载后不能重新初始化到另一个存档槽。 */
bool FLxSaveRepeatedLoadTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestSlot OtherSlot;
	FLxSaveTestWorld TestWorld;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	TStrongObjectPtr<ULxSaveManager> Manager(NewObject<ULxSaveManager>());
	Manager->Initialize(Slot.Name);
	if (!Manager->LoadSave()) return false;
	ULxSaveSystemTestComponent* Player = TestWorld.AddComponent(ID);
	if (!Player) return false;
	Player->PlayerRecord = MakePlayerRecord(ID);
	if (!Manager->RegisterComponent(Player) || !Manager->SaveAll()) return false;
	Player->PlayerRecord.BackpackSlotCount = 60;
	Manager->CacheComponent(Player);
	ULxGameSaveData* PreviousCache = Manager->GetSaveData();
	TestTrue(TEXT("重复加载请求成功且幂等"), Manager->LoadSave());
	TestEqual(TEXT("重复加载保持原缓存对象"), Manager->GetSaveData(), PreviousCache);
	TestEqual(TEXT("重复加载不恢复磁盘旧状态"), Manager->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 60);
	Manager->Initialize(OtherSlot.Name);
	TestTrue(TEXT("再次初始化后保存原缓存"), Manager->SaveCachedData());
	TestFalse(TEXT("成功加载后不会向其他槽位写入"), UGameplayStatics::DoesSaveGameExist(OtherSlot.Name, 0));
	TStrongObjectPtr<ULxSaveManager> Reader(NewObject<ULxSaveManager>());
	Reader->Initialize(Slot.Name);
	if (!TestTrue(TEXT("原存档槽仍可加载"), Reader->LoadSave())) return false;
	TestEqual(TEXT("新的进度保存到原槽位"), Reader->GetSaveData()->Players.FindChecked(ID).BackpackSlotCount, 60);
	return true;
}

/** 验证坏档和未来版本不会被自动转换为空档并覆盖原始文件。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSaveInvalidFileProtectionTest,
	"LxARPG.Save.InvalidFileProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 从真实管理器产出的文件构造截断和损坏，检查失败后的原始字节。 */
bool FLxSaveInvalidFileProtectionTest::RunTest(const FString& Parameters)
{
	FLxSaveTestSlot Slot;
	FLxSaveTestWorld TestWorld;
	const FGameplayTag ID = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	TStrongObjectPtr<ULxSaveManager> Writer(NewObject<ULxSaveManager>());
	Writer->Initialize(Slot.Name);
	if (!Writer->LoadSave()) return false;
	ULxSaveSystemTestComponent* Player = TestWorld.AddComponent(ID);
	if (!Player) return false;
	Player->PlayerRecord = MakePlayerRecord(ID);
	if (!Writer->RegisterComponent(Player) || !Writer->SaveAll()) return false;
	TArray<uint8> ValidFile;
	if (!TestTrue(TEXT("读取管理器创建的有效文件"), UGameplayStatics::LoadDataFromSlot(ValidFile, Slot.Name, 0))) return false;
	if (!TestTrue(TEXT("嵌套存档包含可用于破坏的内容"), ValidFile.Num() > 32)) return false;

	/** 对每份失败样本重新创建管理器，确保加载和随后保存均不会替换源文件。 */
	const auto CheckRejectedFile = [this, &Slot](const TArray<uint8>& FileBytes, const TCHAR* Description)
	{
		if (!TestTrue(TEXT("将故障样本写入临时槽位"), UGameplayStatics::SaveDataToSlot(FileBytes, Slot.Name, 0))) return;
		TStrongObjectPtr<ULxSaveManager> Reader(NewObject<ULxSaveManager>());
		Reader->Initialize(Slot.Name);
		AddExpectedError(TEXT("存档加载失败"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(Description, Reader->LoadSave());
		TestFalse(TEXT("失败加载不进入可保存状态"), Reader->IsSaveLoaded());
		TestNull(TEXT("失败加载不暴露部分反序列化结果"), Reader->GetSaveData());
		TestFalse(TEXT("坏档之后自动保存必须失败"), Reader->SaveAll());
		TestFalse(TEXT("坏档之后最终缓存保存必须失败"), Reader->SaveCachedData());
		TArray<uint8> AfterSave;
		TestTrue(TEXT("失败后原始文件仍可读"), UGameplayStatics::LoadDataFromSlot(AfterSave, Slot.Name, 0));
		TestTrue(TEXT("失败后的自动保存完全保留原始字节"), AfterSave == FileBytes);
	};

	TArray<uint8> TruncatedFile = ValidFile;
	TruncatedFile.SetNum(TruncatedFile.Num() - 7);
	CheckRejectedFile(TruncatedFile, TEXT("有效头部但缺失尾部的文件必须拒绝"));
	TArray<uint8> CorruptedFile = ValidFile;
	CorruptedFile[CorruptedFile.Num() - 16] ^= 0x5A;
	CheckRejectedFile(CorruptedFile, TEXT("长度不变但内容已损坏的文件必须拒绝"));
	Writer->GetSaveData()->FormatVersion = 99;
	if (!TestTrue(TEXT("构造未来版本的有效校验文件"), Writer->SaveAll())) return false;
	TArray<uint8> FutureVersionFile;
	if (!UGameplayStatics::LoadDataFromSlot(FutureVersionFile, Slot.Name, 0)) return false;
	CheckRejectedFile(FutureVersionFile, TEXT("未知内容版本必须拒绝"));
	return true;
}

#endif
