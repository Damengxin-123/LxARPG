#include "LxTimberPanelActor.h"
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

namespace LxTimberPanel
{
 /** 将所有倒角木构件直接写入一个网格描述，不为木板创建组件。 */
 struct FBuilder
 {
  /** 整板的唯一几何描述。 */
  FMeshDescription Description;
  /** 两个固定材质分区：浅木板与深横梁。 */
  FPolygonGroupID Groups[2];
  /** 当前写入的材质分区。 */
  int32 Slot = 0;
  /** 当前木构件的稳定纹理偏移，同根木材六面共享。 */
  float Phase = 0;
  /** 注册属性并建立固定槽位。 */
  FBuilder()
  {
   FStaticMeshAttributes Attributes(Description);
   Attributes.Register();
   Attributes.GetVertexInstanceUVs().SetNumChannels(3);
   const FName Names[]={TEXT("木板"),TEXT("深色横梁")};
   for (int32 I=0; I<2; ++I)
   {
    Groups[I]=Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Groups[I]]=Names[I];
   }
  }
  /** 保存各顶点相同的木材局部三维坐标，令正面、侧面和倒角处纹理连续。 */
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
    // 单独提供非退化的逐面展开坐标，保存资产时生成光照贴图不覆盖木纹坐标。
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

ALxTimberPanelActor::ALxTimberPanelActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 PanelMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TimberPanelMesh"));
 SetRootComponent(PanelMesh);
 PanelMesh->SetMobility(EComponentMobility::Movable);
 PanelMesh->SetCollisionProfileName(TEXT("NoCollision"));
 PanelMesh->SetCanEverAffectNavigation(false);
 PanelCollision=CreateDefaultSubobject<UBoxComponent>(TEXT("TimberPanelCollision"));
 PanelCollision->SetupAttachment(PanelMesh);
 PanelCollision->SetMobility(EComponentMobility::Movable);
 PanelCollision->SetCollisionProfileName(TEXT("BlockAll"));
 PanelCollision->SetGenerateOverlapEvents(false);
 PanelCollision->SetVisibility(false);
 PanelCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALxTimberPanelActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 UpdatePanelIfNeeded();
}

void ALxTimberPanelActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if (PanelMesh && PanelCollision) { UpdatePanelIfNeeded(); }
}

void ALxTimberPanelActor::BeginPlay()
{
 Super::BeginPlay();
 UpdatePanelIfNeeded();
}

void ALxTimberPanelActor::RebuildPanel()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 UpdatePanelIfNeeded();
 RestorePanelRegistration();
}

TArray<double> ALxTimberPanelActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,SizeHandle.X,SizeHandle.Y,SizeHandle.Z,Thickness,bCeiling?1.0:0.0,BeamDrop,bBorder?1.0:0.0,
  BorderWidth,bAlongY?1.0:0.0,PlankLength,PlankWidth,BeamWidth,JointWidth,Bevel,ColorVariation,static_cast<double>(RandomSeed)};
}

void ALxTimberPanelActor::UpdatePanelIfNeeded()
{
 if (IsTemplate()) { return; }
 if (bGeometryCached && PanelMesh->GetStaticMesh() && CachedGeometrySettings==GetGeometrySettings())
 { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false;
 ++GeometryBuildCount;
 GeneratedMeshCache=nullptr;
 PanelMesh->SetStaticMesh(nullptr);
 CachedBounds=FBox(ForceInit);
 GeneratedPlankCount=0; GeneratedBeamCount=0; GeneratedMeshCount=0; GeneratedTriangleCount=0;
 GenerationDiagnostic.Reset();
 RefreshMaterialsAndCollision();
 for (double Value : GetGeometrySettings())
 {
  if (!FMath::IsFinite(Value)) { GenerationDiagnostic=TEXT("尺寸或外观含无效数值，未生成木板。"); return; }
 }
 SizeHandle=FVector(FMath::Clamp(SizeHandle.X,40.0,10000.0),FMath::Clamp(SizeHandle.Y,40.0,10000.0),0);
 Thickness=FMath::Clamp(Thickness,3.f,80.f); BeamDrop=FMath::Clamp(BeamDrop,0.f,60.f);
 BorderWidth=FMath::Clamp(BorderWidth,2.f,40.f);
 BeamWidth=FMath::Clamp(BeamWidth,2.f,30.f);
 PlankLength=FMath::Clamp(PlankLength,50.f,600.f); PlankWidth=FMath::Clamp(PlankWidth,8.f,80.f);
 JointWidth=FMath::Clamp(JointWidth,0.05f,1.f); Bevel=FMath::Clamp(Bevel,0.02f,0.8f);
 ColorVariation=FMath::Clamp(ColorVariation,0.f,0.2f);
 const double U=bAlongY?SizeHandle.Y:SizeHandle.X, V=bAlongY?SizeHandle.X:SizeHandle.Y;
 const double Border=bBorder?FMath::Min<double>(BorderWidth,FMath::Min(U,V)*0.2):0;
 const double InnerU=U-2*Border, InnerV=V-2*Border;
 const int32 Bays=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(InnerU/PlankLength));
 const int32 Rows=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(InnerV/PlankWidth));
 if (static_cast<int64>(Bays)*Rows>4096)
 { GenerationDiagnostic=TEXT("超过4096块木板预算，请增大单板尺寸或分块铺设。"); return; }
 const double CrossWidth=FMath::Min<double>(BeamWidth,InnerU/Bays*0.3);
 const double BayLength=(InnerU-(Bays-1)*CrossWidth)/Bays;
 const double RowWidth=InnerV/Rows;
 const double Depth=Thickness+(bCeiling && (bBorder || Bays>1)?BeamDrop:0);
 const FVector Along=bAlongY?FVector::RightVector:FVector::ForwardVector;
 const FVector Across=bAlongY?FVector::ForwardVector:FVector::RightVector;
 /** 将铺板坐标映射到始终不变的水平矩形范围。 */
 const auto Position=[&](double A,double B,double Z) { return bAlongY?FVector(B,A,Z):FVector(A,B,Z); };
 LxTimberPanel::FBuilder Builder;
 FRandomStream Random(RandomSeed);
 /** 每根木构件的纹理相位与颜色共用随机种子，尺寸相同时稳定重建。 */
 const auto AddTimber=[&](int32 Slot,const FVector& Center,const FVector& Size,const FVector& Forward,float Chamfer)
 {
  Builder.Slot=Slot;
  Builder.Phase=Random.FRandRange(0,32);
  const float Shade=Random.FRandRange(1-ColorVariation,1);
  Builder.Block(Center,Size,Forward,Chamfer,FLinearColor(Shade,Shade,Shade,1));
 };
 // 深色衬底只填补细缝，顶底面都退入木板内部，避免穿透和共面闪烁。
 AddTimber(1,FVector(SizeHandle.X/2,SizeHandle.Y/2,-Thickness/2),FVector(SizeHandle.X-0.2,SizeHandle.Y-0.2,Thickness-0.8),FVector::ForwardVector,0.04f);
 for (int32 Bay=0; Bay<Bays; ++Bay)
 {
  const double Start=Border+Bay*(BayLength+CrossWidth);
  for (int32 Row=0; Row<Rows; ++Row)
  {
   const double A=Border+Row*RowWidth+(Row>0?JointWidth/2:0);
   const double B=Border+(Row+1)*RowWidth-(Row+1<Rows?JointWidth/2:0);
   AddTimber(0,Position(Start+BayLength/2,(A+B)/2,-Thickness/2),FVector(BayLength,B-A,Thickness),Along,Bevel);
   ++GeneratedPlankCount;
  }
  if (Bay+1<Bays)
  {
   AddTimber(1,Position(Start+BayLength+CrossWidth/2,V/2,-Depth/2),FVector(InnerV,CrossWidth,Depth),Across,Bevel);
   ++GeneratedBeamCount;
  }
 }
 if (bBorder)
 {
  // 长边贯通，短边夹在长边之间；四角不叠面，深色边框封住端头纹理切换。
  for (int32 Side=0; Side<2; ++Side)
  {
   AddTimber(1,Position(U/2,Side?V-Border/2:Border/2,-Depth/2),FVector(U,Border,Depth),Along,Bevel);
   AddTimber(1,Position(Side?U-Border/2:Border/2,V/2,-Depth/2),FVector(InnerV,Border,Depth),Across,Bevel);
   GeneratedBeamCount+=2;
  }
 }
 UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
 const FName Slots[]={TEXT("木板"),TEXT("深色横梁")};
 UMaterialInterface* Materials[]={PlankMaterial.Get(),BeamMaterial.Get()};
 for (int32 I=0; I<2; ++I)
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
 CachedBounds=FBox(FVector(0,0,-Depth),FVector(SizeHandle.X,SizeHandle.Y,0));
 GeneratedMeshCount=1;
 GeneratedTriangleCount=Builder.Description.Triangles().Num();
 CachedGeometrySettings=GetGeometrySettings(); bGeometryCached=true;
 RefreshMaterialsAndCollision();
}

void ALxTimberPanelActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={PlankMaterial.Get(),BeamMaterial.Get()};
 for (int32 I=0; I<2; ++I)
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

void ALxTimberPanelActor::RestorePanelRegistration()
{
 if (!IsTemplate() && GetWorld() && !IsActorBeingDestroyed() && (!PanelMesh->IsRegistered() || !PanelCollision->IsRegistered()))
 { RegisterAllComponents(); }
}
void ALxTimberPanelActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxTimberPanelActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxTimberPanelActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 长宽控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
 const bool bWallValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerStruct()==StaticClass() && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bWallValue)
 {
  UObject::PreEditChange(PropertyAboutToChange);
  return;
 }
 // 材质引用、继承属性与模板仍保留演员原有的完整编辑流程。
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxTimberPanelActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxTimberPanelActor,SizeHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxTimberPanelActor,SizeHandle);
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

void ALxTimberPanelActor::PostEditMove(bool bFinished)
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

void ALxTimberPanelActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdatePanelIfNeeded();
}
#endif

void ALxTimberPanelActor::SaveStaticMeshAsset()
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
 for (int32 Slot=0; Slot<2; ++Slot)
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
