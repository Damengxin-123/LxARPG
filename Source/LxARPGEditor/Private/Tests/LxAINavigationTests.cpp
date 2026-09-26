#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Components/SplineComponent.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/NavigationSystem/LxAINavigationRegistry.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIPointActor.h"
#include "LxARPG/LxSource/World/AINavigation/LxAINavigationTags.h"
#include "LxARPG/LxSource/World/AINavigation/LxAIRouteActor.h"
#include "Misc/ScopeExit.h"
#include "Tests/AutomationCommon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAINavigationRegistryTest,
	"LxARPG.AINavigation.Registry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAINavigationRegistryTest::RunTest(const FString& Parameters)
{
	UWorld* WorldA = UWorld::CreateWorld(EWorldType::Game, false);
	UWorld* WorldB = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT
	{
		WorldA->EndPlay(EEndPlayReason::Quit);
		WorldA->DestroyWorld(false);
		WorldB->EndPlay(EEndPlayReason::Quit);
		WorldB->DestroyWorld(false);
	};
	WorldA->InitializeActorsForPlay(FURL());
	WorldB->InitializeActorsForPlay(FURL());

	const FGameplayTag SharedRouteId = LxAITag_RouteRoot;
	const FGameplayTag SharedPointId = LxAITag_PointRoot;
	ALxAIRouteActor* RouteA = WorldA->SpawnActor<ALxAIRouteActor>();
	ALxAIRouteActor* DuplicateRouteA = WorldA->SpawnActor<ALxAIRouteActor>();
	ALxAIRouteActor* RouteB = WorldB->SpawnActor<ALxAIRouteActor>();
	RouteA->RouteId = SharedRouteId;
	DuplicateRouteA->RouteId = SharedRouteId;
	RouteB->RouteId = SharedRouteId;

	ALxAIPointActor* PointA = WorldA->SpawnActor<ALxAIPointActor>();
	ALxAIPointActor* DuplicatePointA = WorldA->SpawnActor<ALxAIPointActor>();
	ALxAIPointActor* PointB = WorldB->SpawnActor<ALxAIPointActor>();
	PointA->PointId = SharedPointId;
	DuplicatePointA->PointId = SharedPointId;
	PointB->PointId = SharedPointId;

	ULxAINavigationRegistry* Registry = NewObject<ULxAINavigationRegistry>();
	TestTrue(TEXT("首条路线应注册成功"), Registry->RegisterRoute(RouteA));
	AddExpectedError(TEXT("注册AI路线失败：世界"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("同一世界同类型的重复路线ID应被拒绝"), Registry->RegisterRoute(DuplicateRouteA));
	TestTrue(TEXT("相同路线ID可在另一个世界注册"), Registry->RegisterRoute(RouteB));
	TestEqual(TEXT("路线查询应返回世界A中的对象"), Registry->FindRoute(WorldA, SharedRouteId), RouteA);
	TestEqual(TEXT("路线查询应返回世界B中的对象"), Registry->FindRoute(WorldB, SharedRouteId), RouteB);

	Registry->UnregisterRoute(DuplicateRouteA);
	TestEqual(TEXT("按未注册对象注销不得移除同ID路线"), Registry->FindRoute(WorldA, SharedRouteId), RouteA);
	Registry->UnregisterRoute(RouteA);
	TestNull(TEXT("按对象注销后世界A路线应不可查询"), Registry->FindRoute(WorldA, SharedRouteId));

	TestTrue(TEXT("世界A点位应注册成功"), Registry->RegisterPoint(PointA));
	AddExpectedError(TEXT("注册AI点位失败：世界"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("同一世界同类型的重复点位ID应被拒绝"), Registry->RegisterPoint(DuplicatePointA));
	TestTrue(TEXT("相同点位ID可在另一个世界注册"), Registry->RegisterPoint(PointB));
	TestEqual(TEXT("点位查询应按世界隔离"), Registry->FindPoint(WorldA, SharedPointId), PointA);
	Registry->UnregisterPoint(DuplicatePointA);
	TestEqual(TEXT("按未注册对象注销不得移除同ID点位"), Registry->FindPoint(WorldA, SharedPointId), PointA);
	RouteB->Destroy();
	PointB->Destroy();
	TestNull(TEXT("路线弱引用失效后查询应立即返回空"), Registry->FindRoute(WorldB, SharedRouteId));
	TestNull(TEXT("点位弱引用失效后查询应立即返回空"), Registry->FindPoint(WorldB, SharedPointId));
	ALxAIRouteActor* ReplacementRouteB = WorldB->SpawnActor<ALxAIRouteActor>();
	ALxAIPointActor* ReplacementPointB = WorldB->SpawnActor<ALxAIPointActor>();
	ReplacementRouteB->RouteId = SharedRouteId;
	ReplacementPointB->PointId = SharedPointId;
	TestTrue(TEXT("失效路线的ID允许重新注册"), Registry->RegisterRoute(ReplacementRouteB));
	TestTrue(TEXT("失效点位的ID允许重新注册"), Registry->RegisterPoint(ReplacementPointB));
	TestEqual(TEXT("失效记录不会遮蔽同ID新路线"), Registry->FindRoute(WorldB, SharedRouteId), ReplacementRouteB);
	TestEqual(TEXT("失效记录不会遮蔽同ID新点位"), Registry->FindPoint(WorldB, SharedPointId), ReplacementPointB);
	Registry->Deinitialize();
	TestNull(TEXT("反初始化后路线缓存应为空"), Registry->FindRoute(WorldB, SharedRouteId));
	TestNull(TEXT("反初始化后点位缓存应为空"), Registry->FindPoint(WorldB, SharedPointId));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAINavigationSpatialContractTest,
	"LxARPG.AINavigation.SpatialContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAINavigationSpatialContractTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); };
	World->InitializeActorsForPlay(FURL());

	ALxAIRouteActor* Route = World->SpawnActor<ALxAIRouteActor>();
	Route->RouteSpline->ClearSplinePoints(false);
	Route->RouteSpline->AddSplinePoint(FVector(100.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local);
	TestTrue(TEXT("路线样条应在编辑视口绘制"), Route->RouteSpline->bDrawDebug);
	Route->AddRoutePoint();
	Route->AddRoutePoint();
	TestEqual(TEXT("可连续添加多个路径点"), Route->RouteSpline->GetNumberOfSplinePoints(), 3);
	TestTrue(TEXT("新增路径点应沿末段方向延伸"),
		Route->RouteSpline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::Local)
			.Equals(FVector(1100.0f, 0.0f, 0.0f), UE_KINDA_SMALL_NUMBER));
	Route->SetActorTransform(FTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(1000.0f, 2000.0f, 3000.0f), FVector(2.0f)));
	const TArray<FVector> WorldRoutePoints = Route->GetWorldRoutePoints();
	TestEqual(TEXT("路线应返回配置点数量"), WorldRoutePoints.Num(), 3);
	if (!WorldRoutePoints.IsEmpty())
	{
		TestTrue(TEXT("局部路线点应经Actor平移、旋转和缩放转换到世界空间"),
			WorldRoutePoints[0].Equals(FVector(1000.0f, 2200.0f, 3000.0f), UE_KINDA_SMALL_NUMBER));
	}

	ALxAIPointActor* Point = World->SpawnActor<ALxAIPointActor>();
	Point->RangeRadiusMeters = 50.0f;
	Point->SetActorLocation(FVector(100.0f, 200.0f, 300.0f));
	Point->SetActorScale3D(FVector(3.0f, 0.5f, 7.0f));
	TestEqual(TEXT("50米点位半径应转换为5000厘米"), Point->GetRangeRadiusCentimeters(), 5000.0f);
	TestTrue(TEXT("球体半径边界应判定为范围内"),
		Point->IsWorldLocationInRange(Point->GetWorldCenter() + FVector(5000.0f, 0.0f, 0.0f)));
	TestFalse(TEXT("超过球体半径的位置应判定为范围外"),
		Point->IsWorldLocationInRange(Point->GetWorldCenter() + FVector(5000.1f, 0.0f, 0.0f)));
	TestEqual(TEXT("Actor缩放不得改变点位的米制范围语义"), Point->GetRangeRadiusCentimeters(), 5000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxAINavigationActorLifecycleTest,
	"LxARPG.AINavigation.ActorLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLxAINavigationActorLifecycleTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper TestWorld;
	if (!TestWorld.CreateTestWorld(EWorldType::Game))
	{
		TestWorld.ForwardErrorMessages(this);
		return false;
	}
	UWorld* World = TestWorld.GetTestWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (!TestNotNull(TEXT("测试世界应关联已初始化的真实游戏实例"), GameInstance))
	{
		return false;
	}
	World->InitializeActorsForPlay(FURL());

	ULxGameInstanceSubsystem* Subsystem = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (!TestNotNull(TEXT("游戏实例初始化应自动创建项目子系统"), Subsystem))
	{
		return false;
	}
	ULxAINavigationRegistry* Registry = Subsystem->GetAINavigationRegistry();
	if (!TestNotNull(TEXT("项目子系统应拥有AI导航注册表"), Registry))
	{
		return false;
	}

	ALxAIRouteActor* Route = World->SpawnActor<ALxAIRouteActor>();
	Route->RouteId = LxAITag_RouteRoot;
	Route->DispatchBeginPlay();
	TestEqual(TEXT("路线BeginPlay应通过游戏实例子系统自动注册"),
		Registry->FindRoute(World, LxAITag_RouteRoot), Route);
	Route->Destroy();
	TestNull(TEXT("路线Destroy触发EndPlay后应自动注销"),
		Registry->FindRoute(World, LxAITag_RouteRoot));

	ALxAIPointActor* Point = World->SpawnActor<ALxAIPointActor>();
	Point->PointId = LxAITag_PointRoot;
	Point->DispatchBeginPlay();
	TestEqual(TEXT("点位BeginPlay应通过游戏实例子系统自动注册"),
		Registry->FindPoint(World, LxAITag_PointRoot), Point);
	Point->Destroy();
	TestNull(TEXT("点位Destroy触发EndPlay后应自动注销"),
		Registry->FindPoint(World, LxAITag_PointRoot));
	return true;
}

#endif
