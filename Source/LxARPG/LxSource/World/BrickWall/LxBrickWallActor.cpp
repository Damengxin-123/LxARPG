#include "LxBrickWallActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Algo/Reverse.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

namespace LxBrickWall
{
 /** 单个材质槽的几何缓冲。 */
 struct FMesh
 {
  /** 分面顶点位置与显式外向法线。 */
  TArray<FVector> Vertices, Normals;
  /** 备用材质使用的平面纹理坐标。 */
  TArray<FVector2D> UV;
  /** 每块石砖共享的颜色与积尘权重。 */
  TArray<FLinearColor> Colors;
  /** 与纹理横轴一致的切线。 */
  TArray<FProcMeshTangent> Tangents;
  /** 朝向实体外部的三角形索引。 */
  TArray<int32> Indices;
  /** 当前墙段的砂浆实心范围，仅裁去完全埋入其中的砖面。 */
  FBox SolidCore = FBox(ForceInit);

  /** 上传一个平面凸多边形，复用顶点并采用最少的扇形三角形。 */
  void EmitPolygon(const TArray<FVector>& Input, const FVector& Normal, const FLinearColor& Color)
  {
   TArray<FVector> Points;
   for (const FVector& P : Input)
   { if (Points.IsEmpty() || !Points.Last().Equals(P,1.e-7)) { Points.Add(P); } }
   if (Points.Num()>1 && Points[0].Equals(Points.Last(),1.e-7)) { Points.Pop(EAllowShrinking::No); }
   if (Points.Num()<3) { return; }
   const FVector T=(Points[1]-Points[0]).GetSafeNormal();
   if (T.IsNearlyZero()) { return; }
   const FVector V=FVector::CrossProduct(Normal,T);
   const int32 Base=Vertices.Num();
   for (const FVector& P : Points)
   {
    Vertices.Add(P); Normals.Add(Normal); Tangents.Emplace(T,false); Colors.Add(Color);
    UV.Emplace(FVector::DotProduct(P,T)/100,FVector::DotProduct(P,V)/100);
   }
   for (int32 I=1; I+1<Points.Num(); ++I)
   {
    if (FVector::CrossProduct(Points[I]-Points[0],Points[I+1]-Points[0]).SizeSquared()>1.e-12)
    { Indices.Append({Base,Base+I,Base+I+1}); }
   }
  }

  /** 从多边形扣除砂浆盒内部；保留接缝两侧的浅槽壁，避免斜视时穿帮。 */
  void ClipAndEmit(const TArray<FVector>& Points, const FVector& Normal, const FLinearColor& Color)
  {
   const FBox Bounds(Points);
   if (!SolidCore.IsValid || !SolidCore.Intersect(Bounds)) { EmitPolygon(Points,Normal,Color); return; }
   if (SolidCore.IsInsideOrOn(Bounds.Min) && SolidCore.IsInsideOrOn(Bounds.Max)) { return; }
   TArray<FVector> Remaining=Points;
   for (int32 Axis=0; Axis<3 && Remaining.Num()>=3; ++Axis)
   {
    for (int32 Side=0; Side<2 && Remaining.Num()>=3; ++Side)
    {
     TArray<FVector> Outside, Inside;
     /** 正距离位于当前裁切平面的外部。 */
     const auto Distance = [&](const FVector& P) { return Side?P[Axis]-SolidCore.Max[Axis]:SolidCore.Min[Axis]-P[Axis]; };
     for (int32 I=0; I<Remaining.Num(); ++I)
     {
      const FVector A=Remaining[I], B=Remaining[(I+1)%Remaining.Num()];
      const double DA=Distance(A), DB=Distance(B);
      if (DA>0) { Outside.Add(A); } else { Inside.Add(A); }
      if ((DA>0)!=(DB>0))
      {
       const FVector Cut=FMath::Lerp(A,B,DA/(DA-DB));
       Outside.Add(Cut); Inside.Add(Cut);
      }
     }
     EmitPolygon(Outside,Normal,Color);
     Remaining=MoveTemp(Inside);
    }
   }
  }

  /** 添加多边形；自动朝外绕序，中心起伏不改变共用的砖边。 */
  void Face(TArray<FVector> Points, const FVector& Outward, const FVector& Origin,
   const FVector& Forward, const FLinearColor& Color, float Relief)
  {
   const FVector Right(-Forward.Y, Forward.X, 0);
   /** 将局部砖坐标转换为墙体坐标。 */
   const auto Convert = [&](const FVector& P) { return Origin + Forward*P.X + Right*P.Y + FVector::UpVector*P.Z; };
   FVector LocalCenter=FVector::ZeroVector;
   for (const FVector& P : Points) { LocalCenter+=P; }
   LocalCenter/=Points.Num();
   // 埋入砂浆的中心起伏不会影响可见轮廓，无需为它额外细分三角形。
   if (SolidCore.IsValid && SolidCore.IsInside(Convert(LocalCenter+Outward*Relief))) { Relief=0; }
   if (FVector::DotProduct(FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]),Outward)<0)
   { Algo::Reverse(Points); }
   if (Relief<=UE_SMALL_NUMBER)
   {
    for (FVector& P : Points) { P=Convert(P); }
    ClipAndEmit(Points,FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]).GetSafeNormal(),Color);
    return;
   }
   FVector Center = FVector::ZeroVector;
   for (const FVector& P : Points) { Center += P; }
   Center = Center / Points.Num() + Outward * Relief;
   for (int32 I=0; I<Points.Num(); ++I)
   {
    FVector A=Center, B=Points[I], C=Points[(I+1)%Points.Num()];
    if (FVector::DotProduct(FVector::CrossProduct(C-A,B-A),Outward)<0) { Swap(B,C); }
    A=Convert(A); B=Convert(B); C=Convert(C);
    const FVector N=FVector::CrossProduct(C-A,B-A).GetSafeNormal();
    ClipAndEmit({A,B,C},N,Color);
   }
  }

  /** 构造闭合的六面、十二条倒角边和八个倒角角面，同一块砖不按墙面拆分。 */
  void Block(const FVector& Center, const FVector& Size, const FVector& Forward,
   float Chamfer, float Relief, const FLinearColor& Color)
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
     Face({Corner[Base][Axis],Corner[Base|(1<<U)][Axis],Corner[Base|(1<<U)|(1<<V)][Axis],Corner[Base|(1<<V)][Axis]},N,Center,Forward,Color,Relief);
    }
    // 与当前轴平行的四条倒角边。
    for (int32 SU=0; SU<2; ++SU) for (int32 SV=0; SV<2; ++SV)
    {
     const int32 A=(SU<<U)|(SV<<V), C=A|(1<<Axis);
     FVector N=FVector::ZeroVector; N[U]=SU?1:-1; N[V]=SV?1:-1;
     Face({Corner[A][U],Corner[A][V],Corner[C][V],Corner[C][U]},N.GetSafeNormal(),Center,Forward,Color,0);
    }
   }
   for (int32 I=0; I<8; ++I)
   {
    const FVector N((I&1)?1:-1,(I&2)?1:-1,(I&4)?1:-1);
    Face({Corner[I][0],Corner[I][1],Corner[I][2]},N.GetSafeNormal(),Center,Forward,Color,0);
   }
  }

  /** 直接追加到同一网格描述，保留逐面法线、切线、颜色和纹理坐标。 */
  void AppendTo(FMeshDescription& Description, const FName SlotName) const
  {
   if (Indices.IsEmpty()) { return; }
   FStaticMeshAttributes Attributes(Description);
   const FPolygonGroupID Group=Description.CreatePolygonGroup();
   Attributes.GetPolygonGroupMaterialSlotNames()[Group]=SlotName;
   TArray<FVertexInstanceID> Instances;
   Instances.Reserve(Vertices.Num());
   Description.ReserveNewVertices(Vertices.Num());
   Description.ReserveNewVertexInstances(Vertices.Num());
   Description.ReserveNewTriangles(Indices.Num()/3);
   for (int32 I=0; I<Vertices.Num(); ++I)
   {
    const FVertexID Vertex=Description.CreateVertex();
    Attributes.GetVertexPositions()[Vertex]=FVector3f(Vertices[I]);
    const FVertexInstanceID Instance=Description.CreateVertexInstance(Vertex);
    Instances.Add(Instance);
    Attributes.GetVertexInstanceNormals()[Instance]=FVector3f(Normals[I]);
    Attributes.GetVertexInstanceTangents()[Instance]=FVector3f(Tangents[I].TangentX);
    Attributes.GetVertexInstanceBinormalSigns()[Instance]=Tangents[I].bFlipTangentY?-1.f:1.f;
    Attributes.GetVertexInstanceColors()[Instance]=FVector4f(Colors[I]);
    Attributes.GetVertexInstanceUVs().Set(Instance,0,FVector2f(UV[I]));
   }
   for (int32 I=0; I<Indices.Num(); I+=3)
   {
    const FVertexInstanceID Triangle[]={Instances[Indices[I]],Instances[Indices[I+1]],Instances[Indices[I+2]]};
    Description.CreateTriangle(Group,Triangle);
   }
  }

  /** 上传单个槽，不为每块石砖单独创建组件或物理体。 */
  void Submit(UProceduralMeshComponent* Mesh, int32 Slot, UMaterialInterface* Material)
  {
   if (Vertices.IsEmpty()) { return; }
   Mesh->CreateMeshSection_LinearColor(Slot,Vertices,Indices,Normals,UV,Colors,Tangents,false);
   Mesh->SetMaterial(Slot,Material ? Material : UMaterial::GetDefaultMaterial(MD_Surface));
  }
 };

 /** 将直墙或直角侧墙的尺寸转换为演员局部轴对齐包围盒。 */
 FBox MakeBounds(const FVector& Center, const FVector& Size, const FVector& Forward)
 {
  FBox Bounds(ForceInit);
  const FVector Right(-Forward.Y,Forward.X,0);
  for (int32 I=0; I<8; ++I)
  {
   Bounds+=Center+Forward*((I&1)?Size.X/2:-Size.X/2)+Right*((I&2)?Size.Y/2:-Size.Y/2)+FVector::UpVector*((I&4)?Size.Z/2:-Size.Z/2);
  }
  return Bounds;
 }
 /** 判断连接是否需要侧向出口。 */
 bool IsCorner(ELxWallConnection Connection) { return Connection==ELxWallConnection::Left || Connection==ELxWallConnection::Right; }
}

ALxBrickWallActor::ALxBrickWallActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 WallMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("BrickWallMesh"));
 SetRootComponent(WallMesh);
 WallMesh->bUseComplexAsSimpleCollision=false;
 WallMesh->bUseAsyncCooking=false;
 WallMesh->SetCollisionProfileName(TEXT("NoCollision"));
 WallMesh->SetCanEverAffectNavigation(false);
 WallMesh->SetMobility(EComponentMobility::Movable);
 StaticWallMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MergedWallMesh"));
 StaticWallMesh->SetupAttachment(WallMesh);
 StaticWallMesh->SetMobility(EComponentMobility::Movable);
 StaticWallMesh->SetCollisionProfileName(TEXT("NoCollision"));
 StaticWallMesh->SetCanEverAffectNavigation(false);
 for (int32 I=0; I<5; ++I)
 {
  UBoxComponent* Box=CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("WallCollision%d"),I));
  Box->SetupAttachment(WallMesh);
  Box->SetMobility(EComponentMobility::Movable);
  Box->SetCollisionProfileName(TEXT("BlockAll"));
  Box->SetGenerateOverlapEvents(false);
  Box->SetVisibility(false);
  Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  CollisionBoxes.Add(Box);
 }
}

void ALxBrickWallActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 UpdateWallIfNeeded();
}

void ALxBrickWallActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if (WallMesh && StaticWallMesh) { UpdateWallIfNeeded(); }
}

void ALxBrickWallActor::BeginPlay()
{
 Super::BeginPlay();
 UpdateWallIfNeeded();
}

void ALxBrickWallActor::RebuildWall()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 UpdateWallIfNeeded();
 RestoreWallRegistration();
}

void ALxBrickWallActor::SaveStaticMeshAsset()
{
#if WITH_EDITOR
 if (IsTemplate()) { return; }
 FText Reason;
 if (!StaticMeshAssetPath.StartsWith(TEXT("/Game/")) || !FPackageName::IsValidLongPackageName(StaticMeshAssetPath,false,&Reason))
 {
  GenerationDiagnostic=TEXT("保存路径必须是 /Game/ 下的有效资产包路径，不能包含扩展名。")+Reason.ToString();
  return;
 }
 bGenerateStaticMesh=true;
 CancelDeferredUpdate();
 UpdateWallIfNeeded();
 RestoreWallRegistration();
 UStaticMesh* Preview=StaticWallMesh->GetStaticMesh();
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
 for (int32 Slot=0; Slot<3; ++Slot)
 { Asset->GetStaticMaterials()[Slot].MaterialInterface=StaticWallMesh->GetMaterial(Slot); }
 FStaticMeshSourceModel& Source=Asset->AddSourceModel();
 Source.BuildSettings.bRecomputeNormals=false;
 Source.BuildSettings.bRecomputeTangents=false;
 Source.BuildSettings.bRemoveDegenerates=false;
 Source.BuildSettings.bGenerateLightmapUVs=true;
 Source.BuildSettings.SrcLightmapIndex=0;
 Source.BuildSettings.DstLightmapIndex=1;
 Asset->SetLightMapCoordinateIndex(1);
 Asset->CreateMeshDescription(0,*Preview->GetMeshDescription(0));
 Asset->CommitMeshDescription(0);
 // 把主墙、柱体和侧墙的独立盒形碰撞一起存入资产，避免总包围盒堵住直角内侧。
 Asset->CreateBodySetup();
 UBodySetup* Body=Asset->GetBodySetup();
 Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 Body->DefaultInstance.SetCollisionProfileName(bEnableCollision?TEXT("BlockAll"):TEXT("NoCollision"));
 if (bEnableCollision)
 {
  for (const FBox& Bounds : CachedCollisionBounds)
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
  GenerationDiagnostic=TEXT("保存整墙静态网格失败，请检查目标目录是否可写。");
  return;
 }
 FAssetRegistryModule::AssetCreated(Asset);
 Modify();
 SavedStaticMesh=Asset;
 GenerationDiagnostic=TEXT("已保存整墙静态网格：")+PackageName;
#else
 GenerationDiagnostic=TEXT("保存静态网格资产仅在编辑器中可用；运行时仍可生成整墙静态网格。");
#endif
}

TArray<double> ALxBrickWallActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,LengthHandle.X,LengthHandle.Y,LengthHandle.Z,WallWidth,WallHeight,ConnectionOverhang,
  StartReturnLength,EndReturnLength,BrickLength,BrickHeight,JointWidth,JointDepth,Bevel,SurfaceRelief,
  ColorVariation,Weathering,static_cast<double>(RandomSeed),static_cast<double>(StartConnection),
  static_cast<double>(EndConnection),bCullInternalFaces?1.0:0.0,bGenerateStaticMesh?1.0:0.0};
}

void ALxBrickWallActor::UpdateWallIfNeeded()
{
 using namespace LxBrickWall;
 if (IsTemplate()) { return; }
 if (bGeometryCached && (bGenerateStaticMesh?StaticWallMesh->GetStaticMesh()!=nullptr:WallMesh->GetNumSections()>0)
  && CachedGeometrySettings==GetGeometrySettings())
 { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false;
 ++GeometryBuildCount;
 GeneratedMeshCache=nullptr;
 // 仅迁移旧版本已经保存的凸碰撞；新版本渲染组件不再保存任何碰撞形状。
 WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 if (WallMesh->GetBodySetup()->AggGeom.ConvexElems.Num()>0) { WallMesh->ClearCollisionConvexMeshes(); }
 WallMesh->ClearAllMeshSections();
 StaticWallMesh->SetStaticMesh(nullptr);
 CachedCollisionBounds.Reset();
 RefreshMaterialsAndCollision();
 GeneratedMeshCount=0; GeneratedBrickCount=0; GeneratedTriangleCount=0; GenerationDiagnostic.Reset();
 if (LengthHandle.ContainsNaN()) { GenerationDiagnostic=TEXT("长度手柄包含无效数值。"); return; }
 for (float Value : {WallWidth,WallHeight,ConnectionOverhang,StartReturnLength,EndReturnLength,BrickLength,BrickHeight,JointWidth,JointDepth,Bevel,SurfaceRelief,ColorVariation,Weathering})
 {
  if (!FMath::IsFinite(Value)) { GenerationDiagnostic=TEXT("尺寸或外观包含无效数值。"); return; }
 }
 // 蓝图调用同样执行安全范围约束，不能只依赖编辑器的 ClampMin。
 LengthHandle=FVector(FMath::Clamp(LengthHandle.X,20.0,10000.0),0,0);
 WallWidth=FMath::Clamp(WallWidth,10.f,500.f); WallHeight=FMath::Clamp(WallHeight,20.f,3000.f);
 ConnectionOverhang=FMath::Clamp(ConnectionOverhang,2.f,100.f);
 StartReturnLength=FMath::Clamp(StartReturnLength,0.f,10000.f); EndReturnLength=FMath::Clamp(EndReturnLength,0.f,10000.f);
 BrickLength=FMath::Clamp(BrickLength,15.f,200.f); BrickHeight=FMath::Clamp(BrickHeight,8.f,100.f);
 JointWidth=FMath::Clamp(JointWidth,0.2f,5.f); JointDepth=FMath::Clamp(JointDepth,0.2f,FMath::Min(5.f,WallWidth*0.2f));
 Bevel=FMath::Clamp(Bevel,0.1f,3.f); SurfaceRelief=FMath::Clamp(SurfaceRelief,0.f,1.5f);
 ColorVariation=FMath::Clamp(ColorVariation,0.f,0.3f); Weathering=FMath::Clamp(Weathering,0.f,0.5f);
 const float Length=LengthHandle.X, PillarWidth=WallWidth+2*ConnectionOverhang;
 const int32 Rows=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(WallHeight/BrickHeight));
 const float RowHeight=WallHeight/Rows;
 const float Gap=FMath::Min(JointWidth,RowHeight*0.2f);
 const float TotalLength=Length+(IsCorner(StartConnection)?StartReturnLength:0)+(IsCorner(EndConnection)?EndReturnLength:0);
 if ((FMath::CeilToDouble(TotalLength/BrickLength)+10)*Rows>4000)
 { GenerationDiagnostic=TEXT("超过4000块石砖预算，请缩短墙体或增大砖块尺寸。"); return; }
 FMesh Stone, Connection, Mortar;
 FRandomStream Random(RandomSeed);
 /** 逐块赋予贯穿各面的统一色差，并仅在靠地处增加克制的积尘。 */
 const auto ColorAt = [&](float Z)
 {
  const float Shade=0.92f+Random.FRandRange(-ColorVariation,ColorVariation);
  const float Dirt=Weathering*FMath::Square(1-FMath::Clamp(Z/80.f,0.f,1.f));
  return FLinearColor(Shade*(1-Dirt),Shade*(1-Dirt*0.94f),Shade*(1-Dirt*0.8f),1);
 };
 /** 砌筑一个直线墙段；首尾裁切为实体砖，所有可见面使用同一个闭合网格。 */
 const auto BuildRun = [&](const FVector& Start, const FVector& Forward, float RunLength)
 {
  if (RunLength<1) { return; }
  const FVector Center=Start+Forward*(RunLength/2)+FVector(0,0,WallHeight/2);
  // 裸露端面也必须缩进，防止砂浆端盖与石砖端面共面闪烁。
  const FVector CoreSize(RunLength-2*FMath::Min(JointDepth,RunLength*0.2f),WallWidth-2*JointDepth,WallHeight-Gap);
  Mortar.Block(Center,CoreSize,Forward,0.1f,0,FLinearColor::White);
  // 砂浆也有倒角，裁切盒退入倒角内侧，保证不会删除仍能从砖缝看见的区域。
  Stone.SolidCore=bCullInternalFaces?MakeBounds(Center,CoreSize-FVector(0.22),Forward):FBox(ForceInit);
  CachedCollisionBounds.Add(MakeBounds(Center,FVector(RunLength,WallWidth,WallHeight),Forward));
  for (int32 Row=0; Row<Rows; ++Row)
  {
   // 等分一行后错半砖，杜绝任意长度末端产生极薄残砖。
   const int32 Columns=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(RunLength/BrickLength));
   const float Pitch=RunLength/Columns;
   const float Offset=(Row%2 && Columns>1)?Pitch/2:0;
   for (int32 Column=0; Column<Columns+(Offset>0?1:0); ++Column)
   {
    const float A=FMath::Max(0.f,Column*Pitch-Offset), B=FMath::Min(RunLength,(Column+1)*Pitch-Offset);
    const float LocalGap=FMath::Min(Gap,(B-A)*0.2f);
    const float Left=A+(A>0?LocalGap/2:0), Right=B-(B<RunLength?LocalGap/2:0);
    const float Bottom=Row*RowHeight+(Row>0?Gap/2:0), Top=(Row+1)*RowHeight-(Row+1<Rows?Gap/2:0);
    const float Relief=Random.FRandRange(0,SurfaceRelief);
    const FVector Size(Right-Left,WallWidth+Relief,Top-Bottom);
    Stone.Block(Start+Forward*((Left+Right)/2)+FVector(0,0,(Bottom+Top)/2),Size,Forward,Bevel,Relief*0.2f,ColorAt((Bottom+Top)/2));
    ++GeneratedBrickCount;
   }
  }
 };
 BuildRun(FVector::ZeroVector,FVector::ForwardVector,Length);
 for (int32 End=0; End<2; ++End)
 {
  const ELxWallConnection Mode=End?EndConnection:StartConnection;
  if (Mode==ELxWallConnection::None) { continue; }
  const FVector Center(End?Length+PillarWidth/2:-PillarWidth/2,0,WallHeight/2);
  const FVector Size(PillarWidth,PillarWidth,WallHeight);
  Mortar.Block(Center,Size-FVector(JointDepth*2,JointDepth*2,Gap),FVector::ForwardVector,0.1f,0,FLinearColor::White);
  Connection.SolidCore=bCullInternalFaces?MakeBounds(Center,Size-FVector(JointDepth*2+0.22f,JointDepth*2+0.22f,Gap+0.22f),FVector::ForwardVector):FBox(ForceInit);
  CachedCollisionBounds.Add(MakeBounds(Center,Size,FVector::ForwardVector));
  for (int32 Row=0; Row<Rows; ++Row)
  {
   const float Bottom=Row*RowHeight+(Row>0?Gap/2:0), Top=(Row+1)*RowHeight-(Row+1<Rows?Gap/2:0);
   Connection.Block(FVector(Center.X,0,(Bottom+Top)/2),FVector(PillarWidth,PillarWidth,Top-Bottom),FVector::ForwardVector,Bevel*1.3f,0,ColorAt((Bottom+Top)/2));
   ++GeneratedBrickCount;
  }
  if (IsCorner(Mode))
  {
   const FVector Direction(0,Mode==ELxWallConnection::Left?-1:1,0);
   BuildRun(FVector(Center.X,0,0)+Direction*(PillarWidth/2),Direction,End?EndReturnLength:StartReturnLength);
  }
 }
 if (bGenerateStaticMesh)
 {
  FMeshDescription Description;
  FStaticMeshAttributes Attributes(Description);
  Attributes.Register();
  Attributes.GetVertexInstanceUVs().SetNumChannels(1);
  const FName Slots[]={TEXT("石砖"),TEXT("连接石"),TEXT("砂浆")};
  Stone.AppendTo(Description,Slots[0]); Connection.AppendTo(Description,Slots[1]); Mortar.AppendTo(Description,Slots[2]);
  // 每次形状变化只创建一个临时静态网格；不修改仍被渲染线程使用的旧资源。
  UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
  UMaterialInterface* Materials[]={WallMaterial.Get(),ConnectionMaterial.Get(),MortarMaterial.Get()};
  for (int32 Slot=0; Slot<3; ++Slot)
  {
   Mesh->GetStaticMaterials().Emplace(Materials[Slot]?Materials[Slot]:UMaterial::GetDefaultMaterial(MD_Surface),Slots[Slot]);
  }
  Mesh->CreateBodySetup();
  Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
  Mesh->GetBodySetup()->DefaultInstance.SetCollisionProfileName(TEXT("NoCollision"));
#if WITH_EDITOR
  // UE 编辑器初始化资源时会按分区表覆盖材质索引，必须提前保留无柱墙的砂浆槽 2。
  int32 SectionIndex=0;
  for (const FPolygonGroupID Group : Description.PolygonGroups().GetElementIDs())
  {
   FMeshSectionInfo Info;
   Info.MaterialIndex=Mesh->GetMaterialIndex(Attributes.GetPolygonGroupMaterialSlotNames()[Group]);
   Info.bEnableCollision=false;
   Mesh->GetSectionInfoMap().Set(0,SectionIndex++,Info);
  }
#endif
  if (!LxGeneratedMeshBuild::Build(Mesh,Description,bEnableNanite))
  {
   GenerationDiagnostic=TEXT("整墙静态网格构建失败。");
   CachedCollisionBounds.Reset();
   RefreshMaterialsAndCollision();
   return;
  }
  GeneratedMeshCache=Mesh;
 StaticWallMesh->SetStaticMesh(Mesh);
 }
 else
 {
  Stone.Submit(WallMesh,0,WallMaterial); Connection.Submit(WallMesh,1,ConnectionMaterial); Mortar.Submit(WallMesh,2,MortarMaterial);
 }
 GeneratedMeshCount=1;
 GeneratedTriangleCount=(Stone.Indices.Num()+Connection.Indices.Num()+Mortar.Indices.Num())/3;
 CachedGeometrySettings=GetGeometrySettings();
 bGeometryCached=true;
 RefreshMaterialsAndCollision();
}

void ALxBrickWallActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={WallMaterial.Get(),ConnectionMaterial.Get(),MortarMaterial.Get()};
 for (int32 Slot=0; Slot<3; ++Slot)
 {
  UMaterialInterface* Material=Materials[Slot]?Materials[Slot]:UMaterial::GetDefaultMaterial(MD_Surface);
  if (WallMesh->GetMaterial(Slot)!=Material) { WallMesh->SetMaterial(Slot,Material); }
  if (StaticWallMesh->GetMaterial(Slot)!=Material) { StaticWallMesh->SetMaterial(Slot,Material); }
 }
 for (int32 I=0; I<CollisionBoxes.Num(); ++I)
 {
  UBoxComponent* Box=CollisionBoxes[I];
  const bool bActive=bEnableCollision && CachedCollisionBounds.IsValidIndex(I);
  if (bActive)
  {
   const FBox& Bounds=CachedCollisionBounds[I];
   if (!Box->GetUnscaledBoxExtent().Equals(Bounds.GetExtent())) { Box->SetBoxExtent(Bounds.GetExtent(),false); }
   if (!Box->GetRelativeLocation().Equals(Bounds.GetCenter())) { Box->SetRelativeLocation(Bounds.GetCenter()); }
  }
  const ECollisionEnabled::Type Mode=bActive?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
  if (Box->GetCollisionEnabled()!=Mode) { Box->SetCollisionEnabled(Mode); }
 }
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(StaticWallMesh);
}

void ALxBrickWallActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxBrickWallActor::RestoreWallRegistration()
{
 if (IsTemplate() || !GetWorld() || IsActorBeingDestroyed()) { return; }
 if (!WallMesh->IsRegistered() || !StaticWallMesh->IsRegistered()) { RegisterAllComponents(); return; }
 for (const UBoxComponent* Box : CollisionBoxes)
 {
  if (Box && !Box->IsRegistered()) { RegisterAllComponents(); return; }
 }
}

void ALxBrickWallActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxBrickWallActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 长度控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
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

void ALxBrickWallActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxBrickWallActor,LengthHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxBrickWallActor,LengthHandle);
 if (!IsTemplate() && (bHandle || (Event.ChangeType&EPropertyChangeType::Interactive)!=0))
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  // AActor 的默认实现会逐次注销所有组件并重跑蓝图；连续操作发送属性通知，并定时更新简化预览。
  UObject::PostEditChangeProperty(Event);
  RestoreWallRegistration();
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditChangeProperty(Event);
 UpdateWallIfNeeded();
 RestoreWallRegistration();
}

void ALxBrickWallActor::PostEditMove(bool bFinished)
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
 UpdateWallIfNeeded();
}

void ALxBrickWallActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdateWallIfNeeded();
}
#endif

FTransform ALxBrickWallActor::GetConnectionTransform(bool bEnd) const
{
 using namespace LxBrickWall;
 const ELxWallConnection Mode=bEnd?EndConnection:StartConnection;
 const float Width=WallWidth+2*ConnectionOverhang;
 FVector Direction(bEnd?1:-1,0,0);
 FVector Location(bEnd?LengthHandle.X:0,0,0);
 if (Mode!=ELxWallConnection::None)
 {
  Location.X+=Direction.X*Width/2;
  if (IsCorner(Mode))
  {
   Direction=FVector(0,Mode==ELxWallConnection::Left?-1:1,0);
   Location+=Direction*(Width/2+(bEnd?EndReturnLength:StartReturnLength));
  }
  else { Location+=Direction*Width/2; }
 }
 return FTransform(Direction.Rotation(),Location)*GetActorTransform();
}
