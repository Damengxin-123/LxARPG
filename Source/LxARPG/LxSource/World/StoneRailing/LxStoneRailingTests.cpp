#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxStoneRailingActor.h"

/** 覆盖风格、尺寸、密度、折线坡度、闭合和无效输入，验证实际几何而非仅检查参数。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStoneRailingGeometryTest,"LxARPG.World.StoneRailing.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxStoneRailingGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 ALxStoneRailingActor* Rail=World->SpawnActor<ALxStoneRailingActor>();
 if (!TestNotNull(TEXT("护栏实例"),Rail)) { return false; }
 for (ELxStoneRailingStyle Style : {ELxStoneRailingStyle::Harbour,ELxStoneRailingStyle::Floral,ELxStoneRailingStyle::Wave})
 {
  Rail->Style=Style; Rail->RebuildRailing();
  TestTrue(TEXT("预设生成真实倒角和雕花"),Rail->GeneratedTriangleCount>1000);
  bool bFinite=true,bOutward=true;
  FBox Bounds(ForceInit);
  for (int32 S=0; S<2; ++S)
  {
   const FProcMeshSection* Section=Rail->RailingMesh->GetProcMeshSection(S);
   if (!TestNotNull(TEXT("有效材质分区"),Section)) { return false; }
   for (const FProcMeshVertex& V : Section->ProcVertexBuffer) { bFinite&=!V.Position.ContainsNaN(); Bounds+=V.Position; }
   for (int32 I=0; I<Section->ProcIndexBuffer.Num(); I+=3)
   {
    const FProcMeshVertex& A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]];
    const FProcMeshVertex& B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]];
    const FProcMeshVertex& C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]];
    bOutward&=FVector::DotProduct(FVector::CrossProduct(C.Position-A.Position,B.Position-A.Position),A.Normal)>1.e-8;
   }
  }
  TestTrue(TEXT("顶点有效、无退化面且法线朝向一致"),bFinite && bOutward);
  TestTrue(TEXT("默认总高准确且底部对齐控制线"),FMath::IsNearlyEqual(Bounds.Max.Z,110.0,.01) && FMath::IsNearlyZero(Bounds.Min.Z,.01));
 }
 const int32 LowCount=Rail->GeneratedPostCount;
 Rail->Density=16; Rail->RebuildRailing();
 TestTrue(TEXT("提高密度增加立柱"),Rail->GeneratedPostCount>LowCount);
 const int32 Decorated=Rail->GeneratedTriangleCount;
 Rail->bOrnaments=false; Rail->RebuildRailing();
 TestTrue(TEXT("关闭雕花减少几何但保留倒角"),Rail->GeneratedTriangleCount<Decorated && Rail->GeneratedTriangleCount>500);
 const int32 Builds=Rail->GeometryBuildCount;
 Rail->OnConstruction(Rail->GetActorTransform());
 TestEqual(TEXT("未改变输入不重建"),Rail->GeometryBuildCount,Builds);
 Rail->Density=8;
 Rail->RailingSpline->SetLocationAtSplinePoint(2,FVector(400,400,80),ESplineCoordinateSpace::Local,true);
 Rail->Height=180; Rail->RebuildRailing();
 TestTrue(TEXT("折线与坡度有效"),Rail->GeneratedPostCount>0 && Rail->GenerationDiagnostic.IsEmpty());
 Rail->RailingSpline->SetSplinePointType(1,ESplinePointType::Curve,true);
 Rail->RebuildRailing();
 TestTrue(TEXT("曲线片段生成连续扶手"),Rail->GeneratedPostCount>0 && Rail->GenerationDiagnostic.IsEmpty());
 for (const FKConvexElem& Convex : Rail->RailingMesh->GetBodySetup()->AggGeom.ConvexElems)
 { TestTrue(TEXT("曲线端柱与扶手均有有效碰撞凸包"),Convex.GetChaosConvexMesh() && Convex.GetChaosConvexMesh()->NumVertices()>=4); }
 Rail->RailingSpline->SetSplinePointType(1,ESplinePointType::Linear,true);
 Rail->bClosedLoop=true; Rail->RebuildRailing();
 TestTrue(TEXT("闭合生成且控制线已闭合"),Rail->RailingSpline->IsClosedLoop() && Rail->GeneratedPostCount>0);
 Rail->bEnableCollision=false; Rail->RebuildRailing();
 TestEqual(TEXT("关闭碰撞即时生效"),Rail->RailingMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Rail->bClosedLoop=false;
 Rail->RailingSpline->SetLocationAtSplinePoint(1,FVector::ZeroVector,ESplineCoordinateSpace::Local,true);
 Rail->RebuildRailing();
 TestEqual(TEXT("无效路径清除旧护栏"),Rail->GeneratedTriangleCount,0);
 TestTrue(TEXT("给出中文错误提示"),!Rail->GenerationDiagnostic.IsEmpty());
 return true;
}
/** 验证拐角顶面不会翻折，柱头落在顶梁投影内，闭合接缝与急弯有明确边界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStoneRailingCornerTest,"LxARPG.World.StoneRailing.Corners",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxStoneRailingCornerTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 ALxStoneRailingActor* Rail=World->SpawnActor<ALxStoneRailingActor>();
 if (!TestNotNull(TEXT("拐角测试对象"),Rail)) { return false; }
 Rail->bOrnaments=false;
 Rail->Density=4;
 // 对真实生成的顶面检测朝上性，旧测试的法线与三角形一致无法发现整个面向下翻折。
 const auto CheckTop=[&]()
 {
  const FProcMeshSection* Mesh=Rail->RailingMesh->GetProcMeshSection(1);
  if (!TestNotNull(TEXT("拐角扶手网格"),Mesh)) { return; }
  int32 Reversed=0;
  for (int32 I=0; I<Mesh->ProcIndexBuffer.Num(); I+=3)
  {
   const FVector A=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I]].Position;
   const FVector B=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+1]].Position;
   const FVector C=Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+2]].Position;
   if (FMath::IsNearlyEqual(A.Z,double(Rail->Height),.001)
    && FMath::IsNearlyEqual(B.Z,A.Z,.001) && FMath::IsNearlyEqual(C.Z,A.Z,.001))
   { Reversed+=FVector::CrossProduct(C-A,B-A).Z<-.0001; }
  }
  TestEqual(TEXT("水平扶手顶面没有向下翻折"),Reversed,0);
 };
 // 柱头顶点必须属于两侧扶手或斜接区域，排除穿出内角的宽托臂。
 const auto CheckCapital=[&](const FVector& Corner,const FVector& Incoming,const FVector& Outgoing)
 {
  const FProcMeshSection* Mesh=Rail->RailingMesh->GetProcMeshSection(0);
  int32 Outside=0;
  for (const FProcMeshVertex& Vertex : Mesh->ProcVertexBuffer)
  {
   const FVector P=Vertex.Position-Corner;
   if (P.SizeSquared2D()>FMath::Square(Rail->Width*1.2) || P.Z<Rail->Height*.83) { continue; }
   const double InAlong=FVector::DotProduct(P,Incoming), OutAlong=FVector::DotProduct(P,Outgoing);
   const double InSide=FMath::Abs(FVector::CrossProduct(Incoming,P).Z);
   const double OutSide=FMath::Abs(FVector::CrossProduct(Outgoing,P).Z);
   const double Half=Rail->Width*.46+.1;
   const bool bInside=(InAlong<=0 && InSide<=Half) || (OutAlong>=0 && OutSide<=Half)
    || (InSide<=Half && OutSide<=Half);
   Outside+=!bInside;
  }
  TestEqual(TEXT("转角柱头不伸出两侧扶手"),Outside,0);
 };
 for (double Turn : {5.0,10.0,45.0,90.0,135.0,-90.0,-135.0})
 for (float Width : {40.0f,100.0f})
 {
  const FVector Corner(400,0,0),Incoming(1,0,0);
  const FVector Outgoing(FMath::Cos(FMath::DegreesToRadians(Turn)),FMath::Sin(FMath::DegreesToRadians(Turn)),0);
  Rail->Width=Width;
  Rail->RailingSpline->SetLocationAtSplinePoint(2,Corner+Outgoing*400,ESplineCoordinateSpace::Local,true);
  Rail->RebuildRailing();
  TestTrue(TEXT("常用转角成功生成"),Rail->GeneratedPostCount>0 && Rail->GenerationDiagnostic.IsEmpty());
  CheckTop(); CheckCapital(Corner,Incoming,Outgoing);
 }
 Rail->RailingSpline->ClearSplinePoints(false);
 for (const FVector& P : {FVector(0,0,0),FVector(400,0,0),FVector(400,400,0),FVector(0,400,0)})
 {
  Rail->RailingSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false);
  Rail->RailingSpline->SetSplinePointType(Rail->RailingSpline->GetNumberOfSplinePoints()-1,ESplinePointType::Linear,false);
 }
 Rail->bClosedLoop=true; Rail->RailingSpline->UpdateSpline(); Rail->RebuildRailing();
 TestEqual(TEXT("四边闭环首尾柱不重复"),Rail->GeneratedPostCount,8);
 CheckTop(); CheckCapital(FVector::ZeroVector,FVector(0,-1,0),FVector(1,0,0));
 return true;
}
#endif
