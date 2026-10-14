#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxSplineRoadActor.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "../SplineSurface/LxSplineSurfaceActor.h"

/** 验证道路爬坡、绕序、碰撞、重建清理、转弯和闭环，及连接片的旧行为兼容性。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxHarborRoadTest, "LxARPG.World.HarborRoad.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxHarborRoadTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 if(!TestNotNull(TEXT("创建测试世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxSplineRoadActor* Road=World->SpawnActor<ALxSplineRoadActor>();
 if(!TestNotNull(TEXT("创建道路"),Road)) { return false; }
 Road->RoadSpline->SetLocationAtSplinePoint(1,FVector(2000,0,400),ESplineCoordinateSpace::Local,true);
 Road->RebuildRoad();
 TestTrue(TEXT("坡道成功生成"),Road->GenerationDiagnostic.IsEmpty());
 FProcMeshSection* Section=Road->RoadMesh->GetProcMeshSection(0);
 if(!TestNotNull(TEXT("坡道路面"),Section)) { return false; }
 TestTrue(TEXT("保留400厘米高差"),FMath::IsNearlyEqual(Section->SectionLocalBox.Max.Z,400.0,0.01));
 TestTrue(TEXT("路基碰撞存在"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num()>0);
 TestEqual(TEXT("直线坡道的路基与两侧路缘石仅需三个凸体"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),3);
 for(double X : {50.0,1000.0,1950.0})
 {
  FHitResult Hit;
  FCollisionQueryParams Query;
  Query.bTraceComplex=false;
  const bool bHit=Road->RoadMesh->LineTraceComponent(Hit,FVector(X,0,1000),FVector(X,0,-100),Query);
  TestTrue(TEXT("合并后的坡道起点、中段和末端碰撞连续"),bHit);
  if(bHit) { TestTrue(TEXT("合并凸体顶面保留坡高"),FMath::IsNearlyEqual(Hit.ImpactPoint.Z,X*0.2,0.1)); }
 }
 Road->RoadMesh->ClearCollisionConvexMeshes();
 Road->CollisionDataVersion=0;
 Road->DispatchBeginPlay();
 TestEqual(TEXT("旧关卡进入游戏时自动恢复优化碰撞"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),3);
 TestEqual(TEXT("迁移后保存当前碰撞版本"),Road->CollisionDataVersion,1);
 // 迁移会替换网格缓冲，后续检查必须重新取得网格段。
 Section=Road->RoadMesh->GetProcMeshSection(0);
 if(!TestNotNull(TEXT("迁移后坡道路面仍存在"),Section)) { return false; }
 bool bTop=false;
 for(const FProcMeshVertex& V : Section->ProcVertexBuffer)
 {
  if(V.Normal.Z>0.9) { bTop=true; TestTrue(TEXT("坡面法线朝上且朝坡下"),V.Normal.X<0); break; }
 }
 TestTrue(TEXT("具有向上顶面"),bTop);
 const int32 OriginalVertices=Section->ProcVertexBuffer.Num();
 Road->RebuildRoad();
 TestEqual(TEXT("反复重建不累加网格"),Road->RoadMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),OriginalVertices);
 Road->bEnableCollision=false;
 Road->bLeftCurb=false;
 Road->RebuildRoad();
 const FProcMeshSection* Left=Road->RoadMesh->GetProcMeshSection(1);
 TestTrue(TEXT("关闭路缘石清理旧槽"),!Left || Left->ProcVertexBuffer.IsEmpty());
 TestEqual(TEXT("关闭碰撞清理凸体"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),0);

 // 平滑宽半径弯道必须保留高差且不误报折叠。
 Road->RoadSpline->ClearSplinePoints(false);
 for(const FVector& P : {FVector(0,0,0),FVector(2000,0,200),FVector(4000,2000,400)})
 { Road->RoadSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false); }
 Road->RoadSpline->UpdateSpline();
 Road->RebuildRoad();
 TestTrue(TEXT("宽半径转弯爬坡成功"),Road->GenerationDiagnostic.IsEmpty());
 TestNotNull(TEXT("弯道生成路面"),Road->RoadMesh->GetProcMeshSection(0));

 // 闭合曲线必须无端盖且首尾位置一致。
 Road->RoadSpline->ClearSplinePoints(false);
 for(const FVector& P : {FVector(-3000,-3000,0),FVector(3000,-3000,0),FVector(3000,3000,0),FVector(-3000,3000,0)})
 { Road->RoadSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false); }
 Road->RoadSpline->SetClosedLoop(true,true);
 Road->RebuildRoad();
 TestTrue(TEXT("闭环成功"),Road->GenerationDiagnostic.IsEmpty());
 Section=Road->RoadMesh->GetProcMeshSection(0);
 if(TestNotNull(TEXT("闭环路面"),Section))
 {
  const auto& V=Section->ProcVertexBuffer;
  TestTrue(TEXT("闭环网格不含端盖"),V.Num()%16==0);
  TestTrue(TEXT("首尾截面一致"),V[0].Position.Equals(V[V.Num()-15].Position,0.01));
  TestTrue(TEXT("闭环UV完整周期"),FMath::IsNearlyEqual(
   V[V.Num()-15].UV0.Y-V[0].UV0.Y,FMath::RoundToDouble(V[V.Num()-15].UV0.Y-V[0].UV0.Y),0.001));
 }
 // 窄半径回转必须拒绝生成；避免不可行走的折叠几何体。
 Road->RoadWidth=6000;
 Road->RebuildRoad();
 TestFalse(TEXT("过宽急弯报告诊断"),Road->GenerationDiagnostic.IsEmpty());
 TestEqual(TEXT("失败后不残留旧几何体"),Road->RoadMesh->GetNumSections(),0);

 ALxSplineSurfaceActor* Patch=World->SpawnActor<ALxSplineSurfaceActor>();
 Patch->bPreserveSplineHeights=true;
 Patch->SurfaceSpline->SetLocationAtSplinePoint(1,FVector(250,-250,100),ESplineCoordinateSpace::Local,true);
 Patch->SurfaceSpline->SetLocationAtSplinePoint(2,FVector(250,250,100),ESplineCoordinateSpace::Local,true);
 Patch->RebuildSurface();
 TestTrue(TEXT("连接片保留控制点高度"),FMath::IsNearlyEqual(
  Patch->SurfaceSpline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::Local).Z,100.0));
 TestNotNull(TEXT("坡面连接片生成"),Patch->SurfaceMesh->GetProcMeshSection(0));
 Patch->bPreserveSplineHeights=false;
 Patch->RebuildSurface();
 TestTrue(TEXT("关闭选项恢复原平面行为"),FMath::IsNearlyZero(
  Patch->SurfaceSpline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::Local).Z));
 return true;
}
/** 验证独立路口、圆角、纹理方向、开口碰撞及关闭后的清理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxHarborJunctionTest, "LxARPG.World.HarborRoad.Junctions",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxHarborJunctionTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 if(!TestNotNull(TEXT("创建路口测试世界"),World)) { return false; }
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxSplineRoadActor* Road=World->SpawnActor<ALxSplineRoadActor>();
 if(!TestNotNull(TEXT("创建路口道路"),Road)) { return false; }
 Road->OptimizedMesh->bEnableNanite=false;
 TestFalse(TEXT("默认关闭左路口"),Road->bLeftJunction);
 TestFalse(TEXT("默认关闭右路口"),Road->bRightJunction);
 /** 验证路面三角形面积、法线和UV有限，防止重叠及退化面。 */
 const auto TopArea=[&]()
 {
  double Area=0;
  const FProcMeshSection* Mesh=Road->RoadMesh->GetProcMeshSection(0);
  if(!TestNotNull(TEXT("路口网格存在"),Mesh)) { return Area; }
  for(int32 I=0; I+2<Mesh->ProcIndexBuffer.Num(); I+=3)
  {
   const FProcMeshVertex& A=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I]];
   const FProcMeshVertex& B=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+1]];
   const FProcMeshVertex& C=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+2]];
   const FVector Cross=FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position);
   TestTrue(TEXT("没有退化或无效三角形"),!Cross.ContainsNaN() && Cross.SizeSquared()>1.e-12);
   TestFalse(TEXT("纹理坐标有限"),A.UV0.ContainsNaN());
   if(A.Normal.Z>0.9) { TestTrue(TEXT("顶面绕序朝上"),Cross.Z>0); Area+=Cross.Z*0.5; }
  }
  return Area;
 };
 /** 检查简单碰撞可行走且没有横跨路口的路缘石。 */
 const auto Trace=[&](double X,double Y,bool bExpected)
 {
  FHitResult Hit;
  const bool bHit=Road->RoadMesh->LineTraceComponent(Hit,FVector(X,Y,800),FVector(X,Y,-100),FCollisionQueryParams());
  TestEqual(TEXT("支路及圆角碰撞范围"),bHit,bExpected);
  if(bHit) { TestTrue(TEXT("开口路面无路缘石阻挡"),FMath::IsNearlyZero(Hit.ImpactPoint.Z,0.1)); }
 };
 /** 圆角小于路缘石宽度时也不能出现顶面反折或无效法线。 */
 const auto CheckCurbs=[&]()
 {
  for(int32 Slot=1; Slot<3; ++Slot)
  {
   const FProcMeshSection* Mesh=Road->RoadMesh->GetProcMeshSection(Slot);
   if(!Mesh) { continue; }
   bool bValid=true;
   for(int32 I=0; I+2<Mesh->ProcIndexBuffer.Num(); I+=3)
   {
    const FProcMeshVertex& A=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I]];
    const FProcMeshVertex& B=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+1]];
    const FProcMeshVertex& C=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+2]];
    if(FMath::IsNearlyEqual(A.Position.Z,static_cast<double>(Road->CurbHeight),0.001)
     && FMath::IsNearlyEqual(B.Position.Z,static_cast<double>(Road->CurbHeight),0.001)
     && FMath::IsNearlyEqual(C.Position.Z,static_cast<double>(Road->CurbHeight),0.001))
    {
     const bool bUp=FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position).Z>0;
     if(bValid && !bUp) { AddError(FString::Printf(TEXT("路缘石槽%d，半径%g，反折顶点 %s / %s / %s"),Slot,Road->LeftJunctionRadius,*A.Position.ToString(),*B.Position.ToString(),*C.Position.ToString())); }
     bValid &= bUp;
    }
   }
   TestTrue(TEXT("圆角路缘石顶面不反折"),bValid);
   const float Radius=Slot==1 ? Road->LeftJunctionRadius : Road->RightJunctionRadius;
   if(Radius>=Road->CurbWidth*2)
   {
    bool bFullWidth=true;
    for(int32 I=0; I+3<Mesh->ProcVertexBuffer.Num(); I+=4)
    {
     const auto& V=Mesh->ProcVertexBuffer;
     if(FMath::IsNearlyEqual(V[I].Position.Z,static_cast<double>(Road->CurbHeight),0.001)
      && FMath::IsNearlyEqual(V[I+1].Position.Z,static_cast<double>(Road->CurbHeight),0.001)
      && FMath::IsNearlyEqual(V[I+2].Position.Z,static_cast<double>(Road->CurbHeight),0.001)
      && FMath::IsNearlyEqual(V[I+3].Position.Z,static_cast<double>(Road->CurbHeight),0.001))
     {
      const float ExpectedWidth=Road->CurbWidth-2*Road->CurbBevel-0.05f;
      bFullWidth &= FVector::Dist2D(V[I].Position,V[I+3].Position)>=ExpectedWidth
       && FVector::Dist2D(V[I+1].Position,V[I+2].Position)>=ExpectedWidth;
     }
    }
    TestTrue(TEXT("非均匀采样下正常圆角路缘石保持完整宽度"),bFullWidth);
   }
  }
 };
 Road->bLeftJunction=true;
 Road->LeftJunctionRadius=0;
 Road->RebuildRoad();
 TestTrue(TEXT("直角丁字路口成功"),Road->GenerationDiagnostic.IsEmpty());
 CheckCurbs();
 TestTrue(TEXT("丁字路面没有重叠或缺面"),FMath::IsNearlyEqual(TopArea(),1800000.0,1.0));
 Trace(1000,-301,true); Trace(1000,-1290,true); Trace(1000,800,false); Trace(600,-500,false);
 bool bTurnedUV=false;
 for(const FProcMeshVertex& V : Road->RoadMesh->GetProcMeshSection(0)->ProcVertexBuffer)
 {
  if(V.Position.Y<-500 && V.Normal.Z>0.9)
  { bTurnedUV=true; TestTrue(TEXT("支路纹理横轴沿主路纵向，纹理旋转九十度"),FMath::Abs(V.Tangent.TangentX.X)>0.99); }
 }
 TestTrue(TEXT("存在支路纹理样本"),bTurnedUV);
 Road->LeftJunctionRadius=150;
 Road->RebuildRoad();
 TestTrue(TEXT("圆角丁字路口成功"),Road->GenerationDiagnostic.IsEmpty());
 CheckCurbs();
 const double Expected=1800000+2*150*150*(1-PI/4);
 TestTrue(TEXT("圆角填充面积符合四分之一圆"),FMath::Abs(TopArea()-Expected)<400);
 Trace(675,-315,true); Trace(600,-450,false);
 for(float Spacing : {10.0f,37.0f,75.0f,250.0f})
 {
  Road->SampleSpacing=Spacing;
  Road->LeftJunctionRadius=400;
  Road->RebuildRoad();
  TestTrue(TEXT("不同采样密度的圆角能够生成"),Road->GenerationDiagnostic.IsEmpty());
  CheckCurbs();
 }
 Road->SampleSpacing=75; Road->LeftJunctionRadius=150;
 Road->bRightJunction=true;
 Road->RightJunctionRadius=0;
 Road->RightJunctionLength=500;
 Road->RebuildRoad();
 TestTrue(TEXT("十字路口成功"),Road->GenerationDiagnostic.IsEmpty());
 Trace(1000,301,true); Trace(1000,790,true); Trace(1000,900,false);
 Road->RightJunctionDistance=1500;
 Road->RebuildRoad();
 TestTrue(TEXT("左右错位路口成功"),Road->GenerationDiagnostic.IsEmpty());
 Trace(1500,700,true); Trace(900,700,false); Trace(900,-700,true);
#if WITH_EDITOR
 FProperty* HandleProperty=FindFProperty<FProperty>(ALxSplineRoadActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxSplineRoadActor,LeftJunctionHandle));
 TestTrue(TEXT("旧手柄保留数据但不再暴露编辑入口"),HandleProperty && !HandleProperty->HasAnyPropertyFlags(CPF_Edit));
 Road->LeftJunctionHandle=FVector(800,-300,0);
 FPropertyChangedEvent HandleEvent(HandleProperty);
 Road->PostEditChangeProperty(HandleEvent);
 TestTrue(TEXT("拖动左手柄改变左侧位置"),FMath::IsNearlyEqual(Road->LeftJunctionDistance,800.0f,1.0f));
 TestTrue(TEXT("左手柄不改变右侧位置"),FMath::IsNearlyEqual(Road->RightJunctionDistance,1500.0f,0.01f));
#endif
 for(float Radius : {0.01f,1.0f,20.0f,150.0f,100000.0f})
 {
  Road->LeftJunctionRadius=Radius;
  Road->LeftJunctionDistance=-100;
  Road->LeftJunctionLength=50;
  Road->RebuildRoad();
  TestTrue(TEXT("端点与极端半径自动约束"),Road->GenerationDiagnostic.IsEmpty());
  CheckCurbs();
 }
 Road->bLeftJunction=false; Road->bRightJunction=false;
 Road->RebuildRoad();
 TestTrue(TEXT("关闭路口恢复原路面"),FMath::IsNearlyEqual(TopArea(),1200000.0,1.0));
 TestEqual(TEXT("关闭路口恢复三个凸体"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),3);
 Road->bRightJunction=true;
 Road->RightJunctionDistance=1000;
 Road->RoadSpline->SetLocationAtSplinePoint(1,FVector(2000,0,400),ESplineCoordinateSpace::Local,true);
 Road->RebuildRoad();
 TestTrue(TEXT("爬坡道路的支路成功"),Road->GenerationDiagnostic.IsEmpty());
 FHitResult SlopeHit;
 if(TestTrue(TEXT("坡道支路碰撞存在"),Road->RoadMesh->LineTraceComponent(SlopeHit,FVector(1000,700,800),FVector(1000,700,-100),FCollisionQueryParams())))
 { TestTrue(TEXT("支路继承接缝坡高"),FMath::IsNearlyEqual(SlopeHit.ImpactPoint.Z,200.0,0.1)); }
 Road->RoadSpline->ClearSplinePoints(false);
 for(const FVector& P : {FVector(0,0,0),FVector(2000,0,200),FVector(4000,2000,400)})
 { Road->RoadSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false); }
 Road->RoadSpline->UpdateSpline();
 Road->RightJunctionLength=250;
 Road->RebuildRoad();
 TestTrue(TEXT("平滑弯道支路保留坡度且不误报折叠"),Road->GenerationDiagnostic.IsEmpty());
 Road->RoadSpline->ClearSplinePoints(false);
 for(const FVector& P : {FVector(-3000,-3000,0),FVector(3000,-3000,0),FVector(3000,3000,0),FVector(-3000,3000,0)})
 { Road->RoadSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false); }
 Road->RoadSpline->SetClosedLoop(true,true);
 Road->RightJunctionDistance=3000; Road->RightJunctionRadius=150;
 Road->RebuildRoad();
 TestTrue(TEXT("闭环支路能够生成"),Road->GenerationDiagnostic.IsEmpty());
 if(!Road->GenerationDiagnostic.IsEmpty()) { AddError(Road->GenerationDiagnostic); }
 Road->bLeftJunction=true; Road->bLeftCurb=false; Road->bEnableCollision=false;
 Road->RebuildRoad();
 TestEqual(TEXT("禁用碰撞清理路口凸体"),Road->RoadMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),0);
 const FProcMeshSection* Curb=Road->RoadMesh->GetProcMeshSection(1);
 TestTrue(TEXT("关闭左路缘石也关闭支路路缘石"),!Curb || Curb->ProcVertexBuffer.IsEmpty());
 Road->LeftJunctionDistance=std::numeric_limits<float>::quiet_NaN();
 Road->RebuildRoad();
 TestFalse(TEXT("无效路口参数有诊断"),Road->GenerationDiagnostic.IsEmpty());
 TestEqual(TEXT("无效配置不残留部分道路"),Road->RoadMesh->GetNumSections(),0);
 return true;
}
#endif
