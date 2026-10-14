#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "LxARPG/LxSource/World/StoneTerrace/LxStoneTerraceActor.h"
#include "LxARPG/LxSource/World/StoneFloor/LxStoneFloorActor.h"
#include "LxARPG/LxSource/World/TimberPanel/LxTimberPanelActor.h"
#include "LxARPG/LxSource/World/TileRoof/LxTileRoofActor.h"
#include "LxARPG/LxSource/World/BrickWall/LxBrickWallActor.h"
#include "LxARPG/LxSource/World/StonePillar/LxStonePillarActor.h"
#include "LxARPG/LxSource/World/CargoPile/LxCargoPileActor.h"
#include "LxARPG/LxSource/World/Seawall/LxSeawallActor.h"
#include "UObject/UnrealType.h"

/** 连续编辑无需松开即可更新包围范围，并在保存入口恢复完整砖块和碰撞。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedInteractivePreviewTest,"LxARPG.World.GeneratedMesh.ContinuousPreview",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxGeneratedInteractivePreviewTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 const TArray<UClass*> Classes={ALxStoneFloorActor::StaticClass(),ALxTimberPanelActor::StaticClass(),
  ALxTileRoofActor::StaticClass(),ALxBrickWallActor::StaticClass(),ALxStonePillarActor::StaticClass(),ALxCargoPileActor::StaticClass()};
 for(UClass* Class:Classes)
 {
  AActor* Actor=World->SpawnActor<AActor>(Class);
  auto* Render=Actor->FindComponentByClass<UStaticMeshComponent>();
  FStructProperty* Handle=FindFProperty<FStructProperty>(Class,TEXT("SizeHandle"));
  if(!Handle) { Handle=FindFProperty<FStructProperty>(Class,TEXT("LengthHandle")); }
  if(!Handle) { Handle=FindFProperty<FStructProperty>(Class,TEXT("HeightHandle")); }
  if(!TestNotNull(TEXT("尺寸手柄"),Handle) || !TestNotNull(TEXT("预览组件"),Render)) { continue; }
  FVector* Size=Handle->ContainerPtrToValuePtr<FVector>(Actor);
  for(int32 Frame=0;Frame<3;++Frame)
  {
   UStaticMesh* Previous=Render->GetStaticMesh();
   if(Class==ALxStonePillarActor::StaticClass()) { Size->Z+=100; } else { Size->X+=100; }
   Actor->PostEditMove(false);
   // 连续输入没有等待空闲，节流窗口到达时必须生成当前形态。
   FTSTicker::GetCoreTicker().Tick(.06f);
   TestTrue(Class->GetName()+TEXT("拖动中使用轻量预览"),LxGeneratedMeshBuild::IsPreview(Actor));
   TestNotNull(Class->GetName()+TEXT("拖动中模型仍可见"),Render->GetStaticMesh().Get());
   TestTrue(Class->GetName()+TEXT("连续输入已生成新形态"),Previous!=Render->GetStaticMesh());
   if(Render->GetStaticMesh()) { TestFalse(TEXT("预览不构建Nanite"),Render->GetStaticMesh()->HasValidNaniteData()); }
  }
  Actor->PostEditMove(true);
  TestTrue(TEXT("松开后仍保持轻量预览"),LxGeneratedMeshBuild::IsPreview(Actor));
 }
 auto* Terrace=World->SpawnActor<ALxStoneTerraceActor>();
 Terrace->PostEditMove(false);Terrace->Size=FVector2D(10000,6000);
 FTSTicker::GetCoreTicker().Tick(.06f);
 const auto* Preview=Terrace->TerraceMesh->GetProcMeshSection(0);
 TestTrue(TEXT("大台地预览只保留连续顶面"),Preview && Preview->ProcIndexBuffer.Num()<100);
 TestTrue(TEXT("预览已经到达新边界"),Preview && Preview->SectionLocalBox.Max.X>4999);
 TestTrue(TEXT("预览期间不烹饪碰撞"),Terrace->TerraceMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 TestEqual(TEXT("预览不进行静态转换"),Terrace->OptimizedMesh->GeometryConversionCount,0);
 const double PreviewStart=FPlatformTime::Seconds();
 for(int32 Frame=0;Frame<30;++Frame)
 {
  Terrace->Size.X+=10;Terrace->PostEditMove(false);
  FTSTicker::GetCoreTicker().Tick(.06f);
 }
 AddInfo(FString::Printf(TEXT("100米级台地连续30次形态预览耗时 %.2f 秒"),FPlatformTime::Seconds()-PreviewStart));
 // 保存前立即修改最终参数，验证队列尚未运行时也不会漏掉最后一次编辑。
 Terrace->Size=FVector2D(2200,1200);Terrace->PostEditMove(false);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestFalse(TEXT("保存入口退出简化状态"),LxGeneratedMeshBuild::IsPreview(Terrace));
 TestTrue(TEXT("完整石砖已恢复"),Terrace->TerraceMesh->GetProcMeshSection(0)->ProcIndexBuffer.Num()>100);
 TestTrue(TEXT("最终碰撞已恢复"),!Terrace->TerraceMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 TestTrue(TEXT("最终网格使用真实Nanite"),Terrace->OptimizedMesh->GetStaticMesh() && Terrace->OptimizedMesh->GetStaticMesh()->HasValidNaniteData());
 const int32 Updates=Terrace->OptimizedMesh->SourceUpdateCount;
 FTSTicker::GetCoreTicker().Tick(.06f);
 TestEqual(TEXT("保存后的旧刷新请求不会覆盖完整模型"),Terrace->OptimizedMesh->SourceUpdateCount,Updates);
 return true;
}

/** 编辑河堤端点时，同一预览周期更新道路、堤身及相邻台地开口。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxGeneratedConnectionPreviewTest,"LxARPG.World.GeneratedMesh.ConnectionPreview",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxGeneratedConnectionPreviewTest::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Terrace=World->SpawnActor<ALxStoneTerraceActor>();
 auto* Wall=World->SpawnActor<ALxSeawallActor>(FVector(1500,0,100),FRotator::ZeroRotator);
 Wall->RoadWidth=400;
 for(double X:{1100.,1000.,950.})
 {
  Wall->SetActorLocation(FVector(X,0,100));Wall->PostEditMove(false);
  FTSTicker::GetCoreTicker().Tick(.06f);
  TestEqual(TEXT("移动中立即显示连接入口"),Terrace->Connections.Num(),1);
  TestNotNull(TEXT("移动中保留堤身轮廓"),Wall->WallMesh->GetProcMeshSection(0));
  TestEqual(TEXT("预览省略逐块岩板"),Wall->GeneratedSlabCount,0);
 }
 Wall->SetActorLocation(FVector(1500,0,100));Wall->PostEditMove(false);
 FTSTicker::GetCoreTicker().Tick(.06f);
 TestTrue(TEXT("移出范围立即恢复台地包边"),Terrace->Connections.IsEmpty());
 return true;
}
#endif
