#include "LxAISpawnPointTestCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxAISpawnPointSaveComponent.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxAISpawnPointSaveData.h"
#include "UObject/UnrealType.h"
#include <limits>

namespace
{
	/** 独占测试世界，销毁时不访问用户存档和场景资源。 */
	struct FLxAISpawnPointTestWorld
	{
		/** 按需创建编辑预览或游戏世界，游戏世界保持尚未开始运行以便恢复组成。 */
		explicit FLxAISpawnPointTestWorld(EWorldType::Type WorldType = EWorldType::Game)
			: World(UWorld::CreateWorld(WorldType, false))
		{
			if (WorldType == EWorldType::Game) World->InitializeActorsForPlay(FURL());
		}

		/** 自动清理测试角色、点位与定时器。 */
		~FLxAISpawnPointTestWorld()
		{
			World->EndPlay(EEndPlayReason::Quit);
			World->DestroyWorld(false);
		}

		/** 创建可在无导航网格场景中使用的刷怪点。 */
		ALxAISpawnPointActor* AddPoint()
		{
			ALxAISpawnPointActor* Point = World->SpawnActor<ALxAISpawnPointActor>();
			if (Point)
			{
				Point->bSpawnOnNavigation = false;
				Point->RangeRadiusMeters = 10.0f;
			}
			return Point;
		}

		/** 当前测试拥有的临时世界。 */
		UWorld* World;
	};

	/** 创建独立概率项，使测试明确区分列表顺序和首领抽取。 */
	FLxAISpawnMonsterType MakeType(float Probability, float BossProbability = 0.0f)
	{
		FLxAISpawnMonsterType Type;
		Type.NormalCharacterClass = ALxAISpawnPointTestCharacter::StaticClass();
		Type.BossCharacterClass = ALxAISpawnPointTestBoss::StaticClass();
		Type.SpawnProbabilityPercent = Probability;
		Type.BossUpgradeProbabilityPercent = BossProbability;
		return Type;
	}

	/** 向组成快照添加实际类及数量，允许用例构造重复条目和失败类。 */
	void AddPopulation(FLxAISpawnPointSaveRecord& Record, UClass* CharacterClass, int32 Count)
	{
		FLxAISpawnPopulationEntry& Entry = Record.Population.AddDefaulted_GetRef();
		Entry.CharacterClass = CharacterClass;
		Entry.Count = Count;
	}

	/** 按实际类读取已采集数量，不依赖散列表遍历导致的条目顺序。 */
	int32 CountClass(const FLxAISpawnPointSaveRecord& Record, UClass* CharacterClass)
	{
		int32 Count = 0;
		for (const FLxAISpawnPopulationEntry& Entry : Record.Population)
		{
			if (Entry.CharacterClass.Get() == CharacterClass) Count += Entry.Count;
		}
		return Count;
	}
}

/** 验证上下限、零波动和区间内增加一只的严格概率边界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointPopulationMathTest,
	"LxARPG.AISpawnPoint.PopulationBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 数量规则使用固定随机值，避免概率性断言产生偶发失败。 */
bool FLxAISpawnPointPopulationMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("低于下限补到下限，与本轮随机值无关"), ALxAISpawnPointActor::CalculateSpawnCount(1, 5, 2, 1.0f), 2);
	TestEqual(TEXT("处于下限且随机值小于一半时增加一只"), ALxAISpawnPointActor::CalculateSpawnCount(3, 5, 2, 0.499f), 1);
	TestEqual(TEXT("随机值恰为一半不创建"), ALxAISpawnPointActor::CalculateSpawnCount(3, 5, 2, 0.5f), 0);
	TestEqual(TEXT("上限前一只允许增加"), ALxAISpawnPointActor::CalculateSpawnCount(6, 5, 2, 0.0f), 1);
	TestEqual(TEXT("达到上限不能再创建"), ALxAISpawnPointActor::CalculateSpawnCount(7, 5, 2, 0.0f), 0);
	TestEqual(TEXT("超过上限时不会继续创建"), ALxAISpawnPointActor::CalculateSpawnCount(9, 5, 2, 0.0f), 0);
	TestEqual(TEXT("零波动时补足基准"), ALxAISpawnPointActor::CalculateSpawnCount(0, 5, 0, 1.0f), 5);
	TestEqual(TEXT("零波动且数量已满足时不随机增怪"), ALxAISpawnPointActor::CalculateSpawnCount(5, 5, 0, 0.0f), 0);
	TestEqual(TEXT("波动大于基准不会产生负下限"), ALxAISpawnPointActor::CalculateSpawnCount(0, 2, 5, 1.0f), 0);
	TestEqual(TEXT("基准和波动均零时保持空群体"), ALxAISpawnPointActor::CalculateSpawnCount(0, 0, 0, 0.0f), 0);
	TestEqual(TEXT("非有限概率不会在区间内补怪"), ALxAISpawnPointActor::CalculateSpawnCount(4, 5, 2,
		std::numeric_limits<float>::quiet_NaN()), 0);
	TestEqual(TEXT("错误的极大参数不会整数溢出或超出安全数量上限"),
		ALxAISpawnPointActor::CalculateSpawnCount(4096, MAX_int32, MAX_int32, 0.0f), 0);
	return true;
}

/** 验证顺序判定、无效项过滤和低概率重复轮次的等价选择。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointSelectionTest,
	"LxARPG.AISpawnPoint.OrderedTypeSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 使用已知分位点确认靠前条目的成功概率先消耗本轮机会。 */
bool FLxAISpawnPointSelectionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("首领升级默认概率为百分之五"), FLxAISpawnMonsterType().BossUpgradeProbabilityPercent, 5.0f);
	TArray<FLxAISpawnMonsterType> Types = {MakeType(50.0f), MakeType(100.0f)};
	TestEqual(TEXT("首项命中时不继续后项"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.49f), 0);
	TestEqual(TEXT("首项未命中后由百分百项兜底"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.51f), 1);
	Types[0].SpawnProbabilityPercent = 100.0f;
	TestEqual(TEXT("首项百分百阻止后续类型出现"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.999f), 0);
	Types[0].SpawnProbabilityPercent = 0.0f;
	TestEqual(TEXT("零概率项即使随机值为零也不会出现"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.0f), 1);
	Types[1].SpawnProbabilityPercent = 0.0f;
	TestEqual(TEXT("全部零概率立即返回无候选"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.5f), INDEX_NONE);
	Types = {MakeType(50.0f), MakeType(50.0f)};
	TestEqual(TEXT("重试空轮后首项的累计权重为三分之二"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.66f), 0);
	TestEqual(TEXT("重试空轮后后项占最后三分之一"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.67f), 1);
	Types[0].NormalCharacterClass = nullptr;
	TestEqual(TEXT("缺少普通角色类的配置不消耗后续项概率"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.0f), 1);
	Types[1].SpawnProbabilityPercent = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("非有限创建概率按禁用处理"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.5f), INDEX_NONE);
	Types.Reset();
	TestEqual(TEXT("空列表不会访问不存在的类型"), ALxAISpawnPointActor::SelectMonsterTypeIndex(Types, 0.5f), INDEX_NONE);
	return true;
}

/** 验证场景实例可编辑性，以及三种标记组件的可见性和隐藏开关。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointEditorContractTest,
	"LxARPG.AISpawnPoint.EditorAndMarkerContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 读取反射属性和真实组件，防止参数仅能修改类默认值或标记开关只更新部分组件。 */
bool FLxAISpawnPointEditorContractTest::RunTest(const FString& Parameters)
{
	const FName InstanceProperties[] = {
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, SpawnPointId),
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, BaseMonsterCount),
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, MonsterCountVariation),
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, CheckIntervalSeconds),
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, MonsterTypes),
		GET_MEMBER_NAME_CHECKED(ALxAISpawnPointActor, bHideMarkerInGame)};
	for (const FName Name : InstanceProperties)
	{
		const FProperty* Property = ALxAISpawnPointActor::StaticClass()->FindPropertyByName(Name);
		if (!TestNotNull(*Name.ToString(), Property)) continue;
		TestTrue(*FString::Printf(TEXT("%s 支持编辑"), *Name.ToString()), Property->HasAnyPropertyFlags(CPF_Edit));
		TestFalse(*FString::Printf(TEXT("%s 支持场景实例覆盖"), *Name.ToString()), Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
		TestFalse(TEXT("参数具有显示名称"), Property->GetMetaData(TEXT("DisplayName")).IsEmpty());
		TestFalse(TEXT("参数具有面板分类"), Property->GetMetaData(TEXT("Category")).IsEmpty());
	}
	const FName TypeProperties[] = {
		GET_MEMBER_NAME_CHECKED(FLxAISpawnMonsterType, NormalCharacterClass),
		GET_MEMBER_NAME_CHECKED(FLxAISpawnMonsterType, BossCharacterClass),
		GET_MEMBER_NAME_CHECKED(FLxAISpawnMonsterType, SpawnProbabilityPercent),
		GET_MEMBER_NAME_CHECKED(FLxAISpawnMonsterType, BossUpgradeProbabilityPercent)};
	for (const FName Name : TypeProperties)
	{
		const FProperty* Property = FLxAISpawnMonsterType::StaticStruct()->FindPropertyByName(Name);
		if (!TestNotNull(*Name.ToString(), Property)) continue;
		TestTrue(TEXT("怪物列表中的类型和概率可编辑"), Property->HasAnyPropertyFlags(CPF_Edit));
		TestFalse(TEXT("怪物列表中的类型和概率支持实例覆盖"), Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}
	FLxAISpawnPointTestWorld PreviewWorld(EWorldType::Editor);
	ALxAISpawnPointActor* Point = PreviewWorld.AddPoint();
	if (!TestNotNull(TEXT("创建点位"), Point)) return false;
	TestNotNull(TEXT("默认附带存档组件"), Point->SaveComponent.Get());
	USphereComponent* Range = Point->FindComponentByClass<USphereComponent>();
	UArrowComponent* Arrow = Point->FindComponentByClass<UArrowComponent>();
	UTextRenderComponent* Text = Point->FindComponentByClass<UTextRenderComponent>();
	if (!TestNotNull(TEXT("活动范围标记"), Range) || !TestNotNull(TEXT("方向标记"), Arrow)
		|| !TestNotNull(TEXT("文字标记"), Text)) return false;
	Point->OnConstruction(Point->GetActorTransform());
	for (USceneComponent* Marker : TArray<USceneComponent*>{Range, Arrow, Text})
	{
		// IsVisible 始终应用运行时隐藏开关；编辑视口使用独立的编辑可见性与渲染路径。
		TestTrue(TEXT("标记在编辑态保持可见"), Marker->IsVisibleInEditor());
		TestTrue(TEXT("编辑世界允许渲染标记"), Marker->ShouldRender());
		TestTrue(TEXT("默认运行时隐藏标记"), Marker->bHiddenInGame);
	}
	Point->bHideMarkerInGame = false;
	Point->OnConstruction(Point->GetActorTransform());
	for (USceneComponent* Marker : TArray<USceneComponent*>{Range, Arrow, Text})
	{
		TestFalse(TEXT("关闭开关后所有标记都可在运行时显示"), Marker->bHiddenInGame);
		TestTrue(TEXT("关闭隐藏开关后编辑态标记继续可见"), Marker->IsVisibleInEditor());
	}
	Point->RangeRadiusMeters = 4.5f;
	Point->SetActorScale3D(FVector(3.0f));
	Point->OnConstruction(Point->GetActorTransform());
	TestEqual(TEXT("米转换为厘米且不随Actor缩放改变活动半径"), Point->GetRangeRadiusCentimeters(), 450.0f);
	FLxAISpawnPointTestWorld GameWorld;
	ALxAISpawnPointActor* GamePoint = GameWorld.AddPoint();
	if (!TestNotNull(TEXT("创建游戏世界点位"), GamePoint)) return false;
	const TArray<USceneComponent*> GameMarkers = {
		GamePoint->FindComponentByClass<USphereComponent>(),
		GamePoint->FindComponentByClass<UArrowComponent>(),
		GamePoint->FindComponentByClass<UTextRenderComponent>()};
	for (USceneComponent* Marker : GameMarkers)
	{
		if (!TestNotNull(TEXT("游戏世界具备对应标记组件"), Marker)) return false;
		TestFalse(TEXT("游戏世界中默认实际不可见"), Marker->IsVisible());
		TestFalse(TEXT("游戏世界默认不渲染标记"), Marker->ShouldRender());
	}
	GamePoint->bHideMarkerInGame = false;
	GamePoint->OnConstruction(GamePoint->GetActorTransform());
	for (USceneComponent* Marker : GameMarkers)
	{
		TestTrue(TEXT("游戏世界中关闭隐藏开关后实际可见"), Marker->IsVisible());
		TestTrue(TEXT("游戏世界中关闭隐藏开关后允许渲染标记"), Marker->ShouldRender());
	}
	return true;
}

/** 验证恢复的真实类型、归属注入时机、存活计数及失败事务的完整回滚。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointRestoreTest,
	"LxARPG.AISpawnPoint.RestoreCompositionAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 存档只写类型和数量，恢复时不受当前首领抽取概率和基准数量变化影响。 */
bool FLxAISpawnPointRestoreTest::RunTest(const FString& Parameters)
{
	FLxAISpawnPointTestWorld TestWorld;
	ALxAISpawnPointActor* Point = TestWorld.AddPoint();
	if (!TestNotNull(TEXT("创建待恢复点位"), Point)) return false;
	Point->SpawnPointId = FGameplayTag::RequestGameplayTag(TEXT("角色.测试角色"));
	Point->BaseMonsterCount = 1;
	Point->MonsterCountVariation = 0;
	Point->MonsterTypes = {MakeType(100.0f, 100.0f)};
	FLxAISpawnPointSaveRecord Record;
	Record.SaveID = Point->SpawnPointId;
	AddPopulation(Record, ALxAISpawnPointTestCharacter::StaticClass(), 1);
	AddPopulation(Record, ALxAISpawnPointTestBoss::StaticClass(), 1);
	AddPopulation(Record, ALxAISpawnPointTestCharacter::StaticClass(), 1);
	if (!TestTrue(TEXT("按原始组成恢复三只角色"), Point->RestorePopulation(Record))) return false;
	TestEqual(TEXT("恢复数量不受改小的刷怪上限截断"), Point->GetCurrentMonsterCount(), 3);
	TArray<ALxAICharacter*> OriginalMembers = Point->GetLivingMonsters();
	for (ALxAICharacter* Character : OriginalMembers)
	{
		TestEqual(TEXT("角色所属对象指向刷怪点"), Character->GetOwner(), static_cast<AActor*>(Point));
		TestEqual(TEXT("行为树通过角色取得所属刷怪点"), Character->GetSpawnPoint(), Point);
		const ALxAISpawnPointTestCharacter* Observed = Cast<ALxAISpawnPointTestCharacter>(Character);
		TestTrue(TEXT("构造脚本执行前已经注入归属"), Observed && Observed->bBindingObservedDuringConstruction);
	}
	FLxAISpawnPointSaveRecord Captured;
	TestTrue(TEXT("采集实际存活组成"), Point->CapturePopulation(Captured));
	TestEqual(TEXT("快照保留点位标签"), Captured.SaveID, Point->SpawnPointId);
	TestEqual(TEXT("重复条目按实际类聚合"), Captured.Population.Num(), 2);
	TestEqual(TEXT("普通角色不会按当前百分百概率重新升级首领"), CountClass(Captured, ALxAISpawnPointTestCharacter::StaticClass()), 2);
	TestEqual(TEXT("首领保留实际角色类"), CountClass(Captured, ALxAISpawnPointTestBoss::StaticClass()), 1);
	if (!TestEqual(TEXT("恢复后可逐只检查三只原始怪物"), OriginalMembers.Num(), 3)) return false;

	FLxAISpawnPointSaveRecord FailingRecord;
	FailingRecord.SaveID = Point->SpawnPointId;
	AddPopulation(FailingRecord, ALxAISpawnPointTestCharacter::StaticClass(), 2);
	AddPopulation(FailingRecord, ALxAISpawnPointFailingTestCharacter::StaticClass(), 1);
	TestFalse(TEXT("角色构造中销毁会使整次恢复失败"), Point->RestorePopulation(FailingRecord));
	TestEqual(TEXT("失败后保留原有三只角色"), Point->GetCurrentMonsterCount(), 3);
	for (ALxAICharacter* Character : OriginalMembers)
	{
		TestTrue(TEXT("原有角色实例未被提前销毁或替换"), IsValid(Character) && Point->GetLivingMonsters().Contains(Character));
	}
	int32 WorldLivingCount = 0;
	for (TActorIterator<ALxAISpawnPointTestCharacter> It(TestWorld.World); It; ++It)
	{
		if (IsValid(*It) && !It->IsActorBeingDestroyed()) ++WorldLivingCount;
	}
	TestEqual(TEXT("失败事务没有在世界中遗留新角色"), WorldLivingCount, 3);
	FailingRecord = Record;
	FailingRecord.SaveID = FGameplayTag::RequestGameplayTag(TEXT("角色.木桩"));
	TestFalse(TEXT("不同标签的存档不能恢复到当前点位"), Point->RestorePopulation(FailingRecord));

	OriginalMembers[0]->GetCharacterAttributeComponent()->SetCharacterDead();
	TestEqual(TEXT("死亡动画期间的角色立即退出存活数量"), Point->GetCurrentMonsterCount(), 2);
	OriginalMembers[1]->SetSpawnPoint(nullptr);
	TestEqual(TEXT("已解除归属的角色不计入数量"), Point->GetCurrentMonsterCount(), 1);
	TestTrue(TEXT("可保存排除死亡和失去归属后的结果"), Point->CapturePopulation(Captured));
	int32 SavedTotal = 0;
	for (const FLxAISpawnPopulationEntry& Entry : Captured.Population) SavedTotal += Entry.Count;
	TestEqual(TEXT("存档仅包含仍存活且归属当前点位的角色"), SavedTotal, 1);
	return true;
}

/** 验证实际运行时补怪、首领升级边界及无候选配置下的有界退出。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAISpawnPointRuntimePopulationTest,
	"LxARPG.AISpawnPoint.RuntimePopulation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/** 通过真实开始运行事件启用检查，未配置标签的点位按无存档模式运行。 */
bool FLxAISpawnPointRuntimePopulationTest::RunTest(const FString& Parameters)
{
	FLxAISpawnPointTestWorld TestWorld;
	ALxAISpawnPointActor* Point = TestWorld.AddPoint();
	if (!TestNotNull(TEXT("创建运行时点位"), Point)) return false;
	Point->BaseMonsterCount = 2;
	Point->MonsterCountVariation = 0;
	Point->CheckIntervalSeconds = 0.25f;
	Point->MonsterTypes = {MakeType(100.0f, 100.0f)};
	Point->CheckPopulation();
	TestEqual(TEXT("初始化前的手动检查不会抢先生成角色"), Point->GetCurrentMonsterCount(), 0);
	Point->DispatchBeginPlay();
	TestEqual(TEXT("无标签点位启动后补足下限"), Point->GetCurrentMonsterCount(), 2);
	TArray<ALxAICharacter*> Members = Point->GetLivingMonsters();
	if (!TestEqual(TEXT("获得两只活怪"), Members.Num(), 2)) return false;
	for (ALxAICharacter* Character : Members)
	{
		TestTrue(TEXT("百分百首领概率只创建首领类型"), Character->IsA<ALxAISpawnPointTestBoss>());
	}
	Point->CheckPopulation();
	TestEqual(TEXT("零波动保持数量恰好等于基准"), Point->GetCurrentMonsterCount(), 2);
	// 定时器一帧只执行一次；仅在本测试作用域内推进帧号，结束时恢复引擎原有值。
	TGuardValue<uint64> FrameGuard(GFrameCounter, GFrameCounter + 1);
	FTimerManager& TimerManager = TestWorld.World->GetTimerManager();
	TimerManager.Tick(0.0f);
	Members[0]->GetCharacterAttributeComponent()->SetCharacterDead();
	Point->MonsterTypes[0].BossUpgradeProbabilityPercent = 0.0f;
	++GFrameCounter;
	TimerManager.Tick(0.10f);
	TestEqual(TEXT("距离检查周期尚差0.15秒时不提前补怪"), Point->GetCurrentMonsterCount(), 1);
	Point->CheckIntervalSeconds = 0.50f;
	++GFrameCounter;
	TimerManager.Tick(0.16f);
	TestEqual(TEXT("累计超过0.25秒后定时器自动补足数量"), Point->GetCurrentMonsterCount(), 2);
	int32 NormalCount = 0;
	for (ALxAICharacter* Character : Point->GetLivingMonsters())
	{
		if (Character->GetClass() == ALxAISpawnPointTestCharacter::StaticClass()) ++NormalCount;
	}
	TestEqual(TEXT("零首领概率补充普通角色"), NormalCount, 1);
	Members[1]->GetCharacterAttributeComponent()->SetCharacterDead();
	++GFrameCounter;
	TimerManager.Tick(0.26f);
	TestEqual(TEXT("修改后的下一轮使用0.50秒间隔，不再沿用0.25秒"), Point->GetCurrentMonsterCount(), 1);
	++GFrameCounter;
	TimerManager.Tick(0.25f);
	TestEqual(TEXT("新间隔到期后自动补足"), Point->GetCurrentMonsterCount(), 2);
	Point->BaseMonsterCount = 3;
	Point->MonsterTypes[0].SpawnProbabilityPercent = 0.0f;
	Point->CheckPopulation();
	TestEqual(TEXT("全零创建概率即使不足下限也立即退出"), Point->GetCurrentMonsterCount(), 2);
	return true;
}

#endif
