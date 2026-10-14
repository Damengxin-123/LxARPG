#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Components/SplineComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/World/Ocean/LxOceanActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"

/** 验证海岸连续编辑只生成轻量预览，保存和运行提交最新完整网格并取消旧请求。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanDeferredBuildTest,"LxARPG.World.Ocean.DeferredBuild",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxOceanDeferredBuildTest::RunTest(const FString& Parameters)
{
 UPackage* Package=CreatePackage(*FString::Printf(TEXT("/Temp/LxOceanDeferred_%s"),*FGuid::NewGuid().ToString(EGuidFormats::Digits)));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,TEXT("海洋延迟生成验证"),Package);
 if(!TestNotNull(TEXT("独立海洋测试世界"),World)) return false;
 World->SetFlags(RF_Public|RF_Standalone);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
 if(!TestNotNull(TEXT("海洋实例"),Ocean)) return false;
 Ocean->bAutoDetectLandscape=false;
 Ocean->CoastMin={-5000,-5000}; Ocean->CoastMax={5000,5000};
 Ocean->WaveFadeDistance=5000; Ocean->OceanExtend=10000;
 Ocean->GridSpacing=100; Ocean->MaxGridSegments=128;
 Ocean->OceanMaterial=UMaterial::GetDefaultMaterial(MD_Surface);
 TestTrue(TEXT("凹形海岸预览成功"),Ocean->SetClosedCoastline({{-50,-50},{50,-50},{50,-10},{-10,-10},{-10,50},{-50,50}}));
 TestTrue(TEXT("编辑器立即显示轻量预览"),Ocean->bUsingEditorPreview && Ocean->bPendingFullMeshBuild);
 TestEqual(TEXT("编辑放置与设置海岸均不生成完整网格"),Ocean->FullMeshBuildCount,0);
 FProcMeshSection* Preview=Ocean->OceanMesh->GetProcMeshSection(0);
 if(!TestNotNull(TEXT("预览网格"),Preview)) return false;
 const int32 PreviewVertices=Preview->ProcVertexBuffer.Num();
 TestTrue(TEXT("预览遵守每轴二十四分段预算"),PreviewVertices<=25*25+6);
 /** 求实际上传海面面积，验证凹形孔洞在低密度预览中仍然精确。 */
 const auto MeshArea=[](const FProcMeshSection* Section)
 {
  double Area=0;
  for(int32 I=0;I<Section->ProcIndexBuffer.Num();I+=3)
  {
   const FVector A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]].Position;
   const FVector B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]].Position;
   const FVector C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]].Position;
   Area+=FMath::Abs(FVector::CrossProduct(B-A,C-A).Z)*0.5;
  }
  return Area;
 };
 TestTrue(TEXT("预览海岸孔洞面积精确"),FMath::Abs(MeshArea(Preview)-(30000.0*30000.0-6400.0*10000.0))<100);
 TestFalse(TEXT("凹形陆地无海面"),Ocean->HasOceanAtLocation({-2000,0,0}));
 TestTrue(TEXT("凹形海湾仍有海面"),Ocean->HasOceanAtLocation({0,0,0}));
 UMaterialInterface* PreviewMaterial=Ocean->OceanMesh->GetMaterial(0);
 const int32 InitialPreviews=Ocean->PreviewMeshBuildCount;
 for(int32 I=0;I<20;++I)
 {
  Ocean->CoastlineSpline->SetLocationAtSplinePoint(1,FVector(5000+I*100,-5000,0),ESplineCoordinateSpace::Local);
  Ocean->OnConstruction(Ocean->GetActorTransform());
 }
 TestEqual(TEXT("同一刷新窗口内不重复生成预览"),Ocean->PreviewMeshBuildCount,InitialPreviews);
 FTSTicker::GetCoreTicker().Tick(0.06f);
 TestEqual(TEXT("连续拖动合并为一次预览"),Ocean->PreviewMeshBuildCount,InitialPreviews+1);
 TestEqual(TEXT("拖动期间不生成完整海面"),Ocean->FullMeshBuildCount,0);
 TestTrue(TEXT("拖动复用动态材质"),Ocean->OceanMesh->GetMaterial(0)==PreviewMaterial);
 TestTrue(TEXT("预览已使用最后一次控制点位置"),Ocean->IsInsideCoastline({6800,-4900,0}));
 Ocean->Tick(0.1f);
 TestEqual(TEXT("预览省略碰撞浪花采样"),Ocean->LastFoamSampleCount,0);
 TestEqual(TEXT("预览不提交碰撞浪花纹理"),Ocean->CompletedFoamCaptures,0);
 // 最后一次编辑尚未到达刷新时刻就直接保存，检查真正的保存回调。
 Ocean->CoastlineSpline->SetLocationAtSplinePoint(1,FVector(8000,-5000,0),ESplineCoordinateSpace::Local);
 Ocean->OnConstruction(Ocean->GetActorTransform());
 const FString Filename=FPaths::ProjectSavedDir()/TEXT("Automation/海洋延迟生成验证.umap");
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
 FSavePackageArgs Args;
 Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 TestTrue(TEXT("保存独立海洋关卡"),UPackage::SavePackage(Package,World,*Filename,Args));
 TestEqual(TEXT("保存只生成一次完整网格"),Ocean->FullMeshBuildCount,1);
 TestFalse(TEXT("保存退出轻量预览"),Ocean->bUsingEditorPreview);
 TestFalse(TEXT("保存清除待生成标记"),Ocean->bPendingFullMeshBuild);
 TestTrue(TEXT("保存提交尚未预览的最新海岸点"),Ocean->IsInsideCoastline({7900,-4900,0}));
 const FProcMeshSection* Complete=Ocean->OceanMesh->GetProcMeshSection(0);
 if(!TestNotNull(TEXT("已保存完整网格"),Complete)) return false;
 TestTrue(TEXT("完整网格恢复配置的细分密度"),Complete->ProcVertexBuffer.Num()>PreviewVertices*10);
 AddInfo(FString::Printf(TEXT("海面顶点：编辑预览=%d，保存完整网格=%d"),PreviewVertices,Complete->ProcVertexBuffer.Num()));
 FTSTicker::GetCoreTicker().Tick(0.06f);
 TestFalse(TEXT("保存前排队的预览不能覆盖完整网格"),Ocean->bUsingEditorPreview);
 TestEqual(TEXT("已取消的预览不重复生成"),Ocean->PreviewMeshBuildCount,InitialPreviews+1);
 TestTrue(TEXT("再次保存海洋关卡"),UPackage::SavePackage(Package,World,*Filename,Args));
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("未修改时重复保存和运行入口均复用完整网格"),Ocean->FullMeshBuildCount,1);
 FProperty* Spacing=FindFProperty<FProperty>(ALxOceanActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxOceanActor,GridSpacing));
 Ocean->PreEditChange(Spacing);
 Ocean->GridSpacing=150;
 FPropertyChangedEvent Changed(Spacing,EPropertyChangeType::Interactive);
 Ocean->PostEditChangeProperty(Changed);
 TestTrue(TEXT("属性连续输入标记完整网格待生成"),Ocean->bPendingFullMeshBuild);
 LxGeneratedMeshBuild::PrepareWorld(World);
 TestEqual(TEXT("运行入口提交未刷新的属性输入"),Ocean->FullMeshBuildCount,2);
 FTSTicker::GetCoreTicker().Tick(0.06f);
 TestFalse(TEXT("运行准备后旧预览不能覆盖完整网格"),Ocean->bUsingEditorPreview);
 // 保存后的新请求有独立序号，旧请求无法提前刷新新海岸。
 Ocean->RequestOceanPreview();
 Ocean->FinalizeOcean();
 Ocean->RequestOceanPreview();
 FTSTicker::GetCoreTicker().Tick(0.06f);
 TestTrue(TEXT("完成保存后再次编辑仍能恢复预览"),Ocean->bUsingEditorPreview);
 return true;
}
#endif
