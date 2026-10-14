#include "LxGeneratedMeshRenderComponent.h"
#include "LxGeneratedMeshBuild.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "UObject/ObjectSaveContext.h"

ULxGeneratedMeshRenderComponent::ULxGeneratedMeshRenderComponent()
{
 SetMobility(EComponentMobility::Movable);
 SetCollisionProfileName(TEXT("NoCollision"));
 SetGenerateOverlapEvents(false);
 SetCanEverAffectNavigation(false);
}

void ULxGeneratedMeshRenderComponent::OnRegister()
{
 Super::OnRegister();
#if WITH_EDITOR
 if (!GetStaticMesh() && GetWorld() && !GetWorld()->IsGameWorld())
 {
  if (UProceduralMeshComponent* Source = Cast<UProceduralMeshComponent>(GetAttachParent()))
  { SetVisibility(false); Source->SetVisibility(true); }
 }
#endif
}

void ULxGeneratedMeshRenderComponent::BeginPlay()
{
 Super::BeginPlay();
 FinalizeMesh();
}

bool ULxGeneratedMeshRenderComponent::IsSourceCurrent() const
{
 return !SourceInputs.IsEmpty() && SourceInputs == LxGeneratedMeshBuild::CaptureInputs(GetOwner());
}

void ULxGeneratedMeshRenderComponent::AcceptUnchangedSource()
{
 SourceInputs = LxGeneratedMeshBuild::CaptureInputs(GetOwner());
}

void ULxGeneratedMeshRenderComponent::MarkSourceDirty()
{
 bPendingMeshBuild = true;
}

void ULxGeneratedMeshRenderComponent::FinalizeMesh()
{
 if (IsTemplate()) { return; }
 LxGeneratedMeshBuild::EndPreview(GetOwner());
 if (AActor* Owner = GetOwner(); Owner && !IsSourceCurrent())
 { Owner->OnConstruction(Owner->GetActorTransform()); }
 if(UProceduralMeshComponent* Source=Cast<UProceduralMeshComponent>(GetAttachParent()))
 { Synchronize(Source,true); }
}

void ULxGeneratedMeshRenderComponent::PreSave(FObjectPreSaveContext SaveContext)
{
#if WITH_EDITOR
 FinalizeMesh();
#endif
 Super::PreSave(SaveContext);
}

void ULxGeneratedMeshRenderComponent::Synchronize(UProceduralMeshComponent* Source, bool bFinalize)
{
 if (!Source || IsTemplate()) { return; }
 if (!bFinalize)
 {
  ++SourceUpdateCount;
  SourceInputs = LxGeneratedMeshBuild::CaptureInputs(GetOwner());
  bPendingMeshBuild = true;
 }
 if (!bEnableNanite)
 {
  SetVisibility(false);
  Source->SetVisibility(true);
  return;
 }
#if WITH_EDITOR
 if(!bFinalize && GetWorld() && !GetWorld()->IsGameWorld())
 {
  // 高频编辑只切换预览，不转换网格描述、不计算摘要，也不提交静态/Nanite 构建。
  bPendingMeshBuild=true;
  SetVisibility(false);
  Source->SetVisibility(true);
  Source->bCastHiddenShadow=false;
  return;
 }
#endif
 // 世界保存准备、组件 PreSave 和 BeginPlay 可以先后调用；已完成时不再访问顶点。
 if (!bPendingMeshBuild && (GeneratedMeshCache || CachedGeometryHash.IsEmpty()))
 {
  ApplyCachedRendering(Source);
  return;
 }
 ++GeometryConversionCount;
 FMeshDescription Description;
 FStaticMeshAttributes Attributes(Description);
 Attributes.Register();
 Attributes.GetVertexInstanceUVs().SetNumChannels(4);
 // 每个源分区独立保留材质槽，左右路缘石使用相同材质时也不能合并槽位。
 for (int32 Slot = 0; Slot < Source->GetNumSections(); ++Slot)
 {
  const FProcMeshSection* Section = Source->GetProcMeshSection(Slot);
  const FPolygonGroupID Group = Description.CreatePolygonGroup();
  Attributes.GetPolygonGroupMaterialSlotNames()[Group] = FName(*FString::Printf(TEXT("分区%d"), Slot));
  if (!Section || !Section->bSectionVisible) { continue; }
  TArray<FVertexID> Vertices;
  for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
  {
   const FVertexID Id = Description.CreateVertex();
   Attributes.GetVertexPositions()[Id] = FVector3f(Vertex.Position);
   Vertices.Add(Id);
  }
  for (int32 Index = 0; Index + 2 < Section->ProcIndexBuffer.Num(); Index += 3)
  {
   FVertexInstanceID Corners[3];
   for (int32 Corner = 0; Corner < 3; ++Corner)
   {
    const uint32 VertexIndex = Section->ProcIndexBuffer[Index + Corner];
    const FProcMeshVertex& Vertex = Section->ProcVertexBuffer[VertexIndex];
    const FVertexInstanceID Id = Description.CreateVertexInstance(Vertices[VertexIndex]);
    Corners[Corner] = Id;
    Attributes.GetVertexInstanceNormals()[Id] = FVector3f(Vertex.Normal);
    Attributes.GetVertexInstanceTangents()[Id] = FVector3f(Vertex.Tangent.TangentX);
    Attributes.GetVertexInstanceBinormalSigns()[Id] = Vertex.Tangent.bFlipTangentY ? -1.f : 1.f;
    Attributes.GetVertexInstanceColors()[Id] = FVector4f(FLinearColor(Vertex.Color));
    const FVector2D UVs[] = {Vertex.UV0, Vertex.UV1, Vertex.UV2, Vertex.UV3};
    for (int32 Channel = 0; Channel < 4; ++Channel)
    { Attributes.GetVertexInstanceUVs().Set(Id, Channel, FVector2f(UVs[Channel])); }
   }
   Description.CreateTriangle(Group, Corners);
  }
 }
 if (Description.Triangles().Num() == 0)
 {
  SetStaticMesh(nullptr);
  GeneratedMeshCache = nullptr;
  CachedGeometryHash.Reset();
  bPendingMeshBuild=false;
  SetVisibility(false);
  Source->SetVisibility(true);
  return;
 }
 const FString Hash = FString::SanitizeFloat(LxGeneratedMeshBuild::CacheVersion)
  + FStaticMeshOperations::ComputeSHAHash(Description).ToString();
 if (!GeneratedMeshCache || Hash != CachedGeometryHash)
 {
  // PIE 与打包游戏采用相同策略：游戏中修改形状立即回退，避免同步构建 Nanite 卡顿。
  if (GetWorld() && GetWorld()->IsGameWorld())
  {
   SetVisibility(false);
   Source->SetVisibility(true);
   return;
  }
#if WITH_EDITOR
  UStaticMesh* Mesh = NewObject<ULxGeneratedStaticMesh>(this, NAME_None, RF_Transactional);
  for (int32 Slot = 0; Slot < Source->GetNumSections(); ++Slot)
  {
   UMaterialInterface* Material = Source->GetMaterial(Slot);
   Mesh->GetStaticMaterials().Emplace(Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface),
    FName(*FString::Printf(TEXT("分区%d"), Slot)));
  }
  Mesh->CreateBodySetup();
  Mesh->GetBodySetup()->DefaultInstance.SetCollisionProfileName(TEXT("NoCollision"));
  if (!LxGeneratedMeshBuild::Build(Mesh, Description, true, true))
  {
   SetVisibility(false);
   Source->SetVisibility(true);
   return;
  }
  GeneratedMeshCache = Mesh;
  CachedGeometryHash = Hash;
#else
  // 打包后发生形状修改时使用原程序化网格，绝不显示失效的烹饪缓存。
  SetVisibility(false);
  Source->SetVisibility(true);
  return;
#endif
 }
 bPendingMeshBuild=false;
 ApplyCachedRendering(Source);
}

void ULxGeneratedMeshRenderComponent::ApplyCachedRendering(UProceduralMeshComponent* Source)
{
 SetStaticMesh(GeneratedMeshCache);
 if (!GeneratedMeshCache)
 {
  SetVisibility(false);
  Source->SetVisibility(true);
  return;
 }
 for (int32 Slot = 0; Slot < Source->GetNumSections(); ++Slot)
 {
  if (GetMaterial(Slot) != Source->GetMaterial(Slot)) { SetMaterial(Slot, Source->GetMaterial(Slot)); }
 }
 SetCastShadow(Source->CastShadow);
 bCastDynamicShadow = Source->bCastDynamicShadow;
 bCastContactShadow = Source->bCastContactShadow;
 bCastHiddenShadow = false;
 Source->bCastHiddenShadow = false;
 SetVisibility(true);
 Source->SetVisibility(false);
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(this);
}
