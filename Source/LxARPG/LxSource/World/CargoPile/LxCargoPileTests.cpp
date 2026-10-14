#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxCargoPileActor.h"
#if WITH_EDITOR
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#endif

/** 检查实际几何范围、承重链、高度限制及货物之间的相交。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCargoPackingTest,"LxARPG.World.CargoPile.Packing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCargoPackingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 ALxCargoPileActor* Pile=World->SpawnActor<ALxCargoPileActor>();
 if (!TestNotNull(TEXT("货物堆实例"),Pile)) { return false; }
 TSet<ELxCargoKind> Seen; int32 Stacked=0;
 for (int32 Seed=0; Seed<8; ++Seed)
 {
  Pile->RandomSeed=Seed; Pile->Disorder=Seed%2?1:0; Pile->SizeHandle=FVector(950,730,99); Pile->StackHeight=320; Pile->RebuildPile();
  UStaticMesh* Mesh=Pile->PileMesh->GetStaticMesh();
  if (!TestNotNull(TEXT("货物静态网格"),Mesh)) { return false; }
  const FBox B=Mesh->GetBoundingBox();
  TestTrue(TEXT("真实顶点严格在拖动矩形和高度内"),B.Min.X>=-.001 && B.Min.Y>=-.001 && B.Min.Z>=-.001
   && B.Max.X<=950.001 && B.Max.Y<=730.001 && B.Max.Z<=320.001);
  TestEqual(TEXT("尺寸手柄处于水平面"),Pile->SizeHandle.Z,0.0);
  TestEqual(TEXT("每件货物仅一个简单碰撞"),Mesh->GetBodySetup()->AggGeom.GetElementCount(),Pile->Placements.Num());
  TInlineComponentArray<UStaticMeshComponent*> Components(Pile); TestEqual(TEXT("整堆仅一个网格组件"),Components.Num(),1);
  for (int32 I=0; I<Pile->Placements.Num(); ++I)
  {
   const auto& P=Pile->Placements[I]; Seen.Add(P.Kind);
   if (Pile->Disorder==0) { TestEqual(TEXT("零混乱度朝向完全对齐"),P.Yaw,0.f); }
   if (P.SupportIndex==INDEX_NONE) { TestEqual(TEXT("底层落地"),P.Position.Z,0.0); }
   else
   {
    ++Stacked; TestTrue(TEXT("支撑索引指向已有货物"),P.SupportIndex<I);
    const auto& Base=Pile->Placements[P.SupportIndex];
    TestTrue(TEXT("上层底面与下层顶面接触"),FMath::IsNearlyEqual(P.Position.Z,Base.Position.Z+Base.Size.Z,.001));
    const FVector Local=FRotator(0,Base.Yaw,0).UnrotateVector(P.Position-Base.Position);
    TestTrue(TEXT("重心落在承重顶面的安全矩形内"),FMath::Abs(Local.X)<=Base.SupportSize.X/2 && FMath::Abs(Local.Y)<=Base.SupportSize.Y/2);
   }
   for (int32 J=0; J<I; ++J)
   {
    const auto& Q=Pile->Placements[J];
    const bool OverlapZ=P.Position.Z<Q.Position.Z+Q.Size.Z-.001 && Q.Position.Z<P.Position.Z+P.Size.Z-.001;
    const bool OverlapXY=FMath::Abs(P.Position.X-Q.Position.X)<(P.Footprint.X+Q.Footprint.X)/2-.001
     && FMath::Abs(P.Position.Y-Q.Position.Y)<(P.Footprint.Y+Q.Footprint.Y)/2-.001;
    TestFalse(TEXT("货物不互相穿插"),OverlapZ && OverlapXY);
   }
  }
#if WITH_EDITOR
  const FMeshDescription* D=Mesh->GetMeshDescription(0); FStaticMeshConstAttributes A(*D); bool bValid=true;
  for (FTriangleID T : D->Triangles().GetElementIDs())
  {
   const auto C=D->GetTriangleVertexInstances(T); FVector V[3];
   for (int32 I=0; I<3; ++I) { V[I]=FVector(A.GetVertexPositions()[D->GetVertexInstanceVertex(C[I])]); }
   const FVector Cross=FVector::CrossProduct(V[2]-V[0],V[1]-V[0]);
   bValid &= Cross.Size()>1.e-8 && FVector::DotProduct(Cross.GetSafeNormal(),FVector(A.GetVertexInstanceNormals()[C[0]]))>.1;
  }
  TestTrue(TEXT("无退化三角形，平滑法线朝向与几何一致"),bValid);
#endif
 }
 TestEqual(TEXT("随机分布能覆盖九种货物"),Seen.Num(),9);
 TestTrue(TEXT("生成了实际堆叠层"),Stacked>10);
 return true;
}

/** 验证种子复现、尺寸变化、移动缓存、空集和异常输入。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCargoEditingTest,"LxARPG.World.CargoPile.Editing",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCargoEditingTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { if (World) { World->DestroyWorld(false); } };
 ALxCargoPileActor* Pile=World->SpawnActor<ALxCargoPileActor>();
 const auto Original=Pile->Placements; const int32 Builds=Pile->GeometryBuildCount;
 Pile->SetActorLocation(FVector(500,200,80)); Pile->OnConstruction(Pile->GetActorTransform());
 TestEqual(TEXT("整体移动不重新随机排列"),Pile->GeometryBuildCount,Builds);
 Pile->RebuildPile(); TestEqual(TEXT("相同种子数量一致"),Pile->Placements.Num(),Original.Num());
 for (int32 I=0; I<Original.Num(); ++I)
 { TestTrue(TEXT("相同种子位置与种类完全一致"),Original[I].Position==Pile->Placements[I].Position && Original[I].Kind==Pile->Placements[I].Kind && Original[I].Yaw==Pile->Placements[I].Yaw); }
 Pile->RerollPile(); TestEqual(TEXT("换一批增加种子"),Pile->RandomSeed,1010);
 Pile->MaxCargoCount=3; Pile->RebuildPile(); TestTrue(TEXT("数量预算生效"),Pile->GeneratedCargoCount<=3);
 Pile->SizeHandle=FVector(60,60,0); Pile->StackHeight=30; Pile->CargoScale=.5; Pile->MaxCargoCount=128; Pile->RebuildPile();
 TestTrue(TEXT("最小范围可生成小货物"),Pile->GeneratedCargoCount>0 && Pile->GeneratedHeight<=30);
 Pile->Density=0; Pile->RebuildPile(); TestNull(TEXT("零密度清空网格"),Pile->PileMesh->GetStaticMesh());
 TestEqual(TEXT("零密度清空碰撞"),Pile->PileMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Pile->Density=1; Pile->bCrates=false; Pile->bBarrels=false; Pile->bSacks=false; Pile->bTimber=false; Pile->bCloth=false; Pile->RebuildPile();
 TestEqual(TEXT("全部种类关闭无货物"),Pile->Placements.Num(),0);
 Pile->SizeHandle.X=std::numeric_limits<double>::quiet_NaN(); Pile->RebuildPile(); TestFalse(TEXT("异常数值有中文提示"),Pile->GenerationDiagnostic.IsEmpty());
 return true;
}
#endif
