#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "LxSplineRoadActor.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"

namespace LxRoadConnectionTests
{
 /** 创建测试道路，位置和方向均通过真实演员变换设置。 */
 ALxSplineRoadActor* Road(UWorld* World,FVector Position, float Yaw, float Width=600)
 {
  ALxSplineRoadActor* R=World->SpawnActor<ALxSplineRoadActor>(Position,FRotator(0,Yaw,0));
  R->OptimizedMesh->bEnableNanite=false;
  R->RoadWidth=Width;
  R->RebuildRoad();
  return R;
 }
 /** 获取指定局部端点截面的世界坐标路面顶点。 */
 TArray<FVector> Seam(ALxSplineRoadActor* R, FVector Center, FVector Axis)
 {
  TArray<FVector> Result;
  if(const FProcMeshSection* S=R->RoadMesh->GetProcMeshSection(0))
  {
   for(const FProcMeshVertex& V : S->ProcVertexBuffer)
   {
    const FVector P=R->RoadMesh->GetComponentTransform().TransformPosition(FVector(V.Position));
    if(V.Normal.Z>0.5 && FMath::Abs(FVector::DotProduct(P-Center,Axis))<0.02)
    { Result.AddUnique(P); }
   }
  }
  return Result;
 }
 /** 检查单个道路在世界位置是否具有可行走顶面。 */
 bool Hit(ALxSplineRoadActor* R,FVector Position)
 {
  FHitResult Result;
  return R->RoadMesh->LineTraceComponent(Result,Position+FVector(0,0,500),Position-FVector(0,0,500),FCollisionQueryParams())
   && FMath::Abs(Result.ImpactPoint.Z-Position.Z)<0.2;
 }
}

/** 检查端点吸附、不同路宽、双向参数、断开恢复及原路径不变。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadEndpointConnectionTest,"LxARPG.World.HarborRoad.AutoEndpoints",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadEndpointConnectionTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Road(World,FVector::ZeroVector,0,600);
 auto* B=Road(World,FVector(2100,0,0),0,300);
 TestEqual(TEXT("两边各显示一条连接"),A->RoadConnections.Num(),1);
 TestEqual(TEXT("接入侧显示连接"),B->RoadConnections.Num(),1);
 if(A->RoadConnections.IsEmpty() || B->RoadConnections.IsEmpty()) { return false; }
 TestTrue(TEXT("双方指向对方对象"),A->RoadConnections[0].OtherRoad==B && B->RoadConnections[0].OtherRoad==A);
 TestTrue(TEXT("未修改原始中心线"),A->RoadSpline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::Local).Equals(FVector(2000,0,0)));
 const FVector Center(2050,0,0);
 for(auto* R : {A,B})
 {
  TestTrue(TEXT("连接无几何错误"),R->GenerationDiagnostic.IsEmpty());
 }
 TestTrue(TEXT("接缝前后都有碰撞"),Hit(A,FVector(2049,0,0)) && Hit(A,FVector(2051,0,0)) && Hit(B,FVector(2101,0,0)));
 TestTrue(TEXT("接入道路端面保持原位置与宽度"),Seam(B,FVector(2100,0,0),FVector::ForwardVector).ContainsByPredicate([](FVector P){ return P.Equals(FVector(2100,150,0),0.05); }));
 B->SetRoadConnectionParameters(0,400,100,0);
 TestEqual(TEXT("从接入侧改宽同步主侧"),A->RoadConnections[0].Width,400.f);
 TestTrue(TEXT("参数修改刷新另一侧几何"),Seam(A,Center,FVector::ForwardVector).ContainsByPredicate([&](FVector P){ return P.Equals(Center+FVector(0,200,0),0.05); }));
 B->SetActorLocation(FVector(2500,0,0)); B->RebuildRoad();
 TestTrue(TEXT("超出范围双向断开"),A->RoadConnections.IsEmpty() && B->RoadConnections.IsEmpty());
 TestTrue(TEXT("断开恢复原端点"),FMath::IsNearlyEqual(A->RoadMesh->GetProcMeshSection(0)->SectionLocalBox.Max.X,2000.,0.01));
 B->SetActorLocation(FVector(2100,0,100)); B->RebuildRoad();
 TestTrue(TEXT("高差阻止误接"),A->RoadConnections.IsEmpty());
 B->SetActorLocation(FVector(2100,0,0)); B->RebuildRoad();
 B->bAutoConnect=false; B->RebuildRoad();
 TestTrue(TEXT("关闭任一侧会断开"),A->RoadConnections.IsEmpty() && B->RoadConnections.IsEmpty());
 return true;
}

/** 检查丁字开口、圆角碰撞、不同宽度、每侧数量和删除恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadTConnectionTest,"LxARPG.World.HarborRoad.AutoJunctions",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadTConnectionTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Main=Road(World,FVector::ZeroVector,0,600);
 auto* Branch=Road(World,FVector(1000,-450,0),-90,300);
 TestEqual(TEXT("主路显示丁字连接"),Main->RoadConnections.Num(),1);
 TestEqual(TEXT("支路显示丁字连接"),Branch->RoadConnections.Num(),1);
 if(Main->RoadConnections.IsEmpty() || Branch->RoadConnections.IsEmpty()) { return false; }
 TestTrue(TEXT("连接类型标记为丁字路口"),Main->RoadConnections[0].bJunction);
 TestTrue(TEXT("主路开口生成成功"),Main->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("支路吸附生成成功"),Branch->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("开口去除了挡路路缘石"),Hit(Main,FVector(1000,-310,0)));
 TestTrue(TEXT("圆角和出口间路面连续"),Hit(Main,FVector(1000,-449,0)) && Hit(Branch,FVector(1000,-452,0)));
 Branch->SetRoadConnectionParameters(0,400,100,250);
 TestEqual(TEXT("从支路调整圆角同步主路"),Main->RoadConnections[0].Radius,100.f);
 TestEqual(TEXT("连接长度同步主路"),Main->RoadConnections[0].Length,250.f);
 const FVector Center(1000,-450,0);
 for(auto* R : {Main,Branch})
 {
  TestTrue(TEXT("修改连接参数不移动支路端面"),Seam(R,Center,FVector::RightVector).ContainsByPredicate([&](FVector P){ return P.Equals(Center+FVector(150,0,0),0.05); }));
 }
 Main->SetRoadConnectionParameters(0,350,0,200);
 TestEqual(TEXT("从主路改宽同步支路"),Branch->RoadConnections[0].Width,350.f);
 TestTrue(TEXT("直角路口可生成"),Main->GenerationDiagnostic.IsEmpty());
 auto* Extra=Road(World,FVector(500,-490,0),-90,100);
 TestEqual(TEXT("同一侧允许多个独立开口"),Extra->RoadConnections.Num(),1);
 TestEqual(TEXT("主路维护同侧两条记录"),Main->RoadConnections.Num(),2);
 Extra->bAutoConnect=false; Extra->RebuildRoad();
 auto* Opposite=Road(World,FVector(1000,450,0),90,300);
 TestEqual(TEXT("主路左右各一个连接"),Main->RoadConnections.Num(),2);
 TestEqual(TEXT("另一侧支路也能连接"),Opposite->RoadConnections.Num(),1);
 Branch->Destroy();
 TestEqual(TEXT("删除支路移除连接"),Main->RoadConnections.Num(),1);
 TestFalse(TEXT("删除后恢复主路边界"),Hit(Main,FVector(1000,-350,0)));
 TestFalse(TEXT("自动连接不启用旧手动开关"),Main->bLeftJunction || Main->bRightJunction);
 return true;
}

/** 验证旋转曲线路口、详情面板参数同步及连续连接的稳定性。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadConnectionEditingTest,"LxARPG.World.HarborRoad.AutoConnectionEditing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadConnectionEditingTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Main=Road(World,FVector(3000,4000,200),35,600);
 Main->RoadSpline->SetTangentAtSplinePoint(0,FVector(2000,400,0),ESplineCoordinateSpace::Local,false);
 Main->RoadSpline->SetTangentAtSplinePoint(1,FVector(2000,-400,0),ESplineCoordinateSpace::Local,true);
 const float Distance=Main->RoadSpline->GetSplineLength()*0.5f;
 const FVector P=Main->RoadSpline->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);
 const FVector T=Main->RoadSpline->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World).GetSafeNormal2D();
 const FVector N(T.Y,-T.X,0);
 auto* Branch=Road(World,P+N*450,N.Rotation().Yaw,300);
 TestEqual(TEXT("旋转后的曲线主路形成连接"),Main->RoadConnections.Num(),1);
 if(Main->RoadConnections.IsEmpty() || Branch->RoadConnections.IsEmpty()) { return false; }
 TestTrue(TEXT("曲线开口没有反折"),Main->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("旋转后的支路无错误"),Branch->GenerationDiagnostic.IsEmpty());
#if WITH_EDITOR
 // 使用与详情面板相同的成员属性和数组下标，验证并非只有蓝图接口能够同步。
 Main->RoadConnections[0].Width=280;
 Main->RoadConnections[0].Radius=80;
 FProperty* WidthProperty=FindFProperty<FProperty>(FLxRoadConnectionSettings::StaticStruct(),TEXT("Width"));
 FProperty* ConnectionsProperty=FindFProperty<FProperty>(ALxSplineRoadActor::StaticClass(),TEXT("RoadConnections"));
 FPropertyChangedEvent Event(WidthProperty,EPropertyChangeType::ValueSet);
 Event.MemberProperty=ConnectionsProperty;
 TArray<TMap<FString,int32>> Indices; Indices.Add({{TEXT("RoadConnections"),0}});
 Event.SetArrayIndexPerObject(Indices);
 Main->PostEditChangeProperty(Event);
 TestEqual(TEXT("详情面板改宽同步对方"),Branch->RoadConnections[0].Width,280.f);
 TestEqual(TEXT("详情面板圆角同步对方"),Branch->RoadConnections[0].Radius,80.f);

#endif
 if(!TestNotNull(*FString::Printf(TEXT("调整后的曲线路口仍存在：%s"),*Main->GenerationDiagnostic),Main->RoadMesh->GetProcMeshSection(0))) { return false; }
 const int32 VertexCount=Main->RoadMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num();
 Main->RebuildRoad(); Branch->RebuildRoad();
 TestEqual(TEXT("重复求解不积累顶点"),Main->RoadMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),VertexCount);
 // 轻量编辑阶段省略烹饪，保存或运行入口必须恢复实际路口碰撞。
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestTrue(TEXT("运行前旋转路口碰撞贯通"),Hit(Main,P+N*301));
 // 回到直线后同时连接起点与中段，检查端点延伸不会移动中段路口接缝。
 Main->RoadSpline->SetTangentAtSplinePoint(0,FVector(2000,0,0),ESplineCoordinateSpace::Local,false);
 Main->RoadSpline->SetTangentAtSplinePoint(1,FVector(2000,0,0),ESplineCoordinateSpace::Local,true);
 const FTransform Transform=Main->GetActorTransform();
 Branch->SetActorLocation(Transform.TransformPosition(FVector(1000,-450,0)));
 auto* Previous=Road(World,Transform.TransformPosition(FVector(-2100,0,0)),35,400);
 Main->RebuildRoad();
 TestEqual(TEXT("同一道路支持端点与侧面同时连接"),Main->RoadConnections.Num(),2);
 TestTrue(TEXT("连续连接主路正常生成"),Main->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("连接前一路正常生成"),Previous->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("连接支路正常生成"),Branch->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("连接后原始样条起点仍不变"),Main->RoadSpline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::Local).IsNearlyZero());
 Previous->SetActorScale3D(FVector(2,1,1)); Previous->RebuildRoad();
 TestTrue(TEXT("非单位缩放提供说明"),!Previous->ConnectionDiagnostic.IsEmpty());
 TestTrue(TEXT("非单位缩放断开连接"),Previous->RoadConnections.IsEmpty());
 Main->bLeftJunction=true; Main->RebuildRoad();
 TestTrue(TEXT("旧路口仍可生成"),Main->GenerationDiagnostic.IsEmpty());
 Main->ClearLegacyJunctions();
 TestFalse(TEXT("清理旧路口标记"),Main->bLeftJunction);
 TestEqual(TEXT("清理后恢复自动丁字连接"),Main->RoadConnections.Num(),1);
 return true;
}
/** 验证大小路端点过渡连续相切，同时不移动两条原道路。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadSmoothEndpointsTest,"LxARPG.World.HarborRoad.SmoothEndpoints",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadSmoothEndpointsTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Road(World,FVector::ZeroVector,0,600);
 auto* B=Road(World,FVector(2200,0,0),0,300);
 TestTrue(TEXT("大小路平滑连接生成成功"),A->GenerationDiagnostic.IsEmpty());
 const FProcMeshSection* Surface=A->RoadMesh->GetProcMeshSection(0);
 if(!TestNotNull(TEXT("平滑连接路面存在"),Surface)) { return false; }
 TArray<FVector> Edge;
 for(const FProcMeshVertex& V : Surface->ProcVertexBuffer)
 {
  const FVector P(V.Position);
  if(V.Normal.Z>0.9 && P.X>=2000-0.001 && P.X<=2200+0.001 && P.Y<0) { Edge.AddUnique(P); }
 }
 Edge.Sort([](FVector P,FVector Q){ return P.X<Q.X; });
 TestTrue(TEXT("过渡包含足够曲线采样"),Edge.Num()>20);
 if(Edge.Num()<3) { return false; }
 const FVector Start=(Edge[1]-Edge[0]).GetSafeNormal2D(), End=(Edge.Last()-Edge[Edge.Num()-2]).GetSafeNormal2D();
 TestTrue(TEXT("入口方向与大路相切"),FVector::DotProduct(Start,FVector::ForwardVector)>0.9999);
 TestTrue(TEXT("出口方向与小路相切"),FVector::DotProduct(End,FVector::ForwardVector)>0.9999);
 TestTrue(TEXT("大路端面未移动"),Edge[0].Equals(FVector(2000,-300,0),0.01));
 TestTrue(TEXT("小路端面未移动"),Edge.Last().Equals(FVector(2200,-150,0),0.01));
 for(double X : {2001.,2050.,2100.,2150.,2199.}) { TestTrue(TEXT("平滑连接碰撞连续"),Hit(A,FVector(X,0,0))); }
 for(int32 Slot=1; Slot<=2; ++Slot)
 {
  const FProcMeshSection* Curb=A->RoadMesh->GetProcMeshSection(Slot);
  if(!TestNotNull(TEXT("连接路缘石存在"),Curb)) { return false; }
  for(int32 I=0; I+2<Curb->ProcIndexBuffer.Num(); I+=3)
  {
   const auto& P=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I]];
   const auto& Q=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I+1]];
   const auto& R=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I+2]];
   if(P.Position.X>2000 && P.Position.X<2200 && P.Normal.Z>0.9)
   { TestTrue(TEXT("过渡路缘石顶面无反折"),FVector::CrossProduct(FVector(R.Position-P.Position),FVector(Q.Position-P.Position)).Z>0); }
  }
 }
 B->SetRoadConnectionParameters(0,420,150,0);
 TestTrue(TEXT("自定义中间宽度仍平滑生成"),A->GenerationDiagnostic.IsEmpty());
 B->SetRoadConnectionParameters(0,0,150,0);
 B->SetActorLocation(FVector(2100,0,0)); B->RebuildRoad();
 bool bShortCurbValid=true;
 for(int32 Slot=1; Slot<=2; ++Slot)
 {
  const FProcMeshSection* Curb=A->RoadMesh->GetProcMeshSection(Slot);
  if(!TestNotNull(TEXT("短距离连接路缘石存在"),Curb)) { return false; }
  for(int32 I=0; I+2<Curb->ProcIndexBuffer.Num(); I+=3)
  {
   const auto& P=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I]];
   const auto& Q=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I+1]];
   const auto& R=Curb->ProcVertexBuffer[Curb->ProcIndexBuffer[I+2]];
   if(P.Position.X>2000 && P.Position.X<2100 && P.Normal.Z>0.9)
   { bShortCurbValid &= FVector::CrossProduct(FVector(R.Position-P.Position),FVector(Q.Position-P.Position)).Z>0; }
  }
 }
 TestTrue(TEXT("短距离变宽不能导致路缘石顶面反折"),bShortCurbValid);
 return true;
}

/** 验证大小路单边对齐时，该侧路面及路缘石保持笔直，覆盖两侧、变宽变窄及整体旋转。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadAlignedEdgeTest,"LxARPG.World.HarborRoad.AlignedEndpointEdge",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadAlignedEdgeTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 for(float Yaw : {0.f,37.f})
 for(float WidthA : {300.f,600.f})
 for(float Sign : {-1.f,1.f})
 for(float Gap : {50.f,100.f})
 {
  UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
  ON_SCOPE_EXIT { World->DestroyWorld(false); };
  const float WidthB=900-WidthA, HalfA=WidthA*0.5f;
  auto* A=Road(World,FVector(100,250,30),Yaw,WidthA);
  const FTransform Transform=A->GetActorTransform();
  const FVector StartB=Transform.TransformPosition(FVector(2000+Gap,Sign*(WidthA-WidthB)*0.5f,0));
  auto* B=Road(World,StartB,Yaw,WidthB);
  if(!TestEqual(TEXT("单边对齐道路建立连接"),A->RoadConnections.Num(),1)) { return false; }
  TestTrue(TEXT("单边对齐连接生成成功"),A->GenerationDiagnostic.IsEmpty());
  const FProcMeshSection* Surface=A->RoadMesh->GetProcMeshSection(0);
  if(!TestNotNull(TEXT("单边对齐路面存在"),Surface)) { return false; }
  int32 AlignedVertices=0;
  double MaxDeviation=0;
  for(const FProcMeshVertex& V : Surface->ProcVertexBuffer)
  {
   const FVector P(V.Position);
   if(P.X>=1999.99 && P.X<=2000+Gap+0.01 && P.Y*Sign>HalfA-50 && V.Normal.Z>0.9)
   { ++AlignedVertices; MaxDeviation=FMath::Max(MaxDeviation,FMath::Abs(P.Y*Sign-HalfA)); }
  }
  TestTrue(TEXT("保留直边两端"),AlignedVertices>=2);
  TestTrue(TEXT("已对齐路边没有横向波浪"),MaxDeviation<0.01);
  const FProcMeshSection* Curb=A->RoadMesh->GetProcMeshSection(Sign<0 ? 1 : 2);
  if(!TestNotNull(TEXT("直边路缘石存在"),Curb)) { return false; }
  bool bStraightCurb=true;
  for(const FProcMeshVertex& V : Curb->ProcVertexBuffer)
  {
   const FVector P(V.Position);
   if(P.X<1999.99 || P.X>2000+Gap+0.01) { continue; }
   const double Offset=P.Y*Sign-HalfA;
   bool bOnProfile=false;
   for(float Expected : {0.f,A->CurbBevel,A->CurbWidth-A->CurbBevel,A->CurbWidth})
   { bOnProfile |= FMath::IsNearlyEqual(Offset,static_cast<double>(Expected),0.01); }
   bStraightCurb &= bOnProfile;
  }
  TestTrue(TEXT("已对齐路缘石内外缘均保持直线和原厚度"),bStraightCurb);
  TestTrue(TEXT("大小路原始位置不变"),A->GetActorTransform().Equals(Transform) && B->GetActorLocation().Equals(StartB,0.001));
 }
 return true;
}

/** 验证端部封口的默认值、独立开关、圆角、铺面、碰撞、接缝及闭环兼容性。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadEndCurbsTest,"LxARPG.World.HarborRoad.EndCurbs",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadEndCurbsTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* R=Road(World,FVector::ZeroVector,0);
 TestFalse(TEXT("起点封口默认关闭"),R->bStartCurb);
 TestFalse(TEXT("终点封口默认关闭"),R->bEndCurb);
 TestNull(TEXT("默认没有端部路缘石网格"),R->RoadMesh->GetProcMeshSection(3));
 /** 查询指定位置的封口路缘石碰撞顶面。 */
 const auto CurbHit=[&](FVector P)
 {
  FHitResult HitResult;
  return R->RoadMesh->LineTraceComponent(HitResult,P+FVector(0,0,500),P-FVector(0,0,500),FCollisionQueryParams())
   && FMath::IsNearlyEqual(HitResult.ImpactPoint.Z,P.Z+R->CurbHeight,0.1);
 };
 R->bStartCurb=true; R->StartCurbRadius=80; R->RebuildRoad();
 const FProcMeshSection* Cap=R->RoadMesh->GetProcMeshSection(3);
 if(!TestNotNull(TEXT("起点封口生成"),Cap)) { return false; }
 TestTrue(TEXT("起点圆角半径生效"),FMath::IsNearlyEqual(Cap->SectionLocalBox.Min.X,-104.,0.01));
 TestTrue(TEXT("独立起点开关不封住终点"),Cap->SectionLocalBox.Max.X<0.01);
 TestTrue(TEXT("起点路面延伸部分可行走"),Hit(R,FVector(-40,0,0)));
 TestTrue(TEXT("起点路缘石具有碰撞"),CurbHit(FVector(-90,0,0)));
 for(int32 Slot : {1,2})
 {
  const FProcMeshSection* Side=R->RoadMesh->GetProcMeshSection(Slot);
  if(!TestNotNull(TEXT("保留侧面路缘石"),Side)) { return false; }
  int32 Shared=0;
  for(const FProcMeshVertex& V : Side->ProcVertexBuffer)
  {
   if(FMath::Abs(V.Position.X)>0.001) { continue; }
   if(Cap->ProcVertexBuffer.ContainsByPredicate([&](const FProcMeshVertex& C){ return C.Position.Equals(V.Position,0.001); })) { ++Shared; }
  }
  TestTrue(TEXT("圆角与两侧路缘石完整对齐"),Shared>=6);
 }
 R->bEndCurb=true; R->EndCurbRadius=150; R->RebuildRoad();
 TestTrue(TEXT("两端可以同时封口"),CurbHit(FVector(-90,0,0)) && CurbHit(FVector(2160,0,0)));
 R->EndCurbRadius=0; R->RebuildRoad();
 TestTrue(TEXT("零半径生成直角端部"),CurbHit(FVector(2012,0,0)));
 R->EndCurbRadius=10000; R->RebuildRoad();
 Cap=R->RoadMesh->GetProcMeshSection(3);
 if(!TestNotNull(TEXT("大半径仍正常生成"),Cap)) { return false; }
 TestTrue(TEXT("圆角限制为半路宽"),FMath::IsNearlyEqual(Cap->SectionLocalBox.Max.X,2324.,0.01));
 bool bValidTriangles=true;
 for(int32 I=0; I+2<Cap->ProcIndexBuffer.Num(); I+=3)
 {
  const auto& A=Cap->ProcVertexBuffer[Cap->ProcIndexBuffer[I]];
  const auto& B=Cap->ProcVertexBuffer[Cap->ProcIndexBuffer[I+1]];
  const auto& C=Cap->ProcVertexBuffer[Cap->ProcIndexBuffer[I+2]];
  const FVector Normal=FVector::CrossProduct(FVector(C.Position-A.Position),FVector(B.Position-A.Position));
  bValidTriangles &= Normal.SizeSquared()>1.e-10 && FVector::DotProduct(Normal,FVector(A.Normal))>0;
 }
 TestTrue(TEXT("圆角没有退化或反折三角形"),bValidTriangles);
 R->StartCurbRadius=80; R->EndCurbRadius=100;
 R->RoadSpline->SetLocationAtSplinePoint(1,FVector(2000,0,400),ESplineCoordinateSpace::Local,true); R->RebuildRoad();
 TestTrue(TEXT("坡道封口保留路面高度"),Hit(R,FVector(2050,0,410)));
 // 横向封口截面保持水平，其高度来自延伸圆角的内缘，而非石材外侧的平面投影。
 TestTrue(TEXT("坡道封口路缘石保留高度"),CurbHit(FVector(2112,0,420)));
 R->bStartCurb=false; R->bEndCurb=false; R->RebuildRoad();
 Cap=R->RoadMesh->GetProcMeshSection(3);
 TestTrue(TEXT("关闭后清理旧封口"),!Cap || Cap->ProcVertexBuffer.IsEmpty());
 R->bStartCurb=true; R->bEndCurb=true;
 R->RoadSpline->ClearSplinePoints(false);
 for(const FVector& P : {FVector(-3000,-3000,0),FVector(3000,-3000,0),FVector(3000,3000,0),FVector(-3000,3000,0)})
 { R->RoadSpline->AddSplinePoint(P,ESplineCoordinateSpace::Local,false); }
 R->RoadSpline->SetClosedLoop(true,true); R->RebuildRoad();
 TestTrue(TEXT("闭环道路仍正常生成"),R->GenerationDiagnostic.IsEmpty());
 Cap=R->RoadMesh->GetProcMeshSection(3);
 TestTrue(TEXT("闭环样条忽略端部封口"),!Cap || Cap->ProcVertexBuffer.IsEmpty());
 return true;
}

/** 验证端部封口不阻碍端点或丁字连接，断开后自动恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadConnectedEndCurbsTest,"LxARPG.World.HarborRoad.ConnectedEndCurbs",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadConnectedEndCurbsTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* A=Road(World,FVector::ZeroVector,0);
 A->bEndCurb=true; A->RebuildRoad();
 auto* B=Road(World,FVector(2100,0,0),0,300);
 B->bStartCurb=true; B->RebuildRoad();
 /** 检查封口网格是否为空，兼容材质槽保留但顶点已清理的情况。 */
 const auto HasCap=[](ALxSplineRoadActor* R)
 { const FProcMeshSection* S=R->RoadMesh->GetProcMeshSection(3); return S && !S->ProcVertexBuffer.IsEmpty(); };
 TestEqual(TEXT("开启封口仍正常端点连接"),A->RoadConnections.Num(),1);
 TestFalse(TEXT("主路连接端不封口"),HasCap(A));
 TestFalse(TEXT("接入道路连接端不封口"),HasCap(B));
 TestTrue(TEXT("连接通道无路缘石阻挡"),Hit(A,FVector(2050,0,0)));
 B->SetActorLocation(FVector(2600,0,0)); B->RebuildRoad();
 TestTrue(TEXT("断开后双方封口自动恢复"),HasCap(A) && HasCap(B));
 B->Destroy();
 auto* Branch=Road(World,FVector(1000,-450,0),-90,300);
 Branch->bStartCurb=true; Branch->RebuildRoad();
 TestEqual(TEXT("开启封口仍正常丁字连接"),Branch->RoadConnections.Num(),1);
 TestFalse(TEXT("丁字连接的支路起点不封口"),HasCap(Branch));
 TestTrue(TEXT("中段连接不取消主路尽头封口"),HasCap(A));
 Branch->SetActorLocation(FVector(1000,-1000,0)); Branch->RebuildRoad();
 TestTrue(TEXT("丁字支路离开后恢复封口"),HasCap(Branch));
 return true;
}

/** 验证独立拖动内部网格不能脱离样条，但演员整体变换与控制点编辑仍可用。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadComponentLockTest,"LxARPG.World.HarborRoad.ComponentTransformLock",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadComponentLockTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* R=Road(World,FVector(150,250,30),25);
 for(USceneComponent* Component : {static_cast<USceneComponent*>(R->RoadMesh.Get()),static_cast<USceneComponent*>(R->OptimizedMesh.Get())})
 {
  Component->SetRelativeLocation(FVector(500,100,50));
  TestTrue(TEXT("内部组件不能独立移动"),Component->GetRelativeTransform().Equals(FTransform::Identity));
  Component->SetRelativeRotation(FRotator(20,40,10));
  TestTrue(TEXT("内部组件不能独立旋转"),Component->GetRelativeTransform().Equals(FTransform::Identity));
  Component->SetRelativeScale3D(FVector(2,0.5,3));
  TestTrue(TEXT("内部组件不能独立缩放"),Component->GetRelativeTransform().Equals(FTransform::Identity));
  Component->SetWorldLocation(FVector(10000,10000,10000));
  TestTrue(TEXT("世界空间拖动同样被锁定"),Component->GetRelativeTransform().Equals(FTransform::Identity));
 }
 const FTransform Transform(FRotator(0,50,0),FVector(300,700,50),FVector(1.2));
 R->SetActorTransform(Transform);
 TestTrue(TEXT("道路演员可以整体移动旋转缩放"),R->GetActorTransform().Equals(Transform));
 TestTrue(TEXT("程序网格继续跟随样条"),R->RoadMesh->GetComponentTransform().Equals(R->RoadSpline->GetComponentTransform()));
 TestTrue(TEXT("优化缓存继续跟随程序网格"),R->OptimizedMesh->GetComponentTransform().Equals(R->RoadMesh->GetComponentTransform()));
 R->RoadSpline->SetLocationAtSplinePoint(1,FVector(2500,0,100),ESplineCoordinateSpace::Local,true);
 R->RebuildRoad();
 TestTrue(TEXT("仍可通过样条调整道路"),R->GenerationDiagnostic.IsEmpty());
 TestTrue(TEXT("样条末端修改生效"),FMath::IsNearlyEqual(R->RoadMesh->GetProcMeshSection(0)->SectionLocalBox.Max.X,2500.,0.01));
 return true;
}

/** 验证多角度连接保持支路全部顶点、支持同侧多个开口并只拒绝重叠。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRoadObliqueConnectionsTest,"LxARPG.World.HarborRoad.ObliqueMultipleConnections",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRoadObliqueConnectionsTest::RunTest(const FString& Parameters)
{
 using namespace LxRoadConnectionTests;
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Main=Road(World,FVector::ZeroVector,0,600);
 Main->RoadSpline->SetLocationAtSplinePoint(1,FVector(8000,0,0),ESplineCoordinateSpace::Local,true);
 Main->AutoConnectDistance=350; Main->RebuildRoad();
 TArray<ALxSplineRoadActor*> Branches;
 for(float Angle : {-35.f,-60.f,-90.f,-120.f})
 {
  const FVector Start(1500+Branches.Num()*1600,-620,0);
  auto* B=Road(World,Start,Angle,300);
  B->bAutoConnect=false; B->RebuildRoad();
  TArray<FVector> Original;
  for(const FProcMeshVertex& V : B->RoadMesh->GetProcMeshSection(0)->ProcVertexBuffer) { Original.Add(FVector(V.Position)); }
  const FVector Tangent=B->RoadSpline->GetTangentAtSplinePoint(0,ESplineCoordinateSpace::World);
  B->AutoConnectDistance=350; B->bAutoConnect=true; B->RebuildRoad();
  TestEqual(TEXT("斜向道路连接成功"),B->RoadConnections.Num(),1);
  TestTrue(TEXT("主路多路口生成成功"),Main->GenerationDiagnostic.IsEmpty());
  TestTrue(TEXT("接入道路路径未改变"),B->RoadSpline->GetTangentAtSplinePoint(0,ESplineCoordinateSpace::World).Equals(Tangent,0.001));
  const FProcMeshSection* Section=B->RoadMesh->GetProcMeshSection(0);
  if(!TestNotNull(TEXT("接入道路仍存在"),Section)) { return false; }
  TestEqual(TEXT("接入道路顶点数量不变"),Section->ProcVertexBuffer.Num(),Original.Num());
  if(Section->ProcVertexBuffer.Num()==Original.Num())
  {
   for(int32 I=0; I<Original.Num(); ++I)
   { if(!FVector(Section->ProcVertexBuffer[I].Position).Equals(Original[I],0.001)) { AddError(TEXT("自动连接改变了原道路顶点")); break; } }
  }
  const FVector Into=-B->GetActorForwardVector();
  TestTrue(TEXT("接入端前方连接片具有碰撞"),Hit(Main,Start+Into*5));
  Branches.Add(B);
 }
 TestEqual(TEXT("同侧四个路口同时保留"),Main->RoadConnections.Num(),4);
 auto* Overlap=Road(World,FVector(Branches[2]->GetActorLocation()+FVector(5,-20,0)),-90,300);
 Overlap->AutoConnectDistance=350; Overlap->RebuildRoad();
 TestTrue(TEXT("重叠开口不生成"),Overlap->RoadConnections.IsEmpty());
 TestTrue(TEXT("重叠原因可见"),!Overlap->ConnectionDiagnostic.IsEmpty());
 TestEqual(TEXT("重叠请求不移除已有路口"),Main->RoadConnections.Num(),4);
 Branches[1]->bAutoConnect=false; Branches[1]->RebuildRoad();
 TestEqual(TEXT("断开其中一路只移除该连接"),Main->RoadConnections.Num(),3);
 return true;
}
#endif
