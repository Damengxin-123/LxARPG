#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/Ocean/LxOceanActor.h"
#include "LxARPG/LxSource/World/SplineRoad/LxSplineRoadActor.h"

/** 在当前关卡的内存副本中统计生成物，比较海洋碰撞浪花开启前后的主线程成本。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedWorldAudit,"LxARPG.Diagnostics.GeneratedWorldRuntime",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxGeneratedWorldAudit::RunTest(const FString& Parameters)
{
 UWorld* World=GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
 if (!TestNotNull(TEXT("待诊断的编辑器关卡"),World)) { return false; }
 int32 ActorCount=0, ComponentCount=0, TriangleCount=0, ConvexCount=0;
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  ++ActorCount;
  // 应用当前版本的道路碰撞算法后再采样；不保存这些诊断会话中的变更。
  if (ALxSplineRoadActor* Road=Cast<ALxSplineRoadActor>(*It)) { Road->RebuildRoad(); }
  TInlineComponentArray<UProceduralMeshComponent*> Meshes;
  It->GetComponents(Meshes);
  for (UProceduralMeshComponent* Mesh : Meshes)
  {
   ++ComponentCount;
   int32 Triangles=0;
   for (int32 Section=0; Section<Mesh->GetNumSections(); ++Section)
   {
    if (const FProcMeshSection* Data=Mesh->GetProcMeshSection(Section)) { Triangles+=Data->ProcIndexBuffer.Num()/3; }
   }
   const int32 Hulls=Mesh->GetBodySetup()->AggGeom.ConvexElems.Num();
   TriangleCount+=Triangles; ConvexCount+=Hulls;
   AddInfo(FString::Printf(TEXT("生成物 %s 类=%s 三角形=%d 凸碰撞=%d 槽=%d 阴影=%d"),
    *It->GetActorLabel(),*It->GetClass()->GetName(),Triangles,Hulls,Mesh->GetNumSections(),Mesh->CastShadow));
  }
 }
 AddInfo(FString::Printf(TEXT("关卡统计 演员=%d 程序网格=%d 三角形=%d 凸碰撞=%d"),ActorCount,ComponentCount,TriangleCount,ConvexCount));
 for (TActorIterator<ALxOceanActor> It(World); It; ++It)
 {
  ALxOceanActor* Ocean=*It;
  const bool bSavedFoam=Ocean->bEnableFoam;
  Ocean->RebuildOcean();
  for (bool bFoam : {false,true})
  {
   Ocean->bEnableFoam=bFoam;
   TArray<double> Samples;
   double Total=0;
   for (int32 Frame=0; Frame<180; ++Frame)
   {
    const double Start=FPlatformTime::Seconds();
    Ocean->Tick(1.f/60.f);
    const double Elapsed=(FPlatformTime::Seconds()-Start)*1000;
    Samples.Add(Elapsed); Total+=Elapsed;
   }
   Samples.Sort();
   AddInfo(FString::Printf(TEXT("海洋采样 %s 浪花=%d 帧数=180 平均毫秒=%.3f P95毫秒=%.3f 最大毫秒=%.3f"),
    *Ocean->GetActorLabel(),bFoam,Total/Samples.Num(),Samples[171],Samples.Last()));
  }
  Ocean->bEnableFoam=bSavedFoam;
 }
 return true;
}
#endif
