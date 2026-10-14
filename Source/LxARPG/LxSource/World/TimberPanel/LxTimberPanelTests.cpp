#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxTimberPanelActor.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/UnrealType.h"
#endif

/** 验证单网格单碰撞、完整双面几何、连续木纹及任意矩形边界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTimberGeometryTest,"LxARPG.World.TimberPanel.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxTimberGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxTimberPanelActor* Panel=World->SpawnActor<ALxTimberPanelActor>();
 if (!TestNotNull(TEXT("木板演员"),Panel)) { return false; }
 TestEqual(TEXT("整板只有一个网格"),Panel->GeneratedMeshCount,1);
 TestEqual(TEXT("默认板块自动均分"),Panel->GeneratedPlankCount,48);
 TestEqual(TEXT("两根接头横梁和四根包边"),Panel->GeneratedBeamCount,6);
 TInlineComponentArray<UBoxComponent*> Boxes(Panel);
 TInlineComponentArray<UStaticMeshComponent*> Meshes(Panel);
 TestEqual(TEXT("只有一个碰撞组件"),Boxes.Num(),1);
 TestEqual(TEXT("只有一个渲染组件"),Meshes.Num(),1);
 TestEqual(TEXT("渲染组件不参与碰撞"),Panel->PanelMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 TestEqual(TEXT("碰撞启用"),Panel->PanelCollision->GetCollisionEnabled(),ECollisionEnabled::QueryAndPhysics);
 Panel->SizeHandle=FVector(613,427,99); Panel->bCeiling=true; Panel->bAlongY=true;
 Panel->RebuildPanel();
 UStaticMesh* Mesh=Panel->PanelMesh->GetStaticMesh();
 if (!TestNotNull(TEXT("天花板静态网格"),Mesh)) { return false; }
 TestEqual(TEXT("手柄保持水平"),Panel->SizeHandle.Z,0.0);
 const FBox Bounds=Mesh->GetBoundingBox();
 TestTrue(TEXT("换向后仍为指定矩形"),Bounds.Min.Equals(FVector(0,0,-22),0.001) && Bounds.Max.Equals(FVector(613,427,0),0.001));
 TestTrue(TEXT("单盒覆盖整板和下凸横梁"),Panel->PanelCollision->GetUnscaledBoxExtent().Equals(Bounds.GetExtent(),0.001));
 TestEqual(TEXT("只有两个材质分区"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),2);
 TestEqual(TEXT("渲染网格不自带重复碰撞盒"),Mesh->GetBodySetup()->AggGeom.BoxElems.Num(),0);
 // 单块无边板：各面倒角应闭合，重复角点的木纹坐标和颜色必须完全相同。
 Panel->SizeHandle=FVector(40,40,0); Panel->bBorder=false; Panel->bAlongY=false;
 Panel->PlankLength=600; Panel->PlankWidth=80; Panel->RebuildPanel();
 Mesh=Panel->PanelMesh->GetStaticMesh();
 TestEqual(TEXT("无包边且无接头时不生成横梁"),Panel->GeneratedBeamCount,0);
 TestTrue(TEXT("无梁天花板碰撞不会额外向下扩大"),Mesh->GetBoundingBox().Min.Equals(FVector(0,0,-12),0.001));
#if WITH_EDITOR
 const FMeshDescription* D=Mesh->GetMeshDescription(0);
 if (!TestNotNull(TEXT("可保存的源几何"),D)) { return false; }
 FStaticMeshConstAttributes A(*D);
 TMap<FVector3f,FVector3f> WoodCoordinates;
 TMap<FVector3f,int32> Welded;
 TMap<uint64,int32> Edges;
 bool bContinuous=true, bNormals=true, bOutward=true, bUVs=true;
 int32 Top=0, Bottom=0;
 for (FTriangleID Triangle : D->Triangles().GetElementIDs())
 {
  if (D->GetTrianglePolygonGroup(Triangle).GetValue()!=0) { continue; }
  const auto Corners=D->GetTriangleVertexInstances(Triangle);
  FVector P[3]; int32 IDs[3];
  for (int32 I=0; I<3; ++I)
  {
   const FVertexInstanceID VI=Corners[I];
   const FVector3f Position=A.GetVertexPositions()[D->GetVertexInstanceVertex(VI)];
   P[I]=FVector(Position);
   const FVector2f UV=A.GetVertexInstanceUVs().Get(VI,0), UV1=A.GetVertexInstanceUVs().Get(VI,1);
   const FVector3f Wood(UV.X,UV.Y,UV1.X);
   if (const FVector3f* Prior=WoodCoordinates.Find(Position)) { bContinuous &= Prior->Equals(Wood,0.00001f); }
   else { WoodCoordinates.Add(Position,Wood); }
   const int32 NextID=Welded.Num();
   IDs[I]=Welded.FindOrAdd(Position,NextID);
  }
  const FVector N=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]).GetSafeNormal();
  bNormals &= FVector(A.GetVertexInstanceNormals()[Corners[0]]).Equals(N,0.0001);
  bOutward &= FVector::DotProduct(N,(P[0]+P[1]+P[2])/3-FVector(20,20,-6))>0;
  Top+=N.Z>0.99; Bottom+=N.Z<-0.99;
  const FVector2f UV0=A.GetVertexInstanceUVs().Get(Corners[0],2);
  const FVector2f UV1=A.GetVertexInstanceUVs().Get(Corners[1],2);
  const FVector2f UV2=A.GetVertexInstanceUVs().Get(Corners[2],2);
  bUVs &= FMath::Abs((UV1.X-UV0.X)*(UV2.Y-UV0.Y)-(UV1.Y-UV0.Y)*(UV2.X-UV0.X))>1.e-10;
  for (int32 I=0; I<3; ++I)
  {
   const uint64 Key=(static_cast<uint64>(FMath::Min(IDs[I],IDs[(I+1)%3]))<<32)|static_cast<uint32>(FMath::Max(IDs[I],IDs[(I+1)%3]));
   ++Edges.FindOrAdd(Key);
  }
 }
 bool bClosed=true; for (const auto& Edge : Edges) { bClosed &= Edge.Value==2; }
 TestTrue(TEXT("单木板包含顶底面"),Top>0 && Bottom>0);
 TestTrue(TEXT("单木板及倒角闭合"),bClosed);
 TestTrue(TEXT("法线与三角形绕序一致且朝外"),bNormals && bOutward);
 TestTrue(TEXT("同一木块转角的三维木纹连续"),bContinuous);
 TestTrue(TEXT("专用光照展开坐标没有退化面"),bUVs);
#endif
 Panel->bBorder=true; Panel->RebuildPanel();
 TestEqual(TEXT("重新启用四条包边"),Panel->GeneratedBeamCount,4);
 Panel->SizeHandle=FVector(10000,10000,0); Panel->PlankLength=50; Panel->PlankWidth=8;
 Panel->RebuildPanel();
 TestEqual(TEXT("超预算清空网格"),Panel->GeneratedMeshCount,0);
 TestNull(TEXT("超预算没有旧网格"),Panel->PanelMesh->GetStaticMesh());
 TestEqual(TEXT("失败时关闭旧碰撞"),Panel->PanelCollision->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 TestFalse(TEXT("超预算提供中文诊断"),Panel->GenerationDiagnostic.IsEmpty());
 return true;
}

/** 验证移动缓存、碰撞独立刷新、重新加载和编辑器合并更新。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxTimberEditingTest,"LxARPG.World.TimberPanel.Editing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxTimberEditingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxTimberPanelActor* Panel=World->SpawnActor<ALxTimberPanelActor>();
 UStaticMesh* Original=Panel->PanelMesh->GetStaticMesh();
 const int32 Builds=Panel->GeometryBuildCount;
 for (int32 I=0; I<20; ++I)
 {
  Panel->SetActorLocation(FVector(I*30,0,0)); Panel->OnConstruction(Panel->GetActorTransform());
 }
 TestEqual(TEXT("整体移动不重建"),Panel->GeometryBuildCount,Builds);
 TestTrue(TEXT("整体移动复用资源"),Panel->PanelMesh->GetStaticMesh()==Original);
 Panel->bEnableCollision=false; Panel->OnConstruction(Panel->GetActorTransform());
 TestEqual(TEXT("关闭碰撞不重建"),Panel->GeometryBuildCount,Builds);
 TestEqual(TEXT("关闭碰撞生效"),Panel->PanelCollision->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
#if WITH_EDITOR
 FProperty* Handle=FindFProperty<FProperty>(ALxTimberPanelActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxTimberPanelActor,SizeHandle));
 for (int32 I=0; I<10; ++I)
 {
  Panel->PreEditChange(nullptr); Panel->SizeHandle.X+=10; Panel->SizeHandle.Y+=5;
  FPropertyChangedEvent Event(Handle); Panel->PostEditChangeProperty(Event);
 }
 TestEqual(TEXT("连续手柄事件合并"),Panel->GeometryBuildCount,Builds);
 Panel->PostEditMove(true);
 TestEqual(TEXT("松手只重建一次"),Panel->GeometryBuildCount,Builds+1);
 TestTrue(TEXT("双轴尺寸均生效"),Panel->PanelMesh->GetStaticMesh()->GetBoundingBox().Max.Equals(FVector(700,450,0),0.001));
 Panel->SizeHandle=FVector(600,400,0); Panel->PostEditUndo();
 TestTrue(TEXT("撤销恢复几何"),Panel->PanelMesh->GetStaticMesh()->GetBoundingBox().Max.Equals(FVector(600,400,0),0.001));
#endif
 Panel->UnregisterAllComponents(); Panel->PanelMesh->SetStaticMesh(nullptr); Panel->RegisterAllComponents();
 TestNotNull(TEXT("重新注册恢复临时静态网格"),Panel->PanelMesh->GetStaticMesh().Get());
 Panel->SizeHandle.X=std::numeric_limits<double>::quiet_NaN(); Panel->RebuildPanel();
 TestNull(TEXT("无效输入不残留网格"),Panel->PanelMesh->GetStaticMesh());
 TestFalse(TEXT("无效输入有中文诊断"),Panel->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
