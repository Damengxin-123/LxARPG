#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxStoneTerraceActor.h"
#include "../SplineRoad/LxSplineRoadActor.h"
#include "../Seawall/LxSeawallActor.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"

namespace LxTerraceTests
{
 /** 创建关闭静态转换的测试台地。 */
 ALxStoneTerraceActor* Terrace(UWorld* World,FVector Position=FVector::ZeroVector)
 {
  auto* A=World->SpawnActor<ALxStoneTerraceActor>(Position,FRotator::ZeroRotator);
  A->OptimizedMesh->bEnableNanite=false;A->RebuildTerrace();return A;
 }
 /** 用真实简单碰撞射线读取表面高度。 */
 double Height(UProceduralMeshComponent* Mesh,FVector P)
 {
  FHitResult Hit;
  return Mesh->LineTraceComponent(Hit,P+FVector(0,0,1000),P-FVector(0,0,1000),FCollisionQueryParams()) ? Hit.ImpactPoint.Z : -10000;
 }
}
/** 验证长宽高、圆角裁切、实际行走碰撞和无变化缓存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerraceGeometryTest,"LxARPG.World.StoneTerrace.Geometry",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerraceGeometryTest::RunTest(const FString& Parameters)
{
 using namespace LxTerraceTests;UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT {World->DestroyWorld(false);};
 auto* A=Terrace(World);A->SetTerraceDimensions(1800,1000,200);
 TestTrue(TEXT("台面碰撞位于设定高度"),FMath::Abs(Height(A->TerraceMesh,FVector::ZeroVector)-200)<.1);
 TestTrue(TEXT("包边大石有凸起"),FMath::Abs(Height(A->TerraceMesh,FVector(0,-480,0))-212)<.1);
 A->CornerRadius=200;A->RebuildTerrace();
 TestTrue(TEXT("圆角外不存在方形碰撞"),Height(A->TerraceMesh,FVector(899,499,0))<-9000);
 TestTrue(TEXT("圆角内仍可行走"),Height(A->TerraceMesh,FVector(700,300,0))>199);
 const int32 Updates=A->OptimizedMesh->SourceUpdateCount;A->RebuildTerrace();
 TestEqual(TEXT("重复重建复用几何"),A->OptimizedMesh->SourceUpdateCount,Updates);
 A->CornerRadius=500;A->RebuildTerrace();
 for(int32 Slot=0;Slot<A->TerraceMesh->GetNumSections();++Slot)
 { for(const FProcMeshVertex& V:A->TerraceMesh->GetProcMeshSection(Slot)->ProcVertexBuffer)
   { if(V.Position.ContainsNaN() || V.Normal.ContainsNaN()) { AddError(TEXT("极限圆角出现非法顶点"));return false; } } }
 A->bEnableCollision=false;A->RebuildTerrace();
 TestTrue(TEXT("关闭碰撞清除凸体"),A->TerraceMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 return true;
}
/** 验证部分边拼接、连接片、取消圆角、移动及删除后的恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerraceJoinTest,"LxARPG.World.StoneTerrace.JoinAndRestore",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerraceJoinTest::RunTest(const FString& Parameters)
{
 using namespace LxTerraceTests;UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT {World->DestroyWorld(false);};
 auto* A=Terrace(World);A->CornerRadius=120;A->RebuildTerrace();
 auto* B=Terrace(World,FVector(2050,0,0));B->CornerRadius=120;B->RebuildTerrace();
 TestEqual(TEXT("双方维护连接"),A->Connections.Num(),1);TestEqual(TEXT("另一台地维护连接"),B->Connections.Num(),1);
 TestTrue(TEXT("原包边位置已让出通道"),FMath::Abs(Height(A->TerraceMesh,FVector(980,0,0))-100)<.1);
 TestTrue(TEXT("间隙连接片具备碰撞"),FMath::Max(Height(A->TerraceMesh,FVector(1025,0,0)),Height(B->TerraceMesh,FVector(1025,0,0)))>99);
 TestTrue(TEXT("拼接角点没有圆角缺口"),Height(A->TerraceMesh,FVector(999,580,0))>99);
 TestTrue(TEXT("接缝处外侧石块完整延续"),Height(B->TerraceMesh,FVector(1051,580,0))>111);
 B->Size.Y=600;B->RebuildTerrace();
 TestTrue(TEXT("未拼接的边段保留大石"),Height(A->TerraceMesh,FVector(980,400,0))>111);
 B->SetActorLocation(FVector(4000,0,0));B->RebuildTerrace();
 TestTrue(TEXT("移动后双向断开"),A->Connections.IsEmpty()&&B->Connections.IsEmpty());
 TestTrue(TEXT("移走后恢复包边碰撞"),Height(A->TerraceMesh,FVector(980,0,0))>111);
 B->SetActorLocation(FVector(2050,0,0));B->RebuildTerrace();B->Destroy();
 TestTrue(TEXT("删除后恢复连接列表"),A->Connections.IsEmpty());
 TestTrue(TEXT("删除后恢复包边"),Height(A->TerraceMesh,FVector(980,0,0))>111);
 return true;
}
/** 验证道路与海堤端口、坡面、端部石移除及断开恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerraceRoadTest,"LxARPG.World.StoneTerrace.RoadAndSeawall",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerraceRoadTest::RunTest(const FString& Parameters)
{
 using namespace LxTerraceTests;UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT {World->DestroyWorld(false);};
 auto* A=Terrace(World);
 auto* R=World->SpawnActor<ALxSplineRoadActor>(FVector(1100,0,120),FRotator::ZeroRotator);
 R->OptimizedMesh->bEnableNanite=false;R->bStartCurb=true;R->RoadWidth=400;R->RebuildRoad();
 TestEqual(TEXT("道路自动接入"),A->Connections.Num(),1);
 TestTrue(TEXT("台地入口无挡路石"),FMath::Abs(Height(A->TerraceMesh,FVector(980,0,0))-100)<.1);
 TestTrue(TEXT("连接片自动适应道路小高差"),FMath::Abs(Height(A->TerraceMesh,FVector(1050,0,0))-110)<.2);
 TestTrue(TEXT("连接后道路起点封口消失"),FMath::Abs(Height(R->RoadMesh,FVector(1101,0,0))-120)<.2);
 TestTrue(TEXT("未改写道路中心线"),R->RoadSpline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::Local).IsNearlyZero());
 const int32 Before=A->OptimizedMesh->SourceUpdateCount;R->CurbWidth=60;R->CurbHeight=35;R->RebuildRoad();
 TestTrue(TEXT("修改道路边石尺寸会刷新台地过渡"),A->OptimizedMesh->SourceUpdateCount>Before && Height(A->TerraceMesh,FVector(1050,220,0))>120);
 R->SetActorLocation(FVector(1100,0,200));R->RebuildRoad();TestTrue(TEXT("高差超限断开"),A->Connections.IsEmpty());
 R->Destroy();
 auto* S=World->SpawnActor<ALxSeawallActor>(FVector(1100,0,100),FRotator::ZeroRotator);
 S->OptimizedMesh->bEnableNanite=false;S->WallOptimizedMesh->bEnableNanite=false;S->RoadWidth=400;S->RebuildRoad();
 TestEqual(TEXT("道路派生海堤同样连接"),A->Connections.Num(),1);
 S->SetActorLocation(FVector(950,0,100));S->RebuildRoad();
 TestEqual(TEXT("河堤插入台地仍连接"),A->Connections.Num(),1);
 TestTrue(TEXT("插入入口移除挡路石"),FMath::Abs(Height(A->TerraceMesh,FVector(980,0,0))-100)<.1);
 TestTrue(TEXT("插入部分不残留道路碰撞"),Height(S->RoadMesh,FVector(975,0,0))<-9000);
 TestTrue(TEXT("裁切后的河堤保持通行"),Height(S->RoadMesh,FVector(1010,0,0))>99);
 TestTrue(TEXT("裁切不改动样条控制点"),S->RoadSpline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::Local).IsNearlyZero());
 S->SetActorLocation(FVector(899,0,100));S->RebuildRoad();
 TestTrue(TEXT("内侧超过连接范围后断开"),A->Connections.IsEmpty());
 S->SetActorLocation(FVector(950,0,100));S->SetActorRotation(FRotator(0,15,0));S->RebuildRoad();
 TestEqual(TEXT("斜向插入河堤仍连接"),A->Connections.Num(),1);
 TestTrue(TEXT("斜向接入生成有效几何"),S->GenerationDiagnostic.IsEmpty());
 if(A->Connections.Num()==1 && A->Connections[0].Bridge.Num()==4)
 {
  FVector Center=FVector::ZeroVector;
  for(FVector Point:A->Connections[0].Bridge) { Center+=Point*.25; }
  TestTrue(TEXT("斜向贴角连接片仍有连续碰撞"),Height(A->TerraceMesh,A->GetActorTransform().TransformPosition(Center))>99);
 }
 S->SetActorLocation(FVector(950,0,120));S->RebuildRoad();
 TestEqual(TEXT("插入接入保留允许的高差"),A->Connections.Num(),1);
 A->bAutoConnect=false;A->RebuildTerrace();TestTrue(TEXT("任一方关闭连接恢复"),A->Connections.IsEmpty());
 return true;
}
/** 验证旋转台地、顶面高差隔离及四边接入。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTerraceRotationTest,"LxARPG.World.StoneTerrace.RotationAndHeight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxTerraceRotationTest::RunTest(const FString& Parameters)
{
 using namespace LxTerraceTests;UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT {World->DestroyWorld(false);};
 auto* A=Terrace(World);A->SetActorRotation(FRotator(0,35,0));A->RebuildTerrace();
 auto* B=Terrace(World,A->GetActorTransform().TransformPosition(FVector(0,1250,0)));B->SetActorRotation(A->GetActorRotation());B->RebuildTerrace();
 TestEqual(TEXT("旋转后的上边仍能连接"),A->Connections.Num(),1);
 B->TerraceHeight=150;B->RebuildTerrace();TestTrue(TEXT("不同标高台地不误拼"),A->Connections.IsEmpty());
 B->TerraceHeight=100;B->SetActorScale3D(FVector(2,1,1));B->RebuildTerrace();
 TestTrue(TEXT("非单位缩放给出连接说明"),!B->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
