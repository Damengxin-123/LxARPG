#include "LxStoneFloorActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Algo/Reverse.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

namespace LxStoneFloor
{
 /** 将所有倒角石板直接写入一个网格描述，不为石板创建组件。 */
 struct FBuilder
 {
  /** 整板的唯一几何描述。 */
  FMeshDescription Description;
  /** 所有石板和低位接缝共用一个材质分区。 */
  FPolygonGroupID Groups[1];
  /** 当前写入的材质分区。 */
  int32 Slot = 0;
  /** 当前石板的稳定纹理偏移，同块石料六面共享。 */
  float Phase = 0;
  /** 注册属性并建立固定槽位。 */
  FBuilder()
  {
   FStaticMeshAttributes Attributes(Description);
   Attributes.Register();
   Attributes.GetVertexInstanceUVs().SetNumChannels(3);
   const FName Names[]={TEXT("粗岩石")};
   for (int32 I=0; I<1; ++I)
   {
    Groups[I]=Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Groups[I]]=Names[I];
   }
  }
  /** 保存各顶点相同的石料局部三维坐标，令正面、侧面和倒角处纹理连续。 */
  void Face(TArray<FVector> Points, const FVector& Outward, const FVector& Origin,
   const FVector& Forward, const FLinearColor& Color)
  {
   if (FVector::DotProduct(FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]),Outward)<0)
   { Algo::Reverse(Points); }
   const FVector Right(-Forward.Y,Forward.X,0);
   const FVector LocalNormal=FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]).GetSafeNormal();
   const FVector Normal=Forward*LocalNormal.X+Right*LocalNormal.Y+FVector::UpVector*LocalNormal.Z;
   const FVector Tangent=(FMath::Abs(LocalNormal.X)<0.9)?(Forward-Normal*FVector::DotProduct(Forward,Normal)).GetSafeNormal():Right;
   FStaticMeshAttributes Attributes(Description);
   TArray<FVertexInstanceID> Instances;
   for (const FVector& P : Points)
   {
    const FVertexID Vertex=Description.CreateVertex();
    Attributes.GetVertexPositions()[Vertex]=FVector3f(Origin+Forward*P.X+Right*P.Y+FVector::UpVector*P.Z);
    const FVertexInstanceID Instance=Description.CreateVertexInstance(Vertex);
    Instances.Add(Instance);
    Attributes.GetVertexInstanceNormals()[Instance]=FVector3f(Normal);
    Attributes.GetVertexInstanceTangents()[Instance]=FVector3f(Tangent);
    Attributes.GetVertexInstanceBinormalSigns()[Instance]=1;
    Attributes.GetVertexInstanceColors()[Instance]=FVector4f(Color);
    Attributes.GetVertexInstanceUVs().Set(Instance,0,FVector2f(P.X/100+Phase,P.Y/100));
    Attributes.GetVertexInstanceUVs().Set(Instance,1,FVector2f(P.Z/100,Phase));
    // 单独提供非退化的逐面展开坐标，保存资产时生成光照贴图不覆盖石纹坐标。
    const FVector Position=Origin+Forward*P.X+Right*P.Y+FVector::UpVector*P.Z;
    Attributes.GetVertexInstanceUVs().Set(Instance,2,FVector2f(FVector::DotProduct(Position,Tangent)/100,
     FVector::DotProduct(Position,FVector::CrossProduct(Normal,Tangent))/100));
   }
   for (int32 I=1; I+1<Instances.Num(); ++I)
   {
    const FVertexInstanceID Triangle[]={Instances[0],Instances[I],Instances[I+1]};
    Description.CreateTriangle(Groups[Slot],Triangle);
   }
  }
  /** 构造闭合的六面、十二条倒角边和八个倒角角面，同一块砖不按墙面拆分。 */
  void Block(const FVector& Center, const FVector& Size, const FVector& Forward,
   float Chamfer, const FLinearColor& Color)
  {
   const FVector H=Size*0.5;
   const float B=FMath::Clamp(Chamfer,0.01f,static_cast<float>(H.GetMin()*0.4));
   // 每个原始角点有三个缩进顶点，分别位于三个主面上。
   FVector Corner[8][3];
   for (int32 I=0; I<8; ++I)
   {
    for (int32 Axis=0; Axis<3; ++Axis)
    {
     for (int32 D=0; D<3; ++D) { Corner[I][Axis][D]=((I&(1<<D))?1:-1)*(H[D]-(D==Axis?0:B)); }
    }
   }
   for (int32 Axis=0; Axis<3; ++Axis)
   {
    const int32 U=(Axis+1)%3, V=(Axis+2)%3;
    for (int32 Side=0; Side<2; ++Side)
    {
     const int32 Base=Side<<Axis;
     FVector N=FVector::ZeroVector; N[Axis]=Side?1:-1;
     Face({Corner[Base][Axis],Corner[Base|(1<<U)][Axis],Corner[Base|(1<<U)|(1<<V)][Axis],Corner[Base|(1<<V)][Axis]},N,Center,Forward,Color);
    }
    // 与当前轴平行的四条倒角边。
    for (int32 SU=0; SU<2; ++SU) for (int32 SV=0; SV<2; ++SV)
    {
     const int32 A=(SU<<U)|(SV<<V), C=A|(1<<Axis);
     FVector N=FVector::ZeroVector; N[U]=SU?1:-1; N[V]=SV?1:-1;
     Face({Corner[A][U],Corner[A][V],Corner[C][V],Corner[C][U]},N.GetSafeNormal(),Center,Forward,Color);
    }
   }
   for (int32 I=0; I<8; ++I)
   {
    const FVector N((I&1)?1:-1,(I&2)?1:-1,(I&4)?1:-1);
    Face({Corner[I][0],Corner[I][1],Corner[I][2]},N.GetSafeNormal(),Center,Forward,Color);
   }
  }


 };
}

ALxStoneFloorActor::ALxStoneFloorActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 PanelMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StoneFloorMesh"));
 SetRootComponent(PanelMesh);
 PanelMesh->SetMobility(EComponentMobility::Movable);
 PanelMesh->SetCollisionProfileName(TEXT("NoCollision"));
 PanelMesh->SetCanEverAffectNavigation(false);
 PanelCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("StoneFloorCollision"));
 PanelCollision->SetupAttachment(PanelMesh);
 PanelCollision->SetMobility(EComponentMobility::Movable);
 PanelCollision->SetCollisionProfileName(TEXT("BlockAll"));
 PanelCollision->SetGenerateOverlapEvents(false);
 PanelCollision->SetVisibility(false);
 PanelCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALxStoneFloorActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 UpdatePanelIfNeeded();
}

void ALxStoneFloorActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if (PanelMesh && PanelCollision) { UpdatePanelIfNeeded(); }
}

void ALxStoneFloorActor::BeginPlay()
{
 Super::BeginPlay();
 UpdatePanelIfNeeded();
}

void ALxStoneFloorActor::RebuildFloor()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 UpdatePanelIfNeeded();
 RestorePanelRegistration();
}

TArray<double> ALxStoneFloorActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,SizeHandle.X,SizeHandle.Y,SizeHandle.Z,Thickness,TileSize,JointWidth,JointDepth,Bevel,ColorVariation,static_cast<double>(RandomSeed)};
}

void ALxStoneFloorActor::UpdatePanelIfNeeded()
{
 if (IsTemplate()) { return; }
 if (bGeometryCached && PanelMesh->GetStaticMesh() && CachedGeometrySettings==GetGeometrySettings())
 { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false;
 ++GeometryBuildCount;
 GeneratedMeshCache=nullptr;
 PanelMesh->SetStaticMesh(nullptr);
 CachedBounds=FBox(ForceInit);
 GeneratedTileCount=0; GeneratedTileSize=0; GeneratedMeshCount=0; GeneratedTriangleCount=0;
 GenerationDiagnostic.Reset();
 RefreshMaterialsAndCollision();
 for (double Value : GetGeometrySettings())
 {
  if (!FMath::IsFinite(Value)) { GenerationDiagnostic=TEXT("尺寸或外观含无效数值，未生成石板。"); return; }
 }
 SizeHandle=FVector(FMath::Clamp(SizeHandle.X,100.0,20000.0),0,0);
 Thickness=FMath::Clamp(Thickness,5.f,150.f); TileSize=FMath::Clamp(TileSize,50.f,600.f);
 JointWidth=FMath::Clamp(JointWidth,.1f,5.f); JointDepth=FMath::Clamp(JointDepth,.2f,FMath::Min(5.f,Thickness*.5f));
 Bevel=FMath::Clamp(Bevel,.1f,3.f); ColorVariation=FMath::Clamp(ColorVariation,0.f,.25f);
 const double Side=SizeHandle.X;
 const int32 Cells=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(Side/TileSize));
 if (static_cast<int64>(Cells)*Cells>1024)
 { GenerationDiagnostic=TEXT("超过1024块石板预算，请增大石板目标边长或分片铺设。"); return; }
 // 两轴均分相同数量的大方板，接缝占用内部空间，外边界精确落在指定范围。
 const double Gap=Cells>1?JointWidth:0;
 const double Actual=(Side-(Cells-1)*Gap)/Cells;
 LxStoneFloor::FBuilder Builder;
 FRandomStream Random(RandomSeed);
 if (Cells>1)
 {
  // 衬底接近外轮廓，保证窄缝中的底部实体连续，板面与侧面无共面闪烁。
  const double BedTop=-JointDepth,BedBottom=-Thickness+.1;
  Builder.Phase=0;
  Builder.Block(FVector(Side/2,Side/2,(BedTop+BedBottom)/2),FVector(Side-.2,Side-.2,BedTop-BedBottom),
   FVector::ForwardVector,.05f,FLinearColor(.46f,.48f,.50f,1));
 }
 for (int32 X=0; X<Cells; ++X) for (int32 Y=0; Y<Cells; ++Y)
 {
  Builder.Phase=Random.FRandRange(0,32);
  const float Shade=Random.FRandRange(1-ColorVariation,1);
  Builder.Block(FVector(X*(Actual+Gap)+Actual/2,Y*(Actual+Gap)+Actual/2,-Thickness/2),
   FVector(Actual,Actual,Thickness),FVector::ForwardVector,Bevel,FLinearColor(Shade,Shade,Shade,1));
  ++GeneratedTileCount;
 }
 GeneratedTileSize=Actual;
 UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
 const FName Slots[]={TEXT("粗岩石")};
 UMaterialInterface* Materials[]={StoneMaterial.Get()};
 for (int32 I=0; I<1; ++I)
 {
  Mesh->GetStaticMaterials().Emplace(Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface),Slots[I]);
#if WITH_EDITOR
  FMeshSectionInfo Info;
  Info.MaterialIndex=I; Info.bEnableCollision=false;
  Mesh->GetSectionInfoMap().Set(0,I,Info);
#endif
 }
 Mesh->CreateBodySetup();
 Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 Mesh->GetBodySetup()->DefaultInstance.SetCollisionProfileName(TEXT("NoCollision"));
 if (!LxGeneratedMeshBuild::Build(Mesh,Builder.Description,bEnableNanite))
 { GenerationDiagnostic=TEXT("整板静态网格构建失败。"); return; }
 GeneratedMeshCache=Mesh;
 PanelMesh->SetStaticMesh(Mesh);
 CachedBounds=FBox(FVector(0,0,-Thickness),FVector(Side,Side,0));
 GeneratedMeshCount=1;
 GeneratedTriangleCount=Builder.Description.Triangles().Num();
 CachedGeometrySettings=GetGeometrySettings(); bGeometryCached=true;
 RefreshMaterialsAndCollision();
}

void ALxStoneFloorActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={StoneMaterial.Get()};
 for (int32 I=0; I<1; ++I)
 {
  UMaterialInterface* Material=Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface);
  if (PanelMesh->GetMaterial(I)!=Material) { PanelMesh->SetMaterial(I,Material); }
 }
 if (CachedBounds.IsValid)
 {
  if (!PanelCollision->GetUnscaledBoxExtent().Equals(CachedBounds.GetExtent()))
  { PanelCollision->SetBoxExtent(CachedBounds.GetExtent(),false); }
  if (!PanelCollision->GetRelativeLocation().Equals(CachedBounds.GetCenter()))
  { PanelCollision->SetRelativeLocation(CachedBounds.GetCenter()); }
 }
 const ECollisionEnabled::Type Mode=bEnableCollision && CachedBounds.IsValid?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
 if (PanelCollision->GetCollisionEnabled()!=Mode) { PanelCollision->SetCollisionEnabled(Mode); }
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(PanelMesh);
}

void ALxStoneFloorActor::RestorePanelRegistration()
{
 if (!IsTemplate() && GetWorld() && !IsActorBeingDestroyed() && (!PanelMesh->IsRegistered() || !PanelCollision->IsRegistered()))
 { RegisterAllComponents(); }
}
void ALxStoneFloorActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxStoneFloorActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxStoneFloorActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 边长控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
 const bool bFloorValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerStruct()==StaticClass() && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bFloorValue)
 {
  UObject::PreEditChange(PropertyAboutToChange);
  return;
 }
 // 材质引用、继承属性与模板仍保留演员原有的完整编辑流程。
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxStoneFloorActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxStoneFloorActor,SizeHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxStoneFloorActor,SizeHandle);
 if (!IsTemplate() && (bHandle || (Event.ChangeType&EPropertyChangeType::Interactive)!=0))
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  // AActor 的默认实现会逐次注销所有组件并重跑蓝图；连续操作发送属性通知，并定时更新简化预览。
  UObject::PostEditChangeProperty(Event);
  RestorePanelRegistration();
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditChangeProperty(Event);
 UpdatePanelIfNeeded();
 RestorePanelRegistration();
}

void ALxStoneFloorActor::PostEditMove(bool bFinished)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 if (!bFinished)
 {
  if (GEngine) { GEngine->BroadcastOnActorMoving(this); }
  if (RootComponent) { RootComponent->PostEditComponentMove(false); }
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditMove(true);
 UpdatePanelIfNeeded();
}

void ALxStoneFloorActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdatePanelIfNeeded();
}
#endif

void ALxStoneFloorActor::SaveStaticMeshAsset()
{
#if WITH_EDITOR
 if (IsTemplate()) { return; }
 FText Reason;
 if (!StaticMeshAssetPath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(StaticMeshAssetPath,false,&Reason))
 {
  GenerationDiagnostic=TEXT("保存路径必须是 /Game/ 下的有效资产包路径，不能包含扩展名。")+Reason.ToString();
  return;
 }
 CancelDeferredUpdate();
 UpdatePanelIfNeeded();
 RestorePanelRegistration();
 UStaticMesh* Preview=PanelMesh->GetStaticMesh();
 if (!Preview || !Preview->GetMeshDescription(0)) { return; }
 FString PackageName=StaticMeshAssetPath;
 for (int32 Suffix=1; FPackageName::DoesPackageExist(PackageName) || FindPackage(nullptr,*PackageName); ++Suffix)
 { PackageName=FString::Printf(TEXT("%s_%d"),*StaticMeshAssetPath,Suffix); }
 UPackage* Package=CreatePackage(*PackageName);
 UStaticMesh* Asset=NewObject<UStaticMesh>(Package,*FPackageName::GetLongPackageAssetName(PackageName),RF_Public|RF_Standalone);
 Asset->InitResources();
 Asset->SetLightingGuid();
 Asset->GetStaticMaterials()=Preview->GetStaticMaterials();
 Asset->GetSectionInfoMap()=Preview->GetSectionInfoMap();
 Asset->GetOriginalSectionInfoMap()=Preview->GetSectionInfoMap();
 for (int32 Slot=0; Slot<1; ++Slot)
 { Asset->GetStaticMaterials()[Slot].MaterialInterface=PanelMesh->GetMaterial(Slot); }
 FStaticMeshSourceModel& Source=Asset->AddSourceModel();
 Source.BuildSettings.bRecomputeNormals=false;
 Source.BuildSettings.bRecomputeTangents=false;
 Source.BuildSettings.bRemoveDegenerates=false;
 Source.BuildSettings.bGenerateLightmapUVs=true;
 Source.BuildSettings.SrcLightmapIndex=2;
 Source.BuildSettings.DstLightmapIndex=3;
 Asset->SetLightMapCoordinateIndex(3);
 Asset->CreateMeshDescription(0,*Preview->GetMeshDescription(0));
 Asset->CommitMeshDescription(0);
 // 独立资产也只保存一个覆盖全板的碰撞盒。
 Asset->CreateBodySetup();
 UBodySetup* Body=Asset->GetBodySetup();
 Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 Body->DefaultInstance.SetCollisionProfileName(bEnableCollision?TEXT("BlockAll"):TEXT("NoCollision"));
 if (bEnableCollision)
 {
  for (const FBox& Bounds : {CachedBounds})
  {
   FKBoxElem Box;
   Box.Center=Bounds.GetCenter();
   const FVector Size=Bounds.GetSize();
   Box.X=Size.X; Box.Y=Size.Y; Box.Z=Size.Z;
   Body->AggGeom.BoxElems.Add(Box);
  }
 }
 Asset->SetImportVersion(EImportStaticMeshVersion::LastVersion);
 LxGeneratedMeshBuild::ConfigureNanite(Asset,bEnableNanite);
 Asset->Build(false);
 Body->CreatePhysicsMeshes();
 const FString Filename=FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension());
 IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename),true);
 FSavePackageArgs SaveArgs;
 SaveArgs.TopLevelFlags=RF_Public|RF_Standalone;
 SaveArgs.SaveFlags=SAVE_NoError;
 if (!UPackage::SavePackage(Package,Asset,*Filename,SaveArgs))
 {
  Asset->ClearFlags(RF_Public|RF_Standalone);
  GenerationDiagnostic=TEXT("保存整板静态网格失败，请检查目标目录是否可写。");
  return;
 }
 FAssetRegistryModule::AssetCreated(Asset);
 Modify();
 SavedStaticMesh=Asset;
 GenerationDiagnostic=TEXT("已保存整板静态网格：")+PackageName;
#else
 GenerationDiagnostic=TEXT("保存静态网格资产仅在编辑器中可用；运行时仍可生成整板静态网格。");
#endif
}
