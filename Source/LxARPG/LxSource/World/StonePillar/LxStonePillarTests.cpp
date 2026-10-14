#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "StaticMeshResources.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxStonePillarActor.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "UObject/UnrealType.h"
#endif

/** 验证两种柱形、两端的全部开关组合、几何闭合和连续石纹。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPillarGeometryTest,"LxARPG.World.StonePillar.Geometry",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPillarGeometryTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxStonePillarActor* Pillar=World->SpawnActor<ALxStonePillarActor>();
 if (!TestNotNull(TEXT("柱子实例"),Pillar)) { return false; }
 TInlineComponentArray<UStaticMeshComponent*> Components(Pillar);
 TestEqual(TEXT("三块石头共用一个网格与物理组件"),Components.Num(),1);
 for (ELxPillarShape Shape : {ELxPillarShape::Round,ELxPillarShape::Square})
 for (int32 Mask=0; Mask<4; ++Mask)
 {
  Pillar->Shape=Shape; Pillar->bBase=(Mask&1)!=0; Pillar->bCapital=(Mask&2)!=0;
  Pillar->HeightHandle=FVector(3,4,360); Pillar->RebuildPillar();
  UStaticMesh* Mesh=Pillar->PillarMesh->GetStaticMesh();
  if (!TestNotNull(TEXT("生成柱子静态网格"),Mesh)) { return false; }
  const int32 Count=1+int32(Pillar->bBase)+int32(Pillar->bCapital);
  TestEqual(TEXT("独立整石数量匹配开关"),Pillar->GeneratedStoneCount,Count);
  TestEqual(TEXT("网格数保持为一"),Pillar->GeneratedMeshCount,1);
  TestTrue(TEXT("手柄限定竖直方向"),Pillar->HeightHandle.Equals(FVector(0,0,360)));
  TestTrue(TEXT("各组合底部归零且总高不变"),FMath::IsNearlyZero(Mesh->GetBoundingBox().Min.Z) && FMath::IsNearlyEqual(Mesh->GetBoundingBox().Max.Z,360.0));
  TestEqual(TEXT("一个物理体内的简单形状逐块生成"),Mesh->GetBodySetup()->AggGeom.GetElementCount(),Count);
  TestEqual(TEXT("实际渲染分区数匹配石块"),Mesh->GetRenderData()->LODResources[0].Sections.Num(),Count);
  TArray<int32> Expected; if (Pillar->bBase) { Expected.Add(0); } Expected.Add(1); if (Pillar->bCapital) { Expected.Add(2); }
  for (int32 I=0; I<Count; ++I)
  { TestEqual(TEXT("关闭端部不会使柱身用错材质槽"),Mesh->GetRenderData()->LODResources[0].Sections[I].MaterialIndex,Expected[I]); }
#if WITH_EDITOR
  const FMeshDescription* D=Mesh->GetMeshDescription(0);
  if (!TestNotNull(TEXT("保留源几何供资产保存"),D)) { return false; }
  FStaticMeshConstAttributes A(*D);
  bool bClosed=true,bContinuous=true,bOutward=true,bUVs=true;
  for (int32 Slot : Expected)
  {
   TMap<FVector3f,int32> Welded; TMap<uint64,int32> Edges; TMap<FVector3f,FVector3f> StoneUV;
   for (FTriangleID T : D->Triangles().GetElementIDs())
   {
    if (D->GetTrianglePolygonGroup(T).GetValue()!=Slot) { continue; }
    const auto C=D->GetTriangleVertexInstances(T);
    FVector P[3]; int32 IDs[3];
    for (int32 I=0; I<3; ++I)
    {
     const FVector3f Pos=A.GetVertexPositions()[D->GetVertexInstanceVertex(C[I])]; P[I]=FVector(Pos);
     const FVector2f UV=A.GetVertexInstanceUVs().Get(C[I],0),UV1=A.GetVertexInstanceUVs().Get(C[I],1);
     const FVector3f Coord(UV.X,UV.Y,UV1.X);
     if (const FVector3f* Old=StoneUV.Find(Pos)) { bContinuous &= Old->Equals(Coord,1.e-6f); } else { StoneUV.Add(Pos,Coord); }
     IDs[I]=Welded.FindOrAdd(Pos,Welded.Num());
    }
    const FVector Cross=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]);
    for (int32 I=0; I<3; ++I)
    {
     bOutward &= FVector::DotProduct(Cross,FVector(A.GetVertexInstanceNormals()[C[I]]))>1.e-7;
     const uint64 Key=(static_cast<uint64>(FMath::Min(IDs[I],IDs[(I+1)%3]))<<32)|static_cast<uint32>(FMath::Max(IDs[I],IDs[(I+1)%3]));
     ++Edges.FindOrAdd(Key);
    }
    const FVector2f U=A.GetVertexInstanceUVs().Get(C[1],3)-A.GetVertexInstanceUVs().Get(C[0],3);
    const FVector2f V=A.GetVertexInstanceUVs().Get(C[2],3)-A.GetVertexInstanceUVs().Get(C[0],3);
    bUVs &= FMath::Abs(U.X*V.Y-U.Y*V.X)>1.e-10;
   }
   for (const auto& Edge : Edges) { bClosed &= Edge.Value==2; }
  }
  TestTrue(TEXT("每块整石及倒角均为闭合表面"),bClosed);
  TestTrue(TEXT("转角与上下表面的石纹坐标连续"),bContinuous);
  TestTrue(TEXT("所有面有效且法线符合朝向"),bOutward);
  TestTrue(TEXT("光照展开没有退化三角形"),bUVs);
#endif
 }
 Pillar->HeightHandle.Z=40; Pillar->BaseHeight=160; Pillar->CapitalHeight=160; Pillar->RebuildPillar();
 TestTrue(TEXT("极短柱子仍保留正的完整柱身"),Pillar->GeneratedShaftHeight>=18);
 TestTrue(TEXT("端部压缩后仍匹配总高"),FMath::IsNearlyEqual(Pillar->PillarMesh->GetStaticMesh()->GetBoundingBox().Max.Z,40.0));
 return true;
}

/** 验证整体移动缓存、碰撞开关、无效输入以及重载组件后的网格恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxPillarEditingTest,"LxARPG.World.StonePillar.Editing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxPillarEditingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 if (!TestNotNull(TEXT("测试世界"),World)) { return false; }
 ALxStonePillarActor* Pillar=World->SpawnActor<ALxStonePillarActor>();
 UStaticMesh* Original=Pillar->PillarMesh->GetStaticMesh();
 const int32 Builds=Pillar->GeometryBuildCount;
 for (int32 I=0; I<20; ++I)
 { Pillar->SetActorLocation(FVector(I*30,0,0)); Pillar->OnConstruction(Pillar->GetActorTransform()); }
 TestEqual(TEXT("整体移动不重建"),Pillar->GeometryBuildCount,Builds);
 TestTrue(TEXT("移动复用同一个网格"),Pillar->PillarMesh->GetStaticMesh()==Original);
 Pillar->bEnableCollision=false; Pillar->OnConstruction(Pillar->GetActorTransform());
 TestEqual(TEXT("碰撞开关不重建几何"),Pillar->GeometryBuildCount,Builds);
 TestEqual(TEXT("碰撞关闭生效"),Pillar->PillarMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Pillar->bEnableCollision=true; Pillar->OnConstruction(Pillar->GetActorTransform());
 TestEqual(TEXT("碰撞恢复生效"),Pillar->PillarMesh->GetCollisionEnabled(),ECollisionEnabled::QueryAndPhysics);
 Pillar->UnregisterAllComponents(); Pillar->PillarMesh->SetStaticMesh(nullptr); Pillar->RegisterAllComponents();
 TestNotNull(TEXT("重载组件恢复网格"),Pillar->PillarMesh->GetStaticMesh().Get());
 Pillar->HeightHandle.Z=std::numeric_limits<double>::quiet_NaN(); Pillar->RebuildPillar();
 TestNull(TEXT("无效输入清空旧网格"),Pillar->PillarMesh->GetStaticMesh());
 TestEqual(TEXT("无效输入关闭碰撞"),Pillar->PillarMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 TestFalse(TEXT("无效输入有中文提示"),Pillar->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
