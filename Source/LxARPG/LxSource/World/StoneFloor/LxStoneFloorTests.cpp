#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "StaticMeshResources.h"
#include "LxStoneFloorActor.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

/** 检查正方形铺装、完整厚度、单网格单碰撞以及所有倒角闭合。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStoneFloorGeometryTest,"LxARPG.World.StoneFloor.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxStoneFloorGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxStoneFloorActor* Floor=World->SpawnActor<ALxStoneFloorActor>();
 if (!TestNotNull(TEXT("石地板实例"),Floor)) { return false; }
 TestEqual(TEXT("默认由16块两米大板铺成"),Floor->GeneratedTileCount,16);
 TInlineComponentArray<UStaticMeshComponent*> Meshes(Floor);
 TInlineComponentArray<UBoxComponent*> Boxes(Floor);
 TestEqual(TEXT("唯一网格组件"),Meshes.Num(),1); TestEqual(TEXT("唯一碰撞盒"),Boxes.Num(),1);
 Floor->SizeHandle=FVector(853,50,30); Floor->Thickness=37; Floor->RebuildFloor();
 UStaticMesh* Mesh=Floor->PanelMesh->GetStaticMesh();
 if (!TestNotNull(TEXT("生成合并静态网格"),Mesh)) { return false; }
 TestTrue(TEXT("手柄只使用边长轴"),Floor->SizeHandle.Equals(FVector(853,0,0)));
 TestTrue(TEXT("任意边长仍保持方形和完整厚度"),Mesh->GetBoundingBox().Min.Equals(FVector(0,0,-37),.001)
  && Mesh->GetBoundingBox().Max.Equals(FVector(853,853,0),.001));
 TestEqual(TEXT("整个地板只有一个材质分区"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),1);
 TestTrue(TEXT("碰撞覆盖板体外轮廓"),Floor->PanelCollision->GetUnscaledBoxExtent().Equals(FVector(426.5,426.5,18.5),.001));
 TestEqual(TEXT("渲染网格不产生额外碰撞"),Floor->PanelMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
#if WITH_EDITOR
 const FMeshDescription* D=Mesh->GetMeshDescription(0);
 if (!TestNotNull(TEXT("可保存的源几何"),D)) { return false; }
 FStaticMeshConstAttributes A(*D);
 TMap<float,FBox> StoneBounds;
 TMap<FVector3f,FVector3f> Coordinates;
 TMap<FVector3f,int32> Welded;
 TMap<uint64,int32> Edges;
 bool bContinuous=true,bNormals=true,bUVs=true;
 for (FTriangleID T : D->Triangles().GetElementIDs())
 {
  const auto C=D->GetTriangleVertexInstances(T);
  FVector P[3]; int32 IDs[3];
  for (int32 I=0; I<3; ++I)
  {
   const FVector3f Position=A.GetVertexPositions()[D->GetVertexInstanceVertex(C[I])]; P[I]=FVector(Position);
   const FVector2f UV=A.GetVertexInstanceUVs().Get(C[I],0),UV1=A.GetVertexInstanceUVs().Get(C[I],1);
   const FVector3f Coord(UV.X,UV.Y,UV1.X);
   if (const FVector3f* Old=Coordinates.Find(Position)) { bContinuous &= Old->Equals(Coord,1.e-6f); }
   else { Coordinates.Add(Position,Coord); }
   if (!StoneBounds.Contains(UV1.Y)) { StoneBounds.Add(UV1.Y,FBox(ForceInit)); }
   StoneBounds[UV1.Y]+=P[I];
   IDs[I]=Welded.FindOrAdd(Position,Welded.Num());
  }
  const FVector N=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]).GetSafeNormal();
  for (int32 I=0; I<3; ++I)
  {
   bNormals &= FVector::DotProduct(N,FVector(A.GetVertexInstanceNormals()[C[I]]))>.99;
   const uint64 Key=(static_cast<uint64>(FMath::Min(IDs[I],IDs[(I+1)%3]))<<32)|static_cast<uint32>(FMath::Max(IDs[I],IDs[(I+1)%3]));
   ++Edges.FindOrAdd(Key);
  }
  const FVector2f U=A.GetVertexInstanceUVs().Get(C[1],2)-A.GetVertexInstanceUVs().Get(C[0],2);
  const FVector2f V=A.GetVertexInstanceUVs().Get(C[2],2)-A.GetVertexInstanceUVs().Get(C[0],2);
  bUVs &= FMath::Abs(U.X*V.Y-U.Y*V.X)>1.e-10;
 }
 bool bClosed=true,bSquare=true;
 for (const auto& Edge : Edges) { bClosed &= Edge.Value==2; }
 for (const auto& Stone : StoneBounds)
 { bSquare &= FMath::IsNearlyEqual(Stone.Value.GetSize().X,Stone.Value.GetSize().Y,.001); }
 TestTrue(TEXT("每块石板包括边缘板都是正方形"),bSquare);
 TestTrue(TEXT("所有石板与衬底闭合"),bClosed);
 TestTrue(TEXT("石纹在面与倒角之间连续"),bContinuous);
 TestTrue(TEXT("法线朝向与三角形一致"),bNormals);
 TestTrue(TEXT("光照展开没有退化三角形"),bUVs);
#endif
 Floor->SizeHandle.X=400; Floor->TileSize=600; Floor->RebuildFloor();
 TestEqual(TEXT("单块方石板不生成多余衬底"),Floor->GeneratedTriangleCount,44);
 TestEqual(TEXT("单块方石板数量"),Floor->GeneratedTileCount,1);
 return true;
}

/** 检查缓存、厚度变化、组件恢复以及预算和无效参数保护。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxStoneFloorEditingTest,"LxARPG.World.StoneFloor.Editing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxStoneFloorEditingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxStoneFloorActor* Floor=World->SpawnActor<ALxStoneFloorActor>();
 const int32 Builds=Floor->GeometryBuildCount;
 for (int32 I=0; I<20; ++I) { Floor->SetActorLocation(FVector(I*30,0,0)); Floor->OnConstruction(Floor->GetActorTransform()); }
 TestEqual(TEXT("整体移动不重建"),Floor->GeometryBuildCount,Builds);
 Floor->bEnableCollision=false; Floor->OnConstruction(Floor->GetActorTransform());
 TestEqual(TEXT("碰撞开关不重建"),Floor->GeometryBuildCount,Builds);
 TestEqual(TEXT("碰撞关闭生效"),Floor->PanelCollision->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Floor->Thickness=50; Floor->RebuildFloor();
 TestTrue(TEXT("厚度向下增长且顶面不动"),Floor->PanelMesh->GetStaticMesh()->GetBoundingBox().Min.Z==-50
  && Floor->PanelMesh->GetStaticMesh()->GetBoundingBox().Max.Z==0);
 Floor->UnregisterAllComponents(); Floor->PanelMesh->SetStaticMesh(nullptr); Floor->RegisterAllComponents();
 TestNotNull(TEXT("重新注册恢复临时网格"),Floor->PanelMesh->GetStaticMesh().Get());
 Floor->bEnableCollision=true; Floor->SizeHandle.X=20000; Floor->TileSize=50; Floor->RebuildFloor();
 TestNull(TEXT("超出预算不保留旧网格"),Floor->PanelMesh->GetStaticMesh());
 TestEqual(TEXT("超出预算关闭旧碰撞"),Floor->PanelCollision->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Floor->SizeHandle.X=std::numeric_limits<double>::quiet_NaN(); Floor->RebuildFloor();
 TestNull(TEXT("无效数值不生成网格"),Floor->PanelMesh->GetStaticMesh());
 TestFalse(TEXT("无效数值给出中文提示"),Floor->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
