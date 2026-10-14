#if WITH_DEV_AUTOMATION_TESTS
#include "LxSeawallActor.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"

namespace LxSeawallTests
{
 /** 写入真实样条控制点，测试编辑器使用的同一生成入口。 */
 void Path(ALxSeawallActor* Wall,const TArray<FVector>& Points,bool bCurve=false,bool bClosed=false)
 {
  Wall->RoadSpline->ClearSplinePoints(false);
  for(int32 I=0;I<Points.Num();++I)
  {
   Wall->RoadSpline->AddSplinePoint(Points[I],ESplineCoordinateSpace::Local,false);
   Wall->RoadSpline->SetSplinePointType(I,bCurve ? ESplinePointType::Curve : ESplinePointType::Linear,false);
  }
  Wall->RoadSpline->SetClosedLoop(bClosed,true); Wall->RebuildSeawall();
 }
 /** 查询组件的简单碰撞。 */
 bool Hit(UPrimitiveComponent* Component,FVector A,FVector B)
 { FHitResult Result; return Component->LineTraceComponent(Result,A,B,FCollisionQueryParams()); }
}

/** 检查坡脚尺寸、镜像、曲线、闭环和急弯诊断，避免出现旧网格或无碰撞残留。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSeawallGeometryTest,"LxARPG.World.Seawall.Geometry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSeawallGeometryTest::RunTest(const FString& Parameters)
{
 using namespace LxSeawallTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* W=World->SpawnActor<ALxSeawallActor>();
 W->bAutoConnect=false; W->WallOptimizedMesh->bEnableNanite=false; W->OptimizedMesh->bEnableNanite=false;
 W->RebuildSeawall();
 TestTrue(TEXT("默认海堤生成成功"),W->SeawallDiagnostic.IsEmpty() && W->GeneratedSlabCount>20);
 if(!W->WallMesh->GetProcMeshSection(0)) { return false; }
 const FBox Bounds=W->WallMesh->GetProcMeshSection(0)->SectionLocalBox;
 TestTrue(TEXT("海侧45度坡脚外扩480厘米"),FMath::IsNearlyEqual(Bounds.Max.Y,804.,.01));
 TestTrue(TEXT("陆侧接近垂直"),Bounds.Min.Y>-351 && Bounds.Min.Y<-349);
 TestTrue(TEXT("堤底低于路面500厘米"),FMath::IsNearlyEqual(Bounds.Min.Z,-500.,.01));
 TestTrue(TEXT("路面可行走"),Hit(W->RoadMesh,{1000,0,100},{1000,0,-100}));
 TestTrue(TEXT("海坡与陆墙具有简单碰撞"),Hit(W->WallMesh,{1000,1500,-250},{1000,0,-250}) && Hit(W->WallMesh,{1000,-1000,-250},{1000,0,-250}));
 W->bSeaOnLeft=true; W->RebuildSeawall();
 TestTrue(TEXT("左右面海正确镜像"),FMath::IsNearlyEqual(W->WallMesh->GetProcMeshSection(0)->SectionLocalBox.Min.Y,-Bounds.Max.Y,.01));
 W->SeaSlopeAngle=60; W->RebuildSeawall();
 TestTrue(TEXT("更陡坡角缩小占地"),W->WallMesh->GetProcMeshSection(0)->SectionLocalBox.Min.Y>-610);
 W->bSeaOnLeft=false;
 Path(W,{{0,0,0},{2000,0,0},{4000,1200,100}},true);
 TestTrue(TEXT("三维曲线生成成功"),W->SeawallDiagnostic.IsEmpty() && W->GeneratedSlabCount>0);
 Path(W,{{0,0,0},{3000,0,0},{3000,3000,0},{0,3000,0}},true,true);
 TestTrue(TEXT("路面能通过但坡脚半径不足的闭环被拒绝"),!W->SeawallDiagnostic.IsEmpty());
 Path(W,{{0,0,0},{6000,0,0},{6000,6000,0},{0,6000,0}},true,true);
 TestTrue(TEXT("闭环生成成功"),W->SeawallDiagnostic.IsEmpty() && W->WallMesh->GetNumSections()>0);
 Path(W,{{0,0,0},{1000,0,0},{1000,20,0},{0,20,0}});
 TestTrue(TEXT("过急弯道拒绝生成并清空道路及碰撞"),!W->SeawallDiagnostic.IsEmpty() && W->RoadMesh->GetNumSections()==0 && W->WallMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 Path(W,{{0,0,0},{2000,0,0}});
 TestTrue(TEXT("修复曲线后正常恢复"),W->SeawallDiagnostic.IsEmpty() && W->GeneratedSlabCount>0);
 W->bEnableCollision=false; W->RebuildSeawall();
 TestTrue(TEXT("关闭碰撞同时清除堤身凸体"),W->WallMesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision && W->WallMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 W->ClearRoad();
 TestTrue(TEXT("清空按钮清除全部几何"),W->WallMesh->GetNumSections()==0 && W->RoadMesh->GetNumSections()==0 && W->GeneratedSlabCount==0);
 return true;
}

/** 检查普通道路与海堤的端点、丁字开口及邻居重建路径。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSeawallConnectionTest,"LxARPG.World.Seawall.RoadConnections",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSeawallConnectionTest::RunTest(const FString& Parameters)
{
 using namespace LxSeawallTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 // 先创建普通道路，验证端点连接的持有者不依赖演员创建顺序。
 auto* Road=World->SpawnActor<ALxSplineRoadActor>(FVector(2100,0,0),FRotator::ZeroRotator);
 auto* Wall=World->SpawnActor<ALxSeawallActor>(); Wall->RebuildSeawall();
 TestEqual(TEXT("海堤与普通道路互相登记"),Wall->RoadConnections.Num(),1);
 TestTrue(TEXT("端点连接片顶部连续"),Hit(Wall->RoadMesh,{2050,0,100},{2050,0,-100}));
 TestTrue(TEXT("连接片下方有堤身支撑"),Hit(Wall->WallMesh,{2050,-800,-250},{2050,0,-250}));
 const int32 Updates=Wall->WallOptimizedMesh->SourceUpdateCount;
 Road->SetActorLocation({2500,0,0}); Road->RebuildRoad();
 TestTrue(TEXT("移动邻路刷新堤身缓存"),Wall->WallOptimizedMesh->SourceUpdateCount>Updates);
 TestTrue(TEXT("断开后连接支撑清除"),!Hit(Wall->WallMesh,{2050,-800,-250},{2050,0,-250}));
 Road->SetActorLocation({1000,-450,0}); Road->SetActorRotation({0,-90,0}); Road->RoadWidth=300; Road->RebuildRoad();
 TestTrue(TEXT("陆侧自动丁字开口"),Wall->RoadConnections.Num()==1 && Wall->RoadConnections[0].bJunction);
 TestTrue(TEXT("路口开口可通行且有下部支撑"),Hit(Wall->RoadMesh,{1000,-400,100},{1000,-400,-100}) && Hit(Wall->WallMesh,{1000,-700,-250},{1000,0,-250}));
 Road->Destroy();
 TestTrue(TEXT("删除支路清除开口"),Wall->RoadConnections.IsEmpty());
 return true;
}
#endif
