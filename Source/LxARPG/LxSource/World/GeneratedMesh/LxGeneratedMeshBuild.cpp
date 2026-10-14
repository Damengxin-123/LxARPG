#include "LxGeneratedMeshBuild.h"
#include "LxGeneratedMeshRenderComponent.h"
#include "LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/Ocean/LxOceanActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "EngineUtils.h"
#include "Components/SplineComponent.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/StructuredArchiveAdapters.h"
#include "UObject/UnrealType.h"
#include "Containers/Ticker.h"

namespace
{
 /** 弱引用不会阻止删除、切换地图或撤销后的对象回收。 */
 TSet<TWeakObjectPtr<AActor>> PreviewActors;
 /** 尚未提交的预览；每个对象在一次节流窗口内只排队一次。 */
 TSet<TWeakObjectPtr<AActor>> PendingPreviews;
 /** 保存阶段统一禁用所有邻居的简化生成。 */
 bool bPreparingWorld = false;
 /** 输入快照只写入稳定对象路径；PIE 包前缀不应造成缓存失效。 */
 class FLxInputArchive final : public FObjectAndNameAsStringProxyArchive
 {
 public:
  /** 包装内存写入器，不加载任何引用资产。 */
  explicit FLxInputArchive(FArchive& Inner) : FObjectAndNameAsStringProxyArchive(Inner, false) {}
  using FObjectAndNameAsStringProxyArchive::operator<<;
  /** 以编辑器原始路径记录引用，允许已保存缓存直接用于 PIE。 */
  virtual FArchive& operator<<(UObject*& Object) override
  {
   FString Path = Object ? UWorld::RemovePIEPrefix(Object->GetPathName()) : FString();
   *this << Path;
   return *this;
  }
  /** TObjectPtr 与普通对象引用使用同一规范化规则。 */
  virtual FArchive& operator<<(FObjectPtr& Object) override
  {
   UObject* Value = Object.Get();
   return *this << Value;
  }
 };
}

TArray<uint8> LxGeneratedMeshBuild::CaptureInputs(const AActor* Actor, bool bIncludeWorldTransform, bool bIncludePreviewState)
{
 TArray<uint8> Inputs;
 if (!Actor) { return Inputs; }
 FMemoryWriter Writer(Inputs);
 FLxInputArchive InputArchive(Writer);
 FArchive& Archive = InputArchive;
 // 输入结构版本独立于几何构建版本，旧快照仅做一次迁移。
 int32 InputVersion = 1;
 double BuildVersion = CacheVersion;
 bool bPreview = bIncludePreviewState && IsPreview(Actor);
 Archive << InputVersion << BuildVersion << bPreview;
 FTransform Transform = bIncludeWorldTransform ? Actor->GetActorTransform() : FTransform::Identity;
 Archive << Transform;
 for (TFieldIterator<FProperty> It(Actor->GetClass()); It; ++It)
 {
  FProperty* Property = *It;
  if (Property->GetOwnerClass() == AActor::StaticClass()
   || !Property->HasAnyPropertyFlags(CPF_Edit)
   || Property->HasAnyPropertyFlags(CPF_EditConst | CPF_Transient)) { continue; }
  FName Name = Property->GetFName();
  Archive << Name;
  for (int32 Index = 0; Index < Property->ArrayDim; ++Index)
  { Property->SerializeItem(FStructuredArchiveFromArchive(Archive).GetSlot(), const_cast<void*>(Property->ContainerPtrToValuePtr<void>(Actor, Index)), nullptr); }
 }
 // 只比较控制曲线和采样设置，跳过样条重参数表以及生成网格数据。
 TInlineComponentArray<USplineComponent*> Splines(Actor);
 for (USplineComponent* Spline : Splines)
 {
  FName Name = Spline->GetFName();
  FTransform SplineTransform = !bIncludeWorldTransform && Spline == Actor->GetRootComponent()
   ? FTransform::Identity : Spline->GetRelativeTransform();
  Archive << Name << SplineTransform;
  Archive << Spline->SplineCurves.Position << Spline->SplineCurves.Rotation << Spline->SplineCurves.Scale;
  bool bClosed = Spline->IsClosedLoop();
  FVector Up = Spline->GetDefaultUpVector(ESplineCoordinateSpace::Local);
  Archive << bClosed << Up << Spline->ReparamStepsPerSegment;
 }
 return Inputs;
}

void LxGeneratedMeshBuild::ConfigureNanite(UStaticMesh* Mesh, bool bEnableNanite)
{
#if WITH_EDITOR
 FMeshNaniteSettings Settings = Mesh->GetNaniteSettings();
 Settings.bEnabled = bEnableNanite;
 Settings.FallbackTarget = ENaniteFallbackTarget::PercentTriangles;
 Settings.FallbackPercentTriangles = 1.0f;
 Settings.FallbackRelativeError = 0.0f;
 Mesh->SetNaniteSettings(Settings);
#endif
}

bool LxGeneratedMeshBuild::Build(UStaticMesh* Mesh, const FMeshDescription& Description, bool bEnableNanite, bool bFinalize)
{
 const double BuildStart=FPlatformTime::Seconds();
 const AActor* Owner = Mesh->GetTypedOuter<AActor>();
 const bool bRuntimeGeneration = Owner && Owner->GetWorld() && Owner->GetWorld()->IsGameWorld();
 ULxGeneratedStaticMesh* Generated=Cast<ULxGeneratedStaticMesh>(Mesh);
 const bool bDefer=Generated && !bRuntimeGeneration && bEnableNanite && !bFinalize;
 ConfigureNanite(Mesh, bEnableNanite && !bRuntimeGeneration && !bDefer);
 UStaticMesh::FBuildMeshDescriptionsParams Params;
 Params.bMarkPackageDirty = false;
 Params.bCommitMeshDescription = WITH_EDITOR;
 Params.PerLODOverrides.AddDefaulted_GetRef().bUseFullPrecisionUVs=true;
#if WITH_EDITOR
 // 完整编辑器构建才能生成 Nanite 数据，快速构建不会生成这些数据。
 Params.bFastBuild = bRuntimeGeneration || !bEnableNanite || bDefer;
 Mesh->SetNumSourceModels(1);
 FMeshBuildSettings& Settings = Mesh->GetSourceModel(0).BuildSettings;
 Settings.bRecomputeNormals = false;
 Settings.bRecomputeTangents = false;
 Settings.bRemoveDegenerates = false;
 Settings.bGenerateLightmapUVs = false;
 Settings.bUseFullPrecisionUVs = true;
#else
 // 运行时新生成或改变形状仍可用；预先保存的网格直接使用已烹饪的 Nanite 数据。
 Params.bFastBuild = true;
#endif
 // 引擎快速构建会保留此标记，转为完整构建前必须显式清除。
 Mesh->bDoFastBuild=Params.bFastBuild;
 const bool bSuccess=Mesh->BuildFromMeshDescriptions({&Description}, Params);
 if(bFinalize) UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("最终静态网格构建 %s 顶点=%d 三角形=%d 耗时=%.2f秒 成功=%d"),
  *Mesh->GetPathName(),Description.Vertices().Num(),Description.Triangles().Num(),FPlatformTime::Seconds()-BuildStart,bSuccess);
 if(Generated && bSuccess) { Generated->bPendingNaniteBuild=bDefer; }
 return bSuccess;
}

bool LxGeneratedMeshBuild::Finalize(UStaticMesh* Mesh)
{
#if WITH_EDITOR
 ULxGeneratedStaticMesh* Generated=Cast<ULxGeneratedStaticMesh>(Mesh);
 if(Generated && Generated->bPendingNaniteBuild)
 {
  const FMeshDescription* Source=Generated->GetMeshDescription(0);
  if(!Source) { return false; }
  // 完整构建会重新提交源描述，使用副本避免读写同一缓冲。
  const FMeshDescription Description=*Source;
  return Build(Generated,Description,true,true);
 }
#endif
 return Mesh!=nullptr;
}

void LxGeneratedMeshBuild::PrepareWorld(UWorld* World)
{
#if WITH_EDITOR
 if(!World || World->IsGameWorld()) { return; }
 const double PrepareStart=FPlatformTime::Seconds();
 int32 RebuiltActors=0;
 TGuardValue<bool> Preparing(bPreparingWorld, true);
 for(TActorIterator<AActor> It(World); It; ++It) { EndPreview(*It); }
 // 海洋使用程序化海面，同样在保存及 PIE 复制世界之前提交最新完整几何。
 for(TActorIterator<ALxOceanActor> It(World); It; ++It) { It->FinalizeOcean(); }
 const double OceanEnd=FPlatformTime::Seconds();
 for(TActorIterator<AActor> It(World); It; ++It)
 {
  AActor* Actor=*It;
  TInlineComponentArray<UStaticMeshComponent*> Components(Actor);
  bool bGenerated=false;
  for(UStaticMeshComponent* Component : Components)
  {
   // 旧关卡的内嵌缓存仍是引擎原生 UStaticMesh，同样需要在运行前提交待编辑参数。
   bGenerated |= Component->IsA<ULxGeneratedMeshRenderComponent>()
    || (Component->GetStaticMesh() && Component->GetStaticMesh()->GetTypedOuter<AActor>()==Actor);
  }
  if(!bGenerated || Actor->IsTemplate()) { continue; }
  // 比较实际输入，既跳过未修改对象，也提交尚未触发防抖的最后一次调整。
  const TArray<uint8> Inputs = CaptureInputs(Actor);
  bool bNeedsConstruction = false;
  for (UStaticMeshComponent* Component : Components)
  {
   if (const auto* Optimized = Cast<ULxGeneratedMeshRenderComponent>(Component))
   { bNeedsConstruction |= !Optimized->IsSourceCurrent(); }
   else if (UStaticMesh* Mesh = Component->GetStaticMesh(); Mesh && Mesh->GetTypedOuter<AActor>() == Actor)
   {
    const auto* Generated = Cast<ULxGeneratedStaticMesh>(Mesh);
    bNeedsConstruction |= !Generated || Generated->PreparedInputs != Inputs;
   }
  }
  if (bNeedsConstruction) { ++RebuiltActors; Actor->OnConstruction(Actor->GetActorTransform()); }
  Actor->GetComponents(Components);
  if (bNeedsConstruction)
  {
   const TArray<uint8> AppliedInputs = CaptureInputs(Actor);
   for (UStaticMeshComponent* Component : Components)
   {
    if (auto* Generated = Cast<ULxGeneratedStaticMesh>(Component->GetStaticMesh()))
    { if (Generated->GetTypedOuter<AActor>() == Actor) { Generated->PreparedInputs = AppliedInputs; } }
   }
  }
 }
 // 依赖对象可能在第一遍中被邻居更新，统一在第二遍完成，避免遗漏或重复构建。
 const double InputsEnd=FPlatformTime::Seconds();
 for (TActorIterator<AActor> It(World); It; ++It)
 {
  TInlineComponentArray<UStaticMeshComponent*> Components(*It);
  for(UStaticMeshComponent* Component : Components)
  {
   if(ULxGeneratedMeshRenderComponent* Optimized=Cast<ULxGeneratedMeshRenderComponent>(Component))
   { Optimized->FinalizeMesh(); }
   else if(Component->GetStaticMesh() && Component->GetStaticMesh()->GetTypedOuter<AActor>() == *It)
   { Finalize(Component->GetStaticMesh()); RefreshMaterialCompatibility(Component); }
  }
 }
 const double PrepareEnd=FPlatformTime::Seconds();
 UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("世界网格准备 %s 海洋=%.2f秒 参数检查及生成=%.2f秒 静态网格完成=%.2f秒 重建对象=%d 总计=%.2f秒"),
  *World->GetPathName(),OceanEnd-PrepareStart,InputsEnd-OceanEnd,PrepareEnd-InputsEnd,RebuiltActors,PrepareEnd-PrepareStart);
#endif
}

void LxGeneratedMeshBuild::RefreshMaterialCompatibility(UStaticMeshComponent* Component)
{
 bool bUnsupported = false;
 for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
 {
  const UMaterialInterface* Material = Component->GetMaterial(Slot);
  if (Material && (Material->GetMaterial()->MaterialDomain != MD_Surface
   || (Material->GetBlendMode() != BLEND_Opaque && Material->GetBlendMode() != BLEND_Masked)))
  {
   bUnsupported = true;
   break;
  }
 }
 Component->SetForceDisableNanite(bUnsupported);
}


bool LxGeneratedMeshBuild::IsPreview(const AActor* Actor)
{
 return !bPreparingWorld && Actor && Actor->GetWorld() && !Actor->GetWorld()->IsGameWorld()
  && PreviewActors.Contains(TWeakObjectPtr<AActor>(const_cast<AActor*>(Actor)));
}

void LxGeneratedMeshBuild::EndPreview(AActor* Actor)
{
 PreviewActors.Remove(Actor);
 PendingPreviews.Remove(Actor);
}

void LxGeneratedMeshBuild::RequestPreview(AActor* Actor)
{
#if WITH_EDITOR
 if(bPreparingWorld || !IsValid(Actor) || Actor->IsTemplate() || !Actor->GetWorld() || Actor->GetWorld()->IsGameWorld()) { return; }
 PreviewActors.Add(Actor);
 if(PendingPreviews.Contains(Actor)) { return; }
 PendingPreviews.Add(Actor);
 const TWeakObjectPtr<AActor> WeakActor(Actor);
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakActor](float)
 {
  if(PendingPreviews.Remove(WeakActor) && WeakActor.IsValid() && !WeakActor->IsActorBeingDestroyed())
  {
   // 不重跑蓝图和组件注销流程，只更新生成器原生几何，避免拖动时闪烁。
   WeakActor->OnConstruction(WeakActor->GetActorTransform());
  }
  for(auto It=PreviewActors.CreateIterator(); It; ++It) { if(!It->IsValid()) { It.RemoveCurrent(); } }
  return false;
 }), 0.05f);
#endif
}
