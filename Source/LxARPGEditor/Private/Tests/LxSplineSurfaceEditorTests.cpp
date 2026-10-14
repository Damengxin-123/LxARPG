#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/SplineComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Editor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/World/SplineSurface/LxSplineSurfaceActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"

namespace
{
 /** 按真实本地轮廓创建直线或光滑地面，不改用户关卡。 */
 void SetGroundOutline(ALxSplineSurfaceActor* Ground, const TArray<FVector>& Points, bool bCurve = false)
 {
  Ground->SurfaceSpline->ClearSplinePoints(false);
  for (const FVector& Point : Points) { Ground->SurfaceSpline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false); }
  for (int32 I = 0; I < Points.Num(); ++I)
  { Ground->SurfaceSpline->SetSplinePointType(I, bCurve ? ESplinePointType::Curve : ESplinePointType::Linear, false); }
  Ground->SurfaceSpline->SetClosedLoop(true, false);
  Ground->SurfaceSpline->UpdateSpline();
  Ground->ControlPointCount = Points.Num();
 }
 /** 直接从可见三角形计算平面面积，同时核对绕序与朝上法线。 */
 double GroundArea(ALxSplineSurfaceActor* Ground, FAutomationTestBase& Test)
 {
  double Area = 0;
  const FProcMeshSection* Section = Ground->SurfaceMesh->GetProcMeshSection(0);
  if (!Test.TestNotNull(TEXT("地面三角形分区"), Section)) { return Area; }
  for (int32 I = 0; I < Section->ProcIndexBuffer.Num(); I += 3)
  {
   const FProcMeshVertex& A = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]];
   const FVector B = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]].Position;
   const FVector C = Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]].Position;
   const FVector Cross = FVector::CrossProduct(C-A.Position,B-A.Position);
   Test.TestTrue(TEXT("顶面绕序、法线及纹理坐标有效"), Cross.Z > 0 && A.Normal.Z > .99 && !A.UV0.ContainsNaN());
   Area += Cross.Z * .5;
  }
  return Area;
 }
}

/** 回归可行走凹地面、米制尺寸、自交保护和移动缓存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSplineGroundGeometry,"LxARPG.World.SplineSurface.GeometryAndCollision",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSplineGroundGeometry::RunTest(const FString& Parameters)
{
 UWorld* World = UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Ground = World->SpawnActor<ALxSplineSurfaceActor>();
 TestTrue(TEXT("源几何使用紧凑二进制组件"), Ground->SurfaceMesh->IsA<ULxCompactProceduralMeshComponent>());
 TestNotNull(TEXT("默认绑定现有海港石板风格"), Ground->SurfaceMaterial.Get());
 TestEqual(TEXT("直线矩形不产生多余采样点"), Ground->GeneratedVertexCount,4);
 TestEqual(TEXT("凸地面只有一个碰撞体"), Ground->SurfaceMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),1);
 const int32 Builds = Ground->GeometryBuildCount;
 Ground->SetActorLocation(FVector(100,200,300));
 Ground->OnConstruction(Ground->GetActorTransform());
 TestEqual(TEXT("整体移动不重新生成本地几何"), Ground->GeometryBuildCount,Builds);
 Ground->SetActorLocation(FVector::ZeroVector);
 SetGroundOutline(Ground,{{0,0,0},{600,0,0},{600,200,0},{200,200,0},{200,600,0},{0,600,0}});
 Ground->HeightOffsetMeters=.25f;
 Ground->CollisionThicknessMeters=.12f;
 Ground->RebuildSurface();
 TestTrue(TEXT("凹地面面积为20平方米"), FMath::IsNearlyEqual(GroundArea(Ground,*this),200000.0,.01));
 TestEqual(TEXT("凹地面保留三角形分区碰撞"), Ground->SurfaceMesh->GetBodySetup()->AggGeom.ConvexElems.Num(),4);
 for (const auto& Probe : TArray<TPair<FVector,bool>>{{FVector(100,400,0),true},{FVector(400,100,0),true},{FVector(400,400,0),false}})
 {
  FHitResult Hit;
  TestEqual(TEXT("凹形缺口不能被碰撞跨越"), Ground->SurfaceMesh->LineTraceComponent(Hit,
   Probe.Key+FVector(0,0,100),Probe.Key-FVector(0,0,100),FCollisionQueryParams()),Probe.Value);
  if (Probe.Value) { TestTrue(TEXT("米制高度与实际行走面一致"),FMath::IsNearlyEqual(Hit.ImpactPoint.Z,25.0,.01)); }
 }
 Ground->bPreserveSplineHeights=true;
 Ground->SurfaceSpline->SetLocationAtSplinePoint(1,FVector(600,0,100),ESplineCoordinateSpace::Local,true);
 Ground->RebuildSurface();
 TestTrue(TEXT("保留高度的坡面仍然可生成"),Ground->GeneratedTriangleCount>0 && Ground->GenerationDiagnostic.IsEmpty());
 Ground->bPreserveSplineHeights=false;
 SetGroundOutline(Ground,{{0,0,0},{600,600,0},{0,600,0},{600,0,0}});
 Ground->RebuildSurface();
 TestTrue(TEXT("自交轮廓明确报告错误"),Ground->GenerationDiagnostic.Contains(TEXT("自交")));
 TestEqual(TEXT("错误轮廓清除旧几何"),Ground->SurfaceMesh->GetNumSections(),0);
 TestTrue(TEXT("错误轮廓清除旧碰撞"),Ground->SurfaceMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 return true;
}

/** 验证连续预览、最后一次输入提交、Nanite、完整恢复及未修改保存复用。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSplineGroundDeferred,"LxARPG.World.SplineSurface.PreviewAndFinalize",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSplineGroundDeferred::RunTest(const FString& Parameters)
{
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Ground=World->SpawnActor<ALxSplineSurfaceActor>();
 TArray<FVector> Circle;
 for(int32 I=0;I<12;++I) { const double Angle=I*2.0*PI/12; Circle.Add(FVector(FMath::Cos(Angle)*2000,FMath::Sin(Angle)*2000,0)); }
 SetGroundOutline(Ground,Circle,true);
 Ground->EdgeSampleSpacingMeters=.25f;
 Ground->FinalizeSurface();
 const int32 FullVertices=Ground->GeneratedVertexCount;
 TestTrue(TEXT("完整曲线采样充分"),FullVertices>256);
 TestTrue(TEXT("最终网格具有Nanite数据"),Ground->OptimizedMesh->GetStaticMesh()->HasValidNaniteData());
 const int32 Builds=Ground->GeometryBuildCount;
 for(int32 I=0;I<20;++I)
 {
  FProperty* Height=FindFProperty<FProperty>(ALxSplineSurfaceActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxSplineSurfaceActor,HeightOffsetMeters));
  Ground->PreEditChange(Height);
  Ground->HeightOffsetMeters=I*.01f;
  FPropertyChangedEvent Event(Height,EPropertyChangeType::Interactive);
  Ground->PostEditChangeProperty(Event);
  TestTrue(TEXT("交互滑块不注销可见地面"),Ground->SurfaceMesh->IsRegistered());
 }
 TestEqual(TEXT("连续参数输入在同一周期合并"),Ground->GeometryBuildCount,Builds);
 FTSTicker::GetCoreTicker().Tick(.06f);
 TestEqual(TEXT("一次预览只重建一次"),Ground->GeometryBuildCount,Builds+1);
 TestTrue(TEXT("预览保留轮廓但降低顶点数量"),Ground->bSurfacePreview && Ground->GeneratedVertexCount<FullVertices && Ground->GeneratedVertexCount<=256);
 TestTrue(TEXT("预览不烹饪行走碰撞"),Ground->SurfaceMesh->GetBodySetup()->AggGeom.ConvexElems.IsEmpty());
 TestEqual(TEXT("预览关闭旧碰撞"),Ground->SurfaceMesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 Ground->HeightOffsetMeters=.40f;
 LxGeneratedMeshBuild::RequestPreview(Ground);
 Ground->FinalizeSurface();
 TestFalse(TEXT("保存运行入口结束预览"),Ground->bSurfacePreview);
 TestEqual(TEXT("完整轮廓顶点数量恢复"),Ground->GeneratedVertexCount,FullVertices);
 TestEqual(TEXT("恢复可行走碰撞"),Ground->SurfaceMesh->GetCollisionEnabled(),ECollisionEnabled::QueryAndPhysics);
 TestTrue(TEXT("最后一次尚未刷新的输入已应用"),FMath::IsNearlyEqual(Ground->SurfaceMesh->GetProcMeshSection(0)->ProcVertexBuffer[0].Position.Z,40.0,.01));
 UStaticMesh* Cached=Ground->OptimizedMesh->GetStaticMesh();
 const int32 FinalBuilds=Ground->GeometryBuildCount;
 Ground->RerunConstructionScripts();
 TestTrue(TEXT("真实构造重跑不重置米制参数"),FMath::IsNearlyEqual(Ground->HeightOffsetMeters,.40f)
  && FMath::IsNearlyEqual(Ground->EdgeSampleSpacingMeters,.25f));
 TestEqual(TEXT("真实构造重跑复用完整几何"),Ground->GeometryBuildCount,FinalBuilds);
 FTSTicker::GetCoreTicker().Tick(.06f);
 Ground->FinalizeSurface();
 TestEqual(TEXT("旧预览请求及重复保存不重建"),Ground->GeometryBuildCount,FinalBuilds);
 TestTrue(TEXT("未修改时复用最终静态缓存"),Ground->OptimizedMesh->GetStaticMesh()==Cached);
 auto* Copy=DuplicateObject<ALxSplineSurfaceActor>(Ground,World->PersistentLevel);
 const int32 CopyBuilds=Copy->GeometryBuildCount;
 Copy->OnConstruction(Copy->GetActorTransform());
 TestEqual(TEXT("复制后复用完整源数据"),Copy->GeometryBuildCount,CopyBuilds);
 Ground->EdgeSampleSpacingMeters=.05f;
 Ground->RebuildSurface();
 TestTrue(TEXT("高密度曲线超过预算时给出提示"),Ground->GenerationDiagnostic.Contains(TEXT("预算")));
 return true;
}

/** 模拟无项目版本的旧厘米属性文件，验证迁移一次后新归档保持米制值。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSplineGroundMigration,"LxARPG.World.SplineSurface.LegacyUnits",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSplineGroundMigration::RunTest(const FString& Parameters)
{
 auto* Source=NewObject<ALxSplineSurfaceActor>();
 Source->HeightOffset=125; Source->MaxEdgeSampleLength=75; Source->CollisionThickness=18;
 Source->GeneratedSurfaceLocation=FVector(250,-350,50);
 TArray<uint8> Legacy;
 FMemoryWriter Writer(Legacy,true);
 FObjectAndNameAsStringProxyArchive Save(Writer,false);
 Source->AActor::Serialize(Save);
 auto* Loaded=NewObject<ALxSplineSurfaceActor>();
 FMemoryReader Reader(Legacy,true); Reader.SetCustomVersions(Writer.GetCustomVersions());
 FObjectAndNameAsStringProxyArchive Load(Reader,false);
 Loaded->Serialize(Load); Loaded->PostLoad();
 TestTrue(TEXT("旧厘米高度迁移为米"),FMath::IsNearlyEqual(Loaded->HeightOffsetMeters,1.25f));
 TestTrue(TEXT("旧厘米采样间距迁移为米"),FMath::IsNearlyEqual(Loaded->EdgeSampleSpacingMeters,.75f));
 TestTrue(TEXT("旧厘米碰撞厚度迁移为米"),FMath::IsNearlyEqual(Loaded->CollisionThicknessMeters,.18f));
 TestTrue(TEXT("旧位置保留原有尺寸"),Loaded->GeneratedSurfaceLocationMeters.Equals(FVector(2.5,-3.5,.5)));
 Loaded->HeightOffsetMeters=2.25f;
 TArray<uint8> Modern;
 FMemoryWriter ModernWriter(Modern,true);
 FObjectAndNameAsStringProxyArchive ModernSave(ModernWriter,false); Loaded->Serialize(ModernSave);
 auto* Reloaded=NewObject<ALxSplineSurfaceActor>();
 FMemoryReader ModernReader(Modern,true); ModernReader.SetCustomVersions(ModernWriter.GetCustomVersions());
 FObjectAndNameAsStringProxyArchive ModernLoad(ModernReader,false);
 Reloaded->Serialize(ModernLoad); Reloaded->PostLoad();
 TestTrue(TEXT("新格式加载不会再次除以100"),FMath::IsNearlyEqual(Reloaded->HeightOffsetMeters,2.25f));
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 auto* Ground=World->SpawnActor<ALxSplineSurfaceActor>();
 Ground->HeightOffset=175; Ground->OnConstruction(Ground->GetActorTransform());
 TestTrue(TEXT("旧蓝图厘米设置节点仍生效"),FMath::IsNearlyEqual(Ground->HeightOffsetMeters,1.75f));
 Ground->HeightOffsetMeters=2.5f; Ground->RebuildSurface();
 TestTrue(TEXT("新米制编辑与旧读取节点一致"),FMath::IsNearlyEqual(Ground->HeightOffset,250.f));
 return true;
}

/** 通过真实编辑器保存流程提交未刷新的参数，输出供新进程独立恢复的验证关卡。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxSplineGroundSave,"LxARPG.World.SplineSurface.PersistentSave",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxSplineGroundSave::RunTest(const FString& Parameters)
{
 const FString Directory=FPaths::ProjectSavedDir()/TEXT("Diagnostics/样条地面");
 IFileManager::Get().MakeDirectory(*Directory,true);
 UPackage* Package=CreatePackage(TEXT("/Temp/样条地面缓存验收"));
 UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false,TEXT("样条地面缓存验收"),Package);
 ON_SCOPE_EXIT { World->DestroyWorld(false); };
 if (!World->IsInitialized()) World->InitWorld(UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
  .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).EnableTraceCollision(true));
 auto* Curve=World->SpawnActor<ALxSplineSurfaceActor>();
 TArray<FVector> Circle;
 for(int32 I=0;I<12;++I) { const double Angle=I*2.0*PI/12; Circle.Add(FVector(FMath::Cos(Angle)*2000,FMath::Sin(Angle)*2000,0)); }
 SetGroundOutline(Curve,Circle,true); Curve->EdgeSampleSpacingMeters=.25f;
 LxGeneratedMeshBuild::RequestPreview(Curve); Curve->RebuildSurface();
 Curve->HeightOffsetMeters=.32f; // 故意不等待预览队列，保存必须捕获最后一次输入。
 auto* Concave=World->SpawnActor<ALxSplineSurfaceActor>(FVector(5000,0,0),FRotator::ZeroRotator);
 SetGroundOutline(Concave,{{0,0,0},{600,0,0},{600,200,0},{200,200,0},{200,600,0},{0,600,0}});
 Concave->HeightOffsetMeters=.18f;
 const FString Filename=Directory/TEXT("地面缓存验收.umap");
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 const double Start=FPlatformTime::Seconds();
 TestTrue(TEXT("保存独立关卡成功"),GEditor->SavePackage(Package,World,*Filename,Args));
 const double FirstSeconds=FPlatformTime::Seconds()-Start;
 TestFalse(TEXT("磁盘保存前已退出预览"),Curve->bSurfacePreview);
 TestTrue(TEXT("磁盘保存包含完整曲线"),Curve->GeneratedVertexCount>256);
 TestTrue(TEXT("保存前提交最新米制参数"),FMath::IsNearlyEqual(Curve->SurfaceMesh->GetProcMeshSection(0)->ProcVertexBuffer[0].Position.Z,32.0,.01));
 auto* Compact=CastChecked<ULxCompactProceduralMeshComponent>(Curve->SurfaceMesh);
 TestTrue(TEXT("完整源几何已保存为紧凑缓存"),Compact->CompactStoredBytes>0 && Compact->CompactStoredBytes<Compact->CompactRawBytes);
 const int32 Builds=Curve->GeometryBuildCount,Compressions=Compact->CompactCompressionCount;
 const double RepeatStart=FPlatformTime::Seconds();
 TestTrue(TEXT("未修改重复保存成功"),GEditor->SavePackage(Package,World,*Filename,Args));
 const double RepeatSeconds=FPlatformTime::Seconds()-RepeatStart;
 TestEqual(TEXT("重复保存不重建曲线"),Curve->GeometryBuildCount,Builds);
 TestEqual(TEXT("重复保存不重新压缩几何"),Compact->CompactCompressionCount,Compressions);
 auto Report=MakeShared<FJsonObject>();
 Report->SetNumberField(TEXT("first_save_seconds"),FMath::RoundToDouble(FirstSeconds*100)/100);
 Report->SetNumberField(TEXT("repeat_save_seconds"),FMath::RoundToDouble(RepeatSeconds*100)/100);
 Report->SetNumberField(TEXT("vertices"),Curve->GeneratedVertexCount);
 Report->SetNumberField(TEXT("triangles"),Curve->GeneratedTriangleCount);
 Report->SetNumberField(TEXT("raw_bytes"),Compact->CompactRawBytes);
 Report->SetNumberField(TEXT("compressed_bytes"),Compact->CompactStoredBytes);
 Report->SetNumberField(TEXT("repeat_builds"),Curve->GeometryBuildCount-Builds);
 Report->SetNumberField(TEXT("repeat_compressions"),Compact->CompactCompressionCount-Compressions);
 Report->SetNumberField(TEXT("map_bytes"),IFileManager::Get().FileSize(*Filename));
 FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
 TestTrue(TEXT("保存验收报告成功"),FFileHelper::SaveStringToFile(Json,*(Directory/TEXT("保存验收.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
 return true;
}
#endif
