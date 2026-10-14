#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/PackagePath.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/SavePackage.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/StrongObjectPtr.h"
#include "PhysicsEngine/BodySetup.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "LxARPG/LxSource/World/Ocean/LxOceanActor.h"

namespace
{
 /** 由公开分区数据独立计算摘要，覆盖全部顶点属性、绕序、分区状态及碰撞设置。 */
 FString MeshFingerprint(UProceduralMeshComponent* Mesh,int64& VertexCount,int64& TriangleCount)
 {
  TArray<uint8> Bytes; FMemoryWriter Writer(Bytes);
  int32 Sections=Mesh->GetNumSections(); Writer << Sections;
  for(int32 S=0;S<Sections;++S)
  {
   FProcMeshSection Section=*Mesh->GetProcMeshSection(S);
   Writer << Section.bEnableCollision << Section.bSectionVisible << Section.SectionLocalBox;
   int32 Vertices=Section.ProcVertexBuffer.Num(); Writer << Vertices << Section.ProcIndexBuffer;
   VertexCount+=Vertices; TriangleCount+=Section.ProcIndexBuffer.Num()/3;
   for(FProcMeshVertex& V:Section.ProcVertexBuffer)
    Writer << V.Position << V.Normal << V.Tangent.TangentX << V.Tangent.bFlipTangentY << V.Color << V.UV0 << V.UV1 << V.UV2 << V.UV3;
  }
  uint8 Collision=uint8(Mesh->GetCollisionEnabled()); Writer << Collision << Mesh->bUseComplexAsSimpleCollision;
  if(UBodySetup* Body=Mesh->GetBodySetup())
  {
   int32 Hulls=Body->AggGeom.ConvexElems.Num(); Writer << Hulls;
   for(FKConvexElem& Convex:Body->AggGeom.ConvexElems) Writer << Convex.VertexData << Convex.ElemBox;
  }
  return LexToString(FIoHash::HashBuffer(Bytes.GetData(),Bytes.Num()));
 }
 /** 比较实际材质资产绑定；运行时动态实例名称可变，其控制参数由 Actor 参数摘要核对。 */
 FString MaterialFingerprint(UProceduralMeshComponent* Mesh)
 {
  TArray<uint8> Bytes; FMemoryWriter Writer(Bytes);
  int32 Count=Mesh->GetNumMaterials(); Writer << Count;
  for(int32 M=0;M<Count;++M)
  {
   UMaterialInterface* Material=Mesh->GetMaterial(M);
   while(auto* Dynamic=Cast<UMaterialInstanceDynamic>(Material)) Material=Dynamic->Parent;
   FString Path=Material?Material->GetPathName():FString(); Writer << Path;
  }
  return LexToString(FIoHash::HashBuffer(Bytes.GetData(),Bytes.Num()));
 }
 /** 写入中文 UTF-8 诊断结果，使基准及独立重载会话可逐组件核对。 */
 bool WriteReport(const FString& Filename,const TSharedRef<FJsonObject>& Report)
 {
  FString Json; const auto Writer=TJsonWriterFactory<>::Create(&Json);
  return FJsonSerializer::Serialize(Report,Writer) && FFileHelper::SaveStringToFile(Json,*Filename,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
 }
 /** 统计恢复后的源网格并独立检查它们全部已升级到紧凑组件。 */
 TSharedRef<FJsonObject> WorldManifest(UWorld* World,int32& MeshCount,int32& CompactCount,int64& Vertices,int64& Triangles)
 {
  auto Manifest=MakeShared<FJsonObject>();
  for(AActor* Actor:World->PersistentLevel->Actors)
  {
   if(!Actor) continue;
   TInlineComponentArray<UProceduralMeshComponent*> Meshes(Actor);
   if(Meshes.IsEmpty()) continue;
   const TArray<uint8> Inputs=LxGeneratedMeshBuild::CaptureInputs(Actor);
   Manifest->SetStringField(Actor->GetName()+TEXT("/参数"),LexToString(FIoHash::HashBuffer(Inputs.GetData(),Inputs.Num())));
   for(UProceduralMeshComponent* Mesh:Meshes)
   {
    ++MeshCount; if(Mesh->IsA<ULxCompactProceduralMeshComponent>()) ++CompactCount;
    const FString Key=Actor->GetName()+TEXT("/")+Mesh->GetName();
    Manifest->SetStringField(Key,MeshFingerprint(Mesh,Vertices,Triangles));
    Manifest->SetStringField(Key+TEXT("/材质"),MaterialFingerprint(Mesh));
   }
  }
  return Manifest;
 }
}

/** 显式基准入口：旧关卡只读加载，输出独立副本；第二个进程验证该副本完整恢复并再次保存。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCompactMeshBenchmark,"LxARPG.Diagnostics.CompactMeshSave",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxCompactMeshBenchmark::RunTest(const FString& Parameters)
{
 FString Mode; FParse::Value(FCommandLine::Get(),TEXT("LxMeshSaveMode="),Mode);
 if(Mode!=TEXT("benchmark") && Mode!=TEXT("reload")) { AddInfo(TEXT("实关卡基准只在显式指定 LxMeshSaveMode 时执行。")); return true; }
 const FString PackageName=TEXT("/Game/项目内容/关卡/唯一关卡");
 const FString Directory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Diagnostics/紧凑网格保存"));
 IFileManager::Get().MakeDirectory(*Directory,true);
 const FString Filename=Directory/TEXT("关卡缓存验证.umap");
 const double LoadStart=FPlatformTime::Seconds();
 UPackage* Package=Mode==TEXT("reload")
  ? LoadPackage(CreatePackage(*PackageName),FPackagePath::FromLocalPath(Filename),LOAD_None)
  : LoadPackage(nullptr,*PackageName,LOAD_None);
 UWorld* World=Package?UWorld::FindWorldInPackage(Package):nullptr;
 if(!TestNotNull(TEXT("加载基准关卡"),World)) return false;
 // 对齐编辑器正常加载流程，注册组件和变换；明确不重跑构造脚本或参数生成。
 World->WorldType=EWorldType::Editor;
 if(!World->IsInitialized()) World->InitWorld(UWorld::InitializationValues().AllowAudioPlayback(false)
  .CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).ShouldSimulatePhysics(false));
 World->UpdateWorldComponents(false,false);
 const double LoadSeconds=FPlatformTime::Seconds()-LoadStart;
 auto Report=MakeShared<FJsonObject>();
 Report->SetStringField(TEXT("mode"),Mode); Report->SetStringField(TEXT("source_package"),PackageName);
 Report->SetStringField(TEXT("copy_file"),Filename); Report->SetNumberField(TEXT("load_seconds"),LoadSeconds);
 int32 MeshCount=0,CompactCount=0; int64 Vertices=0,Triangles=0;
 auto Manifest=WorldManifest(World,MeshCount,CompactCount,Vertices,Triangles);
 TestTrue(TEXT("当前关卡保留程序化源组件"),MeshCount>0);
 TestEqual(TEXT("所有旧原生组件自动迁移为紧凑子类"),CompactCount,MeshCount);
 if(MeshCount<=0 || CompactCount!=MeshCount) return false;
 Report->SetNumberField(TEXT("mesh_count"),MeshCount); Report->SetNumberField(TEXT("vertices"),double(Vertices));
 Report->SetNumberField(TEXT("triangles"),double(Triangles));
 if(Mode==TEXT("reload"))
 {
  FString Json; TSharedPtr<FJsonObject> Previous;
  if(!TestTrue(TEXT("读取第一轮保存数据摘要"),FFileHelper::LoadFileToString(Json,*(Directory/TEXT("首次保存.json")))
   && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Previous))) return false;
  const auto Expected=Previous->GetObjectField(TEXT("manifest"));
  TestEqual(TEXT("重启后参数及网格条目数一致"),Manifest->Values.Num(),Expected->Values.Num());
  for(const auto& Pair:Expected->Values)
   TestEqual(*FString::Printf(TEXT("完整恢复 %s"),*Pair.Key),Manifest->GetStringField(Pair.Key),Pair.Value->AsString());
 }
 // 与编辑器保存入口一样先完成世界网格；计时包含准备与真正的包写入。
 FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
 const double FirstStart=FPlatformTime::Seconds();
 LxGeneratedMeshBuild::PrepareWorld(World);
 const double Prepared=FPlatformTime::Seconds();
 const bool Saved=GEditor->SavePackage(Package,World,*Filename,Args);
 const double FirstSeconds=FPlatformTime::Seconds()-FirstStart;
 TestTrue(TEXT("保存独立副本成功"),Saved); if(!Saved) return false;
 MeshCount=0; CompactCount=0; Vertices=0; Triangles=0;
 Manifest=WorldManifest(World,MeshCount,CompactCount,Vertices,Triangles);
 Report->SetObjectField(TEXT("manifest"),Manifest);
 Report->SetNumberField(TEXT("first_save_seconds"),FirstSeconds);
 Report->SetNumberField(TEXT("first_prepare_seconds"),Prepared-FirstStart);
 Report->SetNumberField(TEXT("first_package_seconds"),FirstSeconds-(Prepared-FirstStart));
 Report->SetNumberField(TEXT("file_bytes"),double(IFileManager::Get().FileSize(*Filename)));
 // 多次保存取中位数，避免单次文件写入波动掩盖性能回退。
 TArray<double> RepeatSamples;
 TArray<TSharedPtr<FJsonValue>> RepeatJson;
 for(int32 Sample=0;Sample<5;++Sample)
 {
  const double RepeatStart=FPlatformTime::Seconds();
  LxGeneratedMeshBuild::PrepareWorld(World);
  TestTrue(TEXT("重复保存独立副本成功"),GEditor->SavePackage(Package,World,*Filename,Args));
  const double Seconds=FPlatformTime::Seconds()-RepeatStart;
  RepeatSamples.Add(Seconds); RepeatJson.Add(MakeShared<FJsonValueNumber>(Seconds));
 }
 RepeatSamples.Sort();
 const double RepeatSeconds=RepeatSamples[RepeatSamples.Num()/2];
 Report->SetNumberField(TEXT("repeat_save_seconds"),RepeatSeconds);
 Report->SetArrayField(TEXT("repeat_save_samples"),RepeatJson);
 int64 RawBytes=0,StoredBytes=0; int32 Compressions=0;
 for(AActor* Actor:World->PersistentLevel->Actors)
 {
  if(!Actor) continue;
  TInlineComponentArray<ULxCompactProceduralMeshComponent*> Meshes(Actor);
  for(const auto* Mesh:Meshes) { RawBytes+=Mesh->CompactRawBytes; StoredBytes+=Mesh->CompactStoredBytes; Compressions+=Mesh->CompactCompressionCount; }
 }
 Report->SetNumberField(TEXT("raw_geometry_bytes"),double(RawBytes));
 Report->SetNumberField(TEXT("compressed_geometry_bytes"),double(StoredBytes));
 Report->SetNumberField(TEXT("compression_count"),Compressions);
 // 显式验证实关卡的每个程序化组件，覆盖完整顶点、全部碰撞几何及材质引用。
 if(FParse::Param(FCommandLine::Get(),TEXT("LxVerifyMeshPIE")))
 {
  int32 VerifiedMeshes=0; double CopySeconds=0;
  for(AActor* Actor:World->PersistentLevel->Actors)
  {
   if(!Actor) continue;
   TInlineComponentArray<ULxCompactProceduralMeshComponent*> Meshes(Actor);
   for(auto* Mesh:Meshes)
   {
    int64 SourceVertices=0,SourceTriangles=0,CopyVertices=0,CopyTriangles=0;
    const FString Expected=MeshFingerprint(Mesh,SourceVertices,SourceTriangles);
    FObjectDuplicationParameters CopyParams(Mesh,GetTransientPackage());
    CopyParams.DuplicateMode=EDuplicateMode::PIE; CopyParams.PortFlags|=PPF_DuplicateForPIE;
    const double CopyStart=FPlatformTime::Seconds();
    TStrongObjectPtr<ULxCompactProceduralMeshComponent> Copy(Cast<ULxCompactProceduralMeshComponent>(StaticDuplicateObjectEx(CopyParams)));
    CopySeconds+=FPlatformTime::Seconds()-CopyStart;
    if(!TestNotNull(*FString::Printf(TEXT("PIE 复制组件 %s"),*Mesh->GetPathName()),Copy.Get())) return false;
    const bool bGeometryMatches=TestEqual(*FString::Printf(TEXT("PIE 保留完整几何及碰撞 %s"),*Mesh->GetPathName()),
     MeshFingerprint(Copy.Get(),CopyVertices,CopyTriangles),Expected);
    const bool bMaterialsMatch=TestEqual(*FString::Printf(TEXT("PIE 保留材质 %s"),*Mesh->GetPathName()),
     MaterialFingerprint(Copy.Get()),MaterialFingerprint(Mesh));
    if(!bGeometryMatches || !bMaterialsMatch) return false;
    ++VerifiedMeshes;
   }
  }
  TestEqual(TEXT("所有实关卡程序化组件都通过 PIE 无损核对"),VerifiedMeshes,MeshCount);
  Report->SetNumberField(TEXT("pie_verified_meshes"),VerifiedMeshes);
  Report->SetNumberField(TEXT("pie_component_copy_seconds"),CopySeconds);
 }
 // 独立重载之后继续编辑海岸参数，验证源几何仍能按参数重建，而非仅能显示缓存。
 if(Mode==TEXT("reload"))
 {
  for(AActor* Actor:World->PersistentLevel->Actors)
  {
   auto* Ocean=Cast<ALxOceanActor>(Actor); if(!Ocean) continue;
   const int32 Before=Ocean->FullMeshBuildCount; Ocean->OceanExtend+=100;
   Ocean->RequestOceanPreview(); Ocean->FinalizeOcean();
   TestEqual(TEXT("紧凑关卡恢复后海岸参数仍可完整重建"),Ocean->FullMeshBuildCount,Before+1);
   TestFalse(TEXT("参数重建提交完整海面"),Ocean->bUsingEditorPreview);
  }
 }
 const FString ReportPath=Directory/(Mode==TEXT("reload")?TEXT("重新加载.json"):TEXT("首次保存.json"));
 TestTrue(TEXT("输出保存时间与逐组件恢复摘要"),WriteReport(ReportPath,Report));
 AddInfo(FString::Printf(TEXT("紧凑保存基准 模式=%s 组件=%d 首次=%.2f秒 重复=%.2f秒 文件=%.2fMB 原始几何=%.2fMB 压缩几何=%.2fMB"),
  *Mode,MeshCount,FirstSeconds,RepeatSeconds,double(IFileManager::Get().FileSize(*Filename))/1000000.0,double(RawBytes)/1000000.0,double(StoredBytes)/1000000.0));
 return true;
}
#endif
