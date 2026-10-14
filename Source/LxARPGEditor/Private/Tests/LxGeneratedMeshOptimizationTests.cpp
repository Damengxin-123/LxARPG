#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/Level.h"
#include "Materials/Material.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshCompiler.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/WoodenDoor/LxWoodenDoorActor.h"
#include "Editor.h"
#include "UObject/SavePackage.h"
#include "Misc/Paths.h"
#include "LxARPG/LxSource/World/BrickWall/LxBrickWallActor.h"
#include "LxARPG/LxSource/World/StoneFloor/LxStoneFloorActor.h"
#include "LxARPG/LxSource/World/StonePillar/LxStonePillarActor.h"
#include "LxARPG/LxSource/World/TimberPanel/LxTimberPanelActor.h"
#include "LxARPG/LxSource/World/TileRoof/LxTileRoofActor.h"
#include "LxARPG/LxSource/World/CargoPile/LxCargoPileActor.h"
#include "LxARPG/LxSource/World/SplineRoad/LxSplineRoadActor.h"
#include "LxARPG/LxSource/World/SplineSurface/LxSplineSurfaceActor.h"
#include "LxARPG/LxSource/World/RingSurface/LxRingSurfaceActor.h"
#include "LxARPG/LxSource/World/RingCylinder/LxRingCylinderActor.h"
#include "LxARPG/LxSource/World/StraightStair/LxStraightStairActor.h"
#include "LxARPG/LxSource/World/BlockChunk/LxBlockChunkActor.h"
#include "LxARPG/LxSource/World/RoundBuilding/LxRoundBuildingActor.h"
#include "Components/SplineComponent.h"

/** 覆盖十二类生成器的真实 Nanite 数据、重复构造复用以及源碰撞保留。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedMeshNaniteTest,"LxARPG.World.GeneratedMesh.NaniteAndReuse",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedMeshNaniteTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("独立测试世界"),World)) { return false; }
 const TArray<UClass*> Classes={ALxBrickWallActor::StaticClass(),ALxStoneFloorActor::StaticClass(),
  ALxStonePillarActor::StaticClass(),ALxTimberPanelActor::StaticClass(),ALxTileRoofActor::StaticClass(),
  ALxCargoPileActor::StaticClass(),ALxSplineRoadActor::StaticClass(),ALxSplineSurfaceActor::StaticClass(),
  ALxRingSurfaceActor::StaticClass(),ALxRingCylinderActor::StaticClass(),ALxStraightStairActor::StaticClass(),
  ALxBlockChunkActor::StaticClass(),ALxWoodenDoorActor::StaticClass()};
 for (UClass* Class : Classes)
 {
  AActor* Actor=World->SpawnActor<AActor>(Class);
  if (!TestNotNull(Class->GetName()+TEXT("实例"),Actor)) { continue; }
  UStaticMeshComponent* Render=Actor->FindComponentByClass<UStaticMeshComponent>();
  if (!TestNotNull(Class->GetName()+TEXT("渲染组件"),Render)) { continue; }
  TestTrue(Class->GetName()+TEXT("编辑时不生成Nanite"),!Render->GetStaticMesh() || !Render->GetStaticMesh()->HasValidNaniteData());
  if(ULxGeneratedMeshRenderComponent* Optimized=Cast<ULxGeneratedMeshRenderComponent>(Render))
  {
   TestNull(TEXT("程序化预览不创建静态网格"),Render->GetStaticMesh());
   TestTrue(TEXT("程序化预览可见"),CastChecked<UProceduralMeshComponent>(Optimized->GetAttachParent())->IsVisible());
  }
  LxGeneratedMeshBuild::PrepareWorld(World);
  FStaticMeshCompilingManager::Get().FinishAllCompilation();
  UStaticMesh* Mesh=Render->GetStaticMesh();
  if (!TestNotNull(Class->GetName()+TEXT("生成网格"),Mesh)) { continue; }
  TestTrue(Class->GetName()+TEXT("具有真实Nanite数据"),Mesh->HasValidNaniteData());
  TestFalse(Class->GetName()+TEXT("网格可被关卡保存"),Mesh->HasAnyFlags(RF_Transient));
  Actor->OnConstruction(Actor->GetActorTransform());
  LxGeneratedMeshBuild::PrepareWorld(World);
  TestTrue(Class->GetName()+TEXT("重复构造复用同一网格"),Render->GetStaticMesh()==Mesh);
  if (ULxGeneratedMeshRenderComponent* Optimized=Cast<ULxGeneratedMeshRenderComponent>(Render))
  {
   UProceduralMeshComponent* Source=Cast<UProceduralMeshComponent>(Optimized->GetAttachParent());
   if (!TestNotNull(TEXT("保留原程序化源"),Source)) { continue; }
   const int32 Sections=Source->GetNumSections();
   const int32 Convexes=Source->GetBodySetup()->AggGeom.ConvexElems.Num();
   TestFalse(TEXT("程序网格不重复显示和投影"),Source->IsVisible());
   TestTrue(TEXT("优化网格可见"),Optimized->IsVisible());
   TestEqual(TEXT("优化网格不重复碰撞"),Optimized->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
   Optimized->bEnableNanite=false;
   Optimized->Synchronize(Source);
   TestTrue(TEXT("关闭优化恢复原始渲染"),Source->IsVisible() && !Optimized->IsVisible());
   Optimized->bEnableNanite=true;
   Optimized->Synchronize(Source);
   Optimized->FinalizeMesh();
   TestTrue(TEXT("重新开启复用缓存"),Optimized->GetStaticMesh()==Mesh);
   TestEqual(TEXT("切换渲染不改碰撞"),Source->GetBodySetup()->AggGeom.ConvexElems.Num(),Convexes);
   TestEqual(TEXT("切换渲染不改原始分区"),Source->GetNumSections(),Sections);
  }
 }
 return true;
}

/** 验证 PIE/复制需要的缓存属性参与序列化，尺寸变化与关闭 Nanite 能正确失效。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedMeshCacheTest,"LxARPG.World.GeneratedMesh.DuplicateAndInvalidate",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedMeshCacheTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("独立测试世界"),World)) { return false; }
 ALxStoneFloorActor* Floor=World->SpawnActor<ALxStoneFloorActor>();
 ALxStoneFloorActor* Copy=DuplicateObject<ALxStoneFloorActor>(Floor,World->PersistentLevel);
 if (!TestNotNull(TEXT("复制对象"),Copy)) { return false; }
 const int32 Builds=Copy->GeometryBuildCount;
 UStaticMesh* Before=Copy->PanelMesh->GetStaticMesh();
 Copy->OnConstruction(Copy->GetActorTransform());
 TestEqual(TEXT("复制后无需重新生成"),Copy->GeometryBuildCount,Builds);
 TestTrue(TEXT("复制后沿用已复制网格"),Copy->PanelMesh->GetStaticMesh()==Before);
 Copy->SizeHandle.X+=200;
 Copy->OnConstruction(Copy->GetActorTransform());
 TestEqual(TEXT("尺寸变化仅生成一次"),Copy->GeometryBuildCount,Builds+1);
 TestTrue(TEXT("源实例不受影响"),Floor->PanelMesh->GetStaticMesh()!=Copy->PanelMesh->GetStaticMesh());
 Copy->bEnableNanite=false;
 Copy->OnConstruction(Copy->GetActorTransform());
 FStaticMeshCompilingManager::Get().FinishAllCompilation();
 TestFalse(TEXT("关闭Nanite后使用普通完整网格"),Copy->PanelMesh->GetStaticMesh()->HasValidNaniteData());
 ALxSplineRoadActor* Road=World->SpawnActor<ALxSplineRoadActor>();
 Road->OptimizedMesh->FinalizeMesh();
 UStaticMesh* RoadMesh=Road->OptimizedMesh->GetStaticMesh();
 Road->RoadWidth+=100;
 Road->RebuildRoad();
 TestTrue(TEXT("道路形状变化只显示程序化预览"),Road->RoadMesh->IsVisible() && !Road->OptimizedMesh->IsVisible());
 TestTrue(TEXT("编辑不触发静态转换"),Road->OptimizedMesh->GetStaticMesh()==RoadMesh);
 Road->OptimizedMesh->FinalizeMesh();
 TestTrue(TEXT("完成转换后使用新尺寸网格"),Road->OptimizedMesh->GetStaticMesh()!=RoadMesh);
 UMaterial* Translucent=NewObject<UMaterial>(GetTransientPackage());
 Translucent->BlendMode=BLEND_Translucent;
 Road->RoadMaterial=Translucent;
 Road->RebuildRoad();
 Road->OptimizedMesh->FinalizeMesh();
 TestTrue(TEXT("透明材质自动使用完整回退网格"),Road->OptimizedMesh->IsForceDisableNanite());
 Road->RoadMaterial=nullptr;
 Road->RebuildRoad();
 Road->OptimizedMesh->FinalizeMesh();
 TestFalse(TEXT("恢复不透明材质后重新启用Nanite"),Road->OptimizedMesh->IsForceDisableNanite());
 Road->ClearRoad();
 TestFalse(TEXT("清空道路不会留下旧渲染"),Road->OptimizedMesh->IsVisible());
 UWorld* GameWorld=UWorld::CreateWorld(EWorldType::Game,false);
 if (TestNotNull(TEXT("运行时回退测试世界"),GameWorld))
 {
  ALxSplineRoadActor* RuntimeRoad=GameWorld->SpawnActor<ALxSplineRoadActor>();
  TestTrue(TEXT("运行时新生成道路保留原渲染"),RuntimeRoad->RoadMesh->IsVisible());
  TestNull(TEXT("运行时不会同步烘焙Nanite"),RuntimeRoad->OptimizedMesh->GetStaticMesh());
  ALxStoneFloorActor* RuntimeFloor=GameWorld->SpawnActor<ALxStoneFloorActor>();
  TestFalse(TEXT("运行时静态生成器不同步烘焙Nanite"),RuntimeFloor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
  GameWorld->DestroyWorld(false);
 }
 return true;
}
/** 验证连续编辑不做完整构建，保存及运行前会提交最新参数并完成转换。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedMeshDeferredTest,"LxARPG.World.GeneratedMesh.DeferredLifecycle",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedMeshDeferredTest::RunTest(const FString& Parameters)
{
 UPackage* Package=CreatePackage(*FString::Printf(TEXT("/Temp/LxDeferredMesh_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,TEXT("延迟网格验证"),Package);
 if(!TestNotNull(TEXT("独立保存验证世界"),World)) { return false; }
 World->SetFlags(RF_Public|RF_Standalone);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxSplineRoadActor* Road=World->SpawnActor<ALxSplineRoadActor>();
 ALxStoneFloorActor* Floor=World->SpawnActor<ALxStoneFloorActor>();
 for(int32 I=0; I<6; ++I)
 {
  Road->RoadWidth+=25; Road->RebuildRoad();
  Floor->SizeHandle.X+=25; Floor->OnConstruction(Floor->GetActorTransform());
  TestNull(TEXT("连续修改道路不生成静态网格"),Road->OptimizedMesh->GetStaticMesh());
  TestFalse(TEXT("连续修改石地板不烘焙Nanite"),Floor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
 }
 const FString Filename=FPaths::ProjectSavedDir()/TEXT("Automation/延迟网格验证.umap");
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
 FSavePackageArgs Args;
 Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 TestTrue(TEXT("保存独立验证关卡"),UPackage::SavePackage(Package,World,*Filename,Args));
 FStaticMeshCompilingManager::Get().FinishAllCompilation();
 TestTrue(TEXT("程序化道路在保存时转为完整网格"),Road->OptimizedMesh->GetStaticMesh() && Road->OptimizedMesh->GetStaticMesh()->HasValidNaniteData());
 TestTrue(TEXT("静态预览在保存时升级为完整网格"),Floor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
 // 覆盖刚拖动手柄、尚未等到防抖提交就直接按运行的场景。
 Floor->SizeHandle.X+=75;
 const int32 Builds=Floor->GeometryBuildCount;
 Road->RoadWidth+=100;
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("运行前提交最后一次尺寸修改"),Floor->GeometryBuildCount,Builds+1);
 TestTrue(TEXT("运行前完成待构建静态预览"),Floor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
 TestTrue(TEXT("运行前道路宽度为最新值"),FMath::IsNearlyEqual(Road->RoadMesh->GetProcMeshSection(0)->SectionLocalBox.GetSize().Y,static_cast<double>(Road->RoadWidth),0.1));
 TestFalse(TEXT("运行前清除道路待构建标记"),Road->OptimizedMesh->bPendingMeshBuild);
 UWorld* GameWorld=UWorld::CreateWorld(EWorldType::Game,false);
 if(TestNotNull(TEXT("运行时复制验证世界"),GameWorld))
 {
  ALxSplineRoadActor* RuntimeRoad=DuplicateObject<ALxSplineRoadActor>(Road,GameWorld->PersistentLevel);
  ALxStoneFloorActor* RuntimeFloor=DuplicateObject<ALxStoneFloorActor>(Floor,GameWorld->PersistentLevel);
  RuntimeRoad->RegisterAllComponents(); RuntimeFloor->RegisterAllComponents();
  RuntimeRoad->RebuildRoad(); RuntimeFloor->OnConstruction(RuntimeFloor->GetActorTransform());
  TestTrue(TEXT("游戏中复用已完成的道路网格"),RuntimeRoad->OptimizedMesh->GetStaticMesh() && RuntimeRoad->OptimizedMesh->GetStaticMesh()->HasValidNaniteData() && RuntimeRoad->OptimizedMesh->IsVisible());
  TestTrue(TEXT("游戏中复用已完成的石地板网格"),RuntimeFloor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
  RuntimeRoad->Destroy(); RuntimeFloor->Destroy(); GameWorld->DestroyWorld(false);
 }
 UStaticMesh* Finished=Floor->PanelMesh->GetStaticMesh();
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("重复准备复用完整网格"),Floor->PanelMesh->GetStaticMesh()==Finished);
 // 材质与碰撞仍在编辑时生效，不必等待完整转换。
 Road->RoadWidth+=10; Road->RebuildRoad();
 TestTrue(TEXT("已完成网格编辑后重新显示最新程序化预览"),Road->RoadMesh->IsVisible() && !Road->OptimizedMesh->IsVisible());
 TestTrue(TEXT("编辑预览仍保留碰撞"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num()>0);
 // 模拟旧关卡：缓存是原生静态网格，但仍属于生成器实例。
 Floor->PanelMesh->SetStaticMesh(NewObject<UStaticMesh>(Floor));
 Floor->SizeHandle.X+=50;
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("旧版本缓存也进入运行前迁移"),Floor->PanelMesh->GetStaticMesh()->IsA<ULxGeneratedStaticMesh>() && Floor->PanelMesh->GetStaticMesh()->HasValidNaniteData());
 return true;
}
/** 验证连续保存、只改一个对象、最后一次样条修改以及游戏启动均使用增量路径。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedMeshIncrementalTest,"LxARPG.World.GeneratedMesh.IncrementalSaveAndPlay",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedMeshIncrementalTest::RunTest(const FString& Parameters)
{
 UPackage* Package=CreatePackage(*FString::Printf(TEXT("/Temp/LxIncremental_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,TEXT("增量网格验证"),Package);
 if(!TestNotNull(TEXT("隔离验证世界"),World)) { return false; }
 World->SetFlags(RF_Public|RF_Standalone);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 TArray<ULxGeneratedMeshRenderComponent*> Renderers;
 const TArray<UClass*> Classes={ALxSplineRoadActor::StaticClass(),ALxSplineSurfaceActor::StaticClass(),
  ALxRingSurfaceActor::StaticClass(),ALxRingCylinderActor::StaticClass(),ALxStraightStairActor::StaticClass(),
  ALxBlockChunkActor::StaticClass(),ALxRoundBuildingActor::StaticClass()};
 for(int32 Index=0; Index<Classes.Num(); ++Index)
 {
  AActor* Actor=World->SpawnActor<AActor>(Classes[Index],FVector(Index*10000,0,0),FRotator::ZeroRotator);
  Renderers.Add(Actor->FindComponentByClass<ULxGeneratedMeshRenderComponent>());
 }
 ALxSplineRoadActor* Road=CastChecked<ALxSplineRoadActor>(Renderers[0]->GetOwner());
 // 使用非默认点数，重载后手动增点仍应保留现有轮廓。
 auto* Surface=CastChecked<ALxSplineSurfaceActor>(Renderers[1]->GetOwner());
 Surface->ControlPointCount=6; Surface->RebuildSurface();
 ALxStoneFloorActor* Floor=World->SpawnActor<ALxStoneFloorActor>();
 LxGeneratedMeshBuild::PrepareWorld(World);
 TArray<int32> Updates,Conversions;
 for(auto* Render : Renderers)
 {
  Updates.Add(Render->SourceUpdateCount); Conversions.Add(Render->GeometryConversionCount);
  TestTrue(TEXT("生成后输入快照一致"),Render->IsSourceCurrent());
 }
 const int32 FloorBuilds=Floor->GeometryBuildCount;
 const double Start=FPlatformTime::Seconds();
 for(int32 Repeat=0; Repeat<3; ++Repeat) { LxGeneratedMeshBuild::PrepareWorld(World); }
 AddInfo(FString::Printf(TEXT("未修改的八个生成对象连续准备三次：%.3f毫秒"),(FPlatformTime::Seconds()-Start)*1000));
 // 真正序列化两次，覆盖组件 PreSave 与世界准备的重复入口。
 const FString Filename=FPaths::ProjectSavedDir()/TEXT("Automation/增量网格验证.umap");
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 for(int32 Repeat=0; Repeat<2; ++Repeat)
 { TestTrue(TEXT("保存隔离关卡"),UPackage::SavePackage(Package,World,*Filename,Args)); }
 for(int32 Index=0; Index<Renderers.Num(); ++Index)
 {
  TestEqual(TEXT("重复保存不生成源几何"),Renderers[Index]->SourceUpdateCount,Updates[Index]);
  TestEqual(TEXT("重复保存不转换或计算几何哈希"),Renderers[Index]->GeometryConversionCount,Conversions[Index]);
 }
 // 不调用构造函数，模拟防抖尚未提交时立即保存或运行。
 Road->RoadWidth+=80;
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("最后一次道路尺寸修改仅提交一次"),Road->OptimizedMesh->SourceUpdateCount,Updates[0]+1);
 TestEqual(TEXT("变动道路仅转换一次"),Road->OptimizedMesh->GeometryConversionCount,Conversions[0]+1);
 TestEqual(TEXT("未修改静态生成器复用几何"),Floor->GeometryBuildCount,FloorBuilds);
 for(int32 Index=1; Index<Renderers.Num(); ++Index)
 {
  TestEqual(TEXT("只改道路不重建其他源几何"),Renderers[Index]->SourceUpdateCount,Updates[Index]);
  TestEqual(TEXT("只改道路不转换其他网格"),Renderers[Index]->GeometryConversionCount,Conversions[Index]);
 }
 const FVector Point=Surface->SurfaceSpline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::Local);
 Surface->SurfaceSpline->SetLocationAtSplinePoint(0,Point+FVector(75,0,0),ESplineCoordinateSpace::Local,true);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("未触发构造的样条控制点修改不遗漏"),Renderers[1]->SourceUpdateCount,Updates[1]+1);
 TestEqual(TEXT("样条修改仅转换一次"),Renderers[1]->GeometryConversionCount,Conversions[1]+1);
 // 复制持久化缓存后，游戏启动不能重新生成、转换或哈希。
 UWorld* GameWorld=UWorld::CreateWorld(EWorldType::Game,false);
 if(TestNotNull(TEXT("游戏启动验证世界"),GameWorld))
 {
  ALxSplineRoadActor* Copy=DuplicateObject<ALxSplineRoadActor>(Road,GameWorld->PersistentLevel);
  Copy->RegisterAllComponents();
  const int32 BeforeUpdates=Copy->OptimizedMesh->SourceUpdateCount;
  const int32 BeforeConversions=Copy->OptimizedMesh->GeometryConversionCount;
  Copy->OnConstruction(Copy->GetActorTransform());
  Copy->DispatchBeginPlay();
  TestEqual(TEXT("游戏启动不生成源几何"),Copy->OptimizedMesh->SourceUpdateCount,BeforeUpdates);
  TestEqual(TEXT("游戏启动不转换或哈希"),Copy->OptimizedMesh->GeometryConversionCount,BeforeConversions);
  TestTrue(TEXT("游戏中显示已完成缓存"),Copy->OptimizedMesh->IsVisible());
  Copy->Destroy(); GameWorld->DestroyWorld(false);
 }
 // 手动清空源后不能恢复旧缓存，也不能在下一次保存重新生成道路。
 Road->ClearRoad(); Road->OptimizedMesh->FinalizeMesh();
 TestNull(TEXT("清空源会移除旧静态缓存"),Road->OptimizedMesh->GetStaticMesh());
 const int32 EmptyConversions=Road->OptimizedMesh->GeometryConversionCount;
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("空几何同样缓存，不重复转换"),Road->OptimizedMesh->GeometryConversionCount,EmptyConversions);
 return true;
}
/** 验证增量提交会更新依赖邻居，同时保留远处已缓存的连接组。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedMeshDependencyTest,"LxARPG.World.GeneratedMesh.IncrementalDependencies",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedMeshDependencyTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 if(!TestNotNull(TEXT("依赖验证世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 TArray<ALxSplineRoadActor*> Roads;
 for(double X : {0.,2100.,10000.,12100.})
 { Roads.Add(World->SpawnActor<ALxSplineRoadActor>(FVector(X,0,0),FRotator::ZeroRotator)); }
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("邻近道路建立连接"),Roads[0]->RoadConnections.Num(),1);
 TestEqual(TEXT("远处道路独立建立连接"),Roads[2]->RoadConnections.Num(),1);
 TArray<int32> Updates,Conversions;
 for(auto* Road : Roads)
 { Updates.Add(Road->OptimizedMesh->SourceUpdateCount); Conversions.Add(Road->OptimizedMesh->GeometryConversionCount); }
 Roads[1]->SetActorLocation(FVector(3000,0,0));
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("移动后原邻居断开连接"),Roads[0]->RoadConnections.IsEmpty());
 TestTrue(TEXT("变化传递至原邻居"),Roads[0]->OptimizedMesh->SourceUpdateCount>Updates[0]);
 for(int32 Index=2; Index<4; ++Index)
 {
  TestEqual(TEXT("远处连接组不重新生成"),Roads[Index]->OptimizedMesh->SourceUpdateCount,Updates[Index]);
  TestEqual(TEXT("远处连接组不重新转换"),Roads[Index]->OptimizedMesh->GeometryConversionCount,Conversions[Index]);
 }
 auto* Outer=World->SpawnActor<ALxRoundBuildingActor>(FVector(0,10000,0),FRotator::ZeroRotator);
 auto* Inner=World->SpawnActor<ALxRoundBuildingActor>(FVector(0,10000,0),FRotator::ZeroRotator);
 Inner->Radius=200; Inner->RebuildBuilding();
 LxGeneratedMeshBuild::PrepareWorld(World);
 const int32 OuterUpdates=Outer->OptimizedMesh->SourceUpdateCount;
 Inner->SetActorLocation(FVector(5000,10000,0));
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("移开内层建筑后恢复原邻居楼板"),Outer->OptimizedMesh->SourceUpdateCount>OuterUpdates);
 const int32 RestoredUpdates=Outer->OptimizedMesh->SourceUpdateCount;
 const int32 RestoredConversions=Outer->OptimizedMesh->GeometryConversionCount;
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("依赖更新后再次保存不重建"),Outer->OptimizedMesh->SourceUpdateCount,RestoredUpdates);
 TestEqual(TEXT("依赖更新后再次保存不转换"),Outer->OptimizedMesh->GeometryConversionCount,RestoredConversions);
 return true;
}
#endif
