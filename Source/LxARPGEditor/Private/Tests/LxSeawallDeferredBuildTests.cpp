#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshCompiler.h"
#include "UObject/SavePackage.h"
#include "LxARPG/LxSource/World/Seawall/LxSeawallActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"

/** 检查连续编辑不构建静态网格，编辑期间简化几何并实时预览，保存与运行前只提交最终形态。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSeawallDeferredBuildTest,"LxARPG.World.Seawall.DeferredBuild",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxSeawallDeferredBuildTest::RunTest(const FString& Parameters)
{
 UPackage* Package=CreatePackage(*FString::Printf(TEXT("/Temp/LxSeawallDeferred_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,TEXT("海堤延迟构建验证"),Package);
 if (!TestNotNull(TEXT("独立测试世界"),World)) { return false; }
 World->SetFlags(RF_Public|RF_Standalone);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxSeawallActor* Wall=World->SpawnActor<ALxSeawallActor>();
 if (!TestNotNull(TEXT("海堤实例"),Wall)) { return false; }
 const int32 InitialRoadUpdates=Wall->OptimizedMesh->SourceUpdateCount;
 const int32 InitialWallUpdates=Wall->WallOptimizedMesh->SourceUpdateCount;
 TestNull(TEXT("新海堤使用程序化预览"),Wall->WallOptimizedMesh->GetStaticMesh());
 for (int32 I=0; I<20; ++I)
 {
  Wall->SetActorLocationAndRotation(FVector(I*25,0,0),FRotator(0,I*3,0));
  Wall->PostEditMove(false);
 }
 Wall->PostEditMove(true);
 TestEqual(TEXT("连续移动合并为一次轻量预览"),Wall->OptimizedMesh->SourceUpdateCount,InitialRoadUpdates+1);
 TestEqual(TEXT("堤身同步切换一次轻量预览"),Wall->WallOptimizedMesh->SourceUpdateCount,InitialWallUpdates+1);
 TestEqual(TEXT("编辑预览不生成逐块岩板"),Wall->GeneratedSlabCount,0);
 TestEqual(TEXT("编辑不转换静态网格"),Wall->WallOptimizedMesh->GeometryConversionCount,0);
 LxGeneratedMeshBuild::PrepareWorld(World);
 FStaticMeshCompilingManager::Get().FinishAllCompilation();
 UStaticMesh* SavedWall=Wall->WallOptimizedMesh->GetStaticMesh();
 UStaticMesh* SavedRoad=Wall->OptimizedMesh->GetStaticMesh();
 if (!TestNotNull(TEXT("运行前生成堤身静态网格"),SavedWall)) { return false; }
 TestTrue(TEXT("完整构建具有Nanite数据"),SavedWall->HasValidNaniteData());
 const int32 InitialConversions=Wall->WallOptimizedMesh->GeometryConversionCount;
 Wall->SetActorLocation(FVector(1500,0,0));
 Wall->PostEditMove(true);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("退出轻量预览只重新检查一次静态描述"),Wall->WallOptimizedMesh->GeometryConversionCount,InitialConversions+1);
 TestTrue(TEXT("仅改变位置复用静态缓存"),Wall->WallOptimizedMesh->GetStaticMesh()==SavedWall);
 const int32 BeforeShapeConversions=Wall->WallOptimizedMesh->GeometryConversionCount;
 const int32 BeforeShapeUpdates=Wall->WallOptimizedMesh->SourceUpdateCount;
 for (int32 I=0; I<20; ++I)
 {
  Wall->RoadSpline->SetLocationAtSplinePoint(1,FVector(2200+I*10,0,0),ESplineCoordinateSpace::Local,true);
  Wall->PostEditMove(false);
  FTSTicker::GetCoreTicker().Tick(.06f);
  TestEqual(TEXT("每次形态预览均省略岩板"),Wall->GeneratedSlabCount,0);
 }
 TestTrue(TEXT("未松开鼠标也能持续生成预览"),Wall->WallOptimizedMesh->SourceUpdateCount>BeforeShapeUpdates);
 Wall->PostEditMove(true);
 TestTrue(TEXT("预览已到达最新端点"),Wall->RoadMesh->GetProcMeshSection(0)->SectionLocalBox.Max.X>2389);
 TestEqual(TEXT("改变长度仍不转换静态网格"),Wall->WallOptimizedMesh->GeometryConversionCount,BeforeShapeConversions);
 TestTrue(TEXT("形态变化显示程序化预览"),Wall->WallMesh->IsVisible() && !Wall->WallOptimizedMesh->IsVisible());
 TestTrue(TEXT("预览保留旧静态缓存直至保存"),Wall->WallOptimizedMesh->GetStaticMesh()==SavedWall);
 const int32 BeforePropertyUpdates=Wall->WallOptimizedMesh->SourceUpdateCount;
 FProperty* Height=FindFProperty<FProperty>(ALxSeawallActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxSeawallActor,WallHeight));
 for (int32 I=0; I<10; ++I)
 {
  Wall->PreEditChange(Height);
  Wall->WallHeight+=5;
  FPropertyChangedEvent Event(Height,EPropertyChangeType::Interactive);
  Wall->PostEditChangeProperty(Event);
 }
 TestEqual(TEXT("一个节流周期内的数值输入合并刷新"),Wall->WallOptimizedMesh->SourceUpdateCount,BeforePropertyUpdates);
 // 模拟最后一次连续编辑尚未提交就立即保存。
 const FString Filename=FPaths::ProjectSavedDir()/TEXT("Automation/海堤延迟构建验证.umap");
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
 FSavePackageArgs Args;
 Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 TestTrue(TEXT("保存测试关卡"),UPackage::SavePackage(Package,World,*Filename,Args));
 FStaticMeshCompilingManager::Get().FinishAllCompilation();
 TestEqual(TEXT("保存提交最终形态一次"),Wall->WallOptimizedMesh->SourceUpdateCount,BeforePropertyUpdates+1);
 TestEqual(TEXT("保存只转换最终堤身一次"),Wall->WallOptimizedMesh->GeometryConversionCount,BeforeShapeConversions+1);
 TestTrue(TEXT("形态变化生成新静态网格"),Wall->WallOptimizedMesh->GetStaticMesh()!=SavedWall);
 TestTrue(TEXT("长度变化同时更新顶部道路"),Wall->OptimizedMesh->GetStaticMesh()!=SavedRoad);
 TestFalse(TEXT("保存完成清除待构建标记"),Wall->WallOptimizedMesh->bPendingMeshBuild);
 TestTrue(TEXT("保存后的碰撞仍有效"),!Wall->WallMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 const int32 SavedUpdates=Wall->WallOptimizedMesh->SourceUpdateCount;
 const int32 SavedConversions=Wall->WallOptimizedMesh->GeometryConversionCount;
 TestTrue(TEXT("重复保存测试关卡"),UPackage::SavePackage(Package,World,*Filename,Args));
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("重复保存运行不重复生成源几何"),Wall->WallOptimizedMesh->SourceUpdateCount,SavedUpdates);
 TestEqual(TEXT("重复保存运行不重复静态转换"),Wall->WallOptimizedMesh->GeometryConversionCount,SavedConversions);
 Wall->WallHeight+=10;
 FPropertyChangedEvent FinalEvent(Height,EPropertyChangeType::Interactive);
 Wall->PostEditChangeProperty(FinalEvent);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("运行前提交尚未完成的形态编辑"),Wall->WallOptimizedMesh->SourceUpdateCount,SavedUpdates+1);
 TestEqual(TEXT("运行前只转换改变的堤身一次"),Wall->WallOptimizedMesh->GeometryConversionCount,SavedConversions+1);
 return true;
}
#endif
