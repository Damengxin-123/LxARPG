#include "LxTileRoofActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Algo/Reverse.h"
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

namespace LxTileRoof
{
 /** 将所有倒角屋顶构件直接写入一个网格描述，不为屋顶构件创建组件。 */
 struct FBuilder
 {
  /** 整板的唯一几何描述。 */
  FMeshDescription Description;
  /** 瓦片和石沿使用两个固定材质分区。 */
  FPolygonGroupID Groups[2];
  /** 当前写入的材质分区。 */
  int32 Slot = 0;
  /** 当前屋顶构件的稳定纹理偏移，同块石料六面共享。 */
  float Phase = 0;
  /** 当前构件沿本地 Y 的高度变化率，瓦片含搭接形成的额外倾斜。 */
  double Shear = 0;
  /** 第二坡面绕进深中央镜像，修正绕序以保持朝外法线。 */
  bool bMirror = false;
  /** 整个双坡屋顶的水平进深。 */
  double FullDepth = 0;
  /** 注册属性并建立固定槽位。 */
  FBuilder()
  {
   FStaticMeshAttributes Attributes(Description);
   Attributes.Register();
   Attributes.GetVertexInstanceUVs().SetNumChannels(3);
   const FName Names[]={TEXT("蓝灰瓦片"),TEXT("灰白石沿")};
   for (int32 I=0; I<2; ++I)
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
   /** 把局部闭合方块变为保持水平投影范围的斜面构件。 */
   const auto Map=[&](const FVector& P)
   { FVector Q=Origin+FVector(P.X,P.Y,P.Z+Shear*P.Y); if (bMirror) { Q.Y=FullDepth-Q.Y; } return Q; };
   if (bMirror) { Algo::Reverse(Points); }
   const FVector Normal=FVector::CrossProduct(Map(Points[2])-Map(Points[0]),Map(Points[1])-Map(Points[0])).GetSafeNormal();
   const FVector Tangent=(Map(Points[1])-Map(Points[0])).GetSafeNormal();
   FStaticMeshAttributes Attributes(Description);
   TArray<FVertexInstanceID> Instances;
   for (const FVector& P : Points)
   {
    const FVertexID Vertex=Description.CreateVertex();
    Attributes.GetVertexPositions()[Vertex]=FVector3f(Map(P));
    const FVertexInstanceID Instance=Description.CreateVertexInstance(Vertex);
    Instances.Add(Instance);
    Attributes.GetVertexInstanceNormals()[Instance]=FVector3f(Normal);
    Attributes.GetVertexInstanceTangents()[Instance]=FVector3f(Tangent);
    Attributes.GetVertexInstanceBinormalSigns()[Instance]=1;
    Attributes.GetVertexInstanceColors()[Instance]=FVector4f(Color);
    Attributes.GetVertexInstanceUVs().Set(Instance,0,FVector2f(P.X/100+Phase,P.Y/100));
    Attributes.GetVertexInstanceUVs().Set(Instance,1,FVector2f(P.Z/100,Phase));
    // 单独提供非退化的逐面展开坐标，保存资产时生成光照贴图不覆盖石纹坐标。
    const FVector Position=Map(P);
    Attributes.GetVertexInstanceUVs().Set(Instance,2,FVector2f(FVector::DotProduct(Position,Tangent)/100,
     FVector::DotProduct(Position,FVector::CrossProduct(Normal,Tangent))/100));
   }
   for (int32 I=1; I+1<Instances.Num(); ++I)
   {
    const FVertexInstanceID Triangle[]={Instances[0],Instances[I],Instances[I+1]};
    Description.CreateTriangle(Groups[Slot],Triangle);
   }
  }
  /** 构建平底斜顶的封闭侧墙，全部表面沿用同一块石料坐标。 */
  void SideWall(double X0,double X1,double Run,double Bottom,double Slope,double TopOffset,const FLinearColor& Color)
  {
   Shear=0;
   const FVector A(X0,0,Bottom),B(X1,0,Bottom),C(X1,Run,Bottom),D(X0,Run,Bottom);
   const FVector E(X0,0,TopOffset),F(X1,0,TopOffset),G(X1,Run,Run*Slope+TopOffset),H(X0,Run,Run*Slope+TopOffset);
   const FVector O=FVector::ZeroVector,T=FVector::ForwardVector;
   Face({A,B,C,D},FVector(0,0,-1),O,T,Color);
   Face({E,F,G,H},FVector(0,-Slope,1),O,T,Color);
   Face({A,D,H,E},FVector(-1,0,0),O,T,Color);
   Face({B,C,G,F},FVector(1,0,0),O,T,Color);
   Face({A,B,F,E},FVector(0,-1,0),O,T,Color);
   Face({D,C,G,H},FVector(0,1,0),O,T,Color);
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


ALxTileRoofActor::ALxTileRoofActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 RoofMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TileRoofMesh"));
 SetRootComponent(RoofMesh); RoofMesh->SetMobility(EComponentMobility::Movable);
 RoofMesh->SetCollisionProfileName(TEXT("BlockAll")); RoofMesh->SetGenerateOverlapEvents(false);
}
void ALxTileRoofActor::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); UpdateRoofIfNeeded(); }
void ALxTileRoofActor::PostRegisterAllComponents()
{ Super::PostRegisterAllComponents(); if (RoofMesh) { UpdateRoofIfNeeded(); } }
void ALxTileRoofActor::BeginPlay()
{ Super::BeginPlay(); UpdateRoofIfNeeded(); }
void ALxTileRoofActor::RebuildRoof()
{ CancelDeferredUpdate(); bGeometryCached=false; UpdateRoofIfNeeded(); RestoreRoofRegistration(); }
TArray<double> ALxTileRoofActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,static_cast<double>(RoofType),bCloseSides?1.0:0.0,SideWallInset,SideWallDrop,RidgeHeight,SizeHandle.X,SizeHandle.Y,SizeHandle.Z,SlopeAngle,BackingThickness,TileWidth,TileLength,Overlap,TileThickness,JointWidth,
  bLeftStone?1.0:0.0,bRightStone?1.0:0.0,bTopStone?1.0:0.0,EdgeWidth,EdgeHeight,TopDepth,TopHeight,StoneLength,ColorVariation,static_cast<double>(RandomSeed)};
}
void ALxTileRoofActor::UpdateRoofIfNeeded()
{
 if (IsTemplate()) { return; }
 if (bGeometryCached && RoofMesh->GetStaticMesh() && CachedGeometrySettings==GetGeometrySettings())
 { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false; ++GeometryBuildCount;
 GeneratedMeshCache=nullptr; RoofMesh->SetStaticMesh(nullptr);
 GeneratedTileCount=0; GeneratedRowCount=0; GeneratedStoneCount=0; GeneratedMeshCount=0; GeneratedTriangleCount=0;
 GenerationDiagnostic.Reset(); RefreshMaterialsAndCollision();
 for (double V : GetGeometrySettings())
 { if (!FMath::IsFinite(V)) { GenerationDiagnostic=TEXT("屋顶尺寸或外观包含无效数值，未生成。"); return; } }
 SizeHandle=FVector(FMath::Clamp(SizeHandle.X,150.0,20000.0),FMath::Clamp(SizeHandle.Y,150.0,20000.0),0);
 SlopeAngle=FMath::Clamp(SlopeAngle,10.f,50.f); BackingThickness=FMath::Clamp(BackingThickness,1.f,12.f);
 TileWidth=FMath::Clamp(TileWidth,20.f,100.f); TileLength=FMath::Clamp(TileLength,30.f,120.f);
 Overlap=FMath::Clamp(Overlap,2.f,FMath::Min(25.f,TileLength*.35f)); TileThickness=FMath::Clamp(TileThickness,.8f,5.f);
 JointWidth=FMath::Clamp(JointWidth,.1f,1.f); EdgeWidth=FMath::Clamp(EdgeWidth,8.f,50.f); EdgeHeight=FMath::Clamp(EdgeHeight,8.f,35.f);
 TopDepth=FMath::Clamp(TopDepth,12.f,60.f); TopHeight=FMath::Clamp(TopHeight,20.f,100.f);
 StoneLength=FMath::Clamp(StoneLength,40.f,160.f); ColorVariation=FMath::Clamp(ColorVariation,0.f,.2f);
 SideWallInset=FMath::Clamp(SideWallInset,1.f,FMath::Min(20.f,EdgeWidth*.4f));
 SideWallDrop=FMath::Clamp(SideWallDrop,0.f,500.f); RidgeHeight=FMath::Clamp(RidgeHeight,6.f,40.f);
 const bool bGable=RoofType==ELxRoofType::Gable;
 const int32 Slopes=bGable?2:1;
 const double W=SizeHandle.X,FullD=SizeHandle.Y,D=FullD/Slopes,Slope=FMath::Tan(FMath::DegreesToRadians(SlopeAngle));
 const double Cos=FMath::Cos(FMath::DegreesToRadians(SlopeAngle));
 const double Left=bLeftStone?EdgeWidth-2:0,Right=bRightStone?EdgeWidth-2:0;
 const double TileRun=D-(bTopStone?(bGable?TopDepth/2:TopDepth)-2:0),TileSpan=W-Left-Right;
 const int32 Cols=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(TileSpan/TileWidth));
 const int32 Rows=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(TileRun/((TileLength-Overlap)*Cos)));
 if (static_cast<int64>(Rows)*Cols*Slopes>4096)
 { GenerationDiagnostic=TEXT("超过4096片瓦预算，请增大瓦片尺寸或分段铺设屋顶。"); return; }
 const double Pitch=TileRun/Rows,TileW=(TileSpan-(Cols-1)*JointWidth)/Cols;
 const double Lap=FMath::Min<double>(Overlap*Cos,Pitch*.35);
 const double T=FMath::Min<double>(TileThickness,Pitch*Slope*.65);
 const double LiftRate=(T+.03)/Pitch,MaxTileTop=T+LiftRate*(Pitch+Lap);
 // 石沿自动高于瓦片的搭接前缘。
 const double StoneBottom=T*.55,StoneTop=FMath::Max<double>(EdgeHeight,MaxTileTop+3);
 LxTileRoof::FBuilder Builder; Builder.FullDepth=FullD; FRandomStream Random(RandomSeed);
 FKAggregateGeom Collision;
 /** 按构件局部三维坐标保留连续石纹，每块构件只使用一个稳定相位。 */
 const auto Add=[&](int32 Slot,const FVector& Center,const FVector& Size,double Shear,float Bevel,float ShadeScale=1.f)
 {
  Builder.Slot=Slot; Builder.Shear=Shear; Builder.Phase=Random.FRandRange(0,32);
  const float Shade=ShadeScale*Random.FRandRange(1-ColorVariation,1);
  Builder.Block(Center,Size,FVector::ForwardVector,Bevel,FLinearColor(Shade,Shade,Shade,1));
 };
 for (int32 Plane=0; Plane<Slopes; ++Plane)
 {
 Builder.bMirror=Plane==1;
 // 背衬端面退入石沿，避免与中央屋脊外端面共面闪烁。
 const double BackLeft=(bLeftStone || bTopStone)?.5:0,BackRight=(bRightStone || bTopStone)?.5:0;
 Add(0,FVector((BackLeft+W-BackRight)/2,D/2,D*Slope/2-BackingThickness/2),FVector(W-BackLeft-BackRight,D,BackingThickness),Slope,.1f,.65f);
 for (int32 Row=0; Row<Rows; ++Row)
 {
  const double Start=Row*Pitch,End=FMath::Min(TileRun,Start+Pitch+Lap),Length=End-Start;
  const double Mid=(Start+End)/2;
  for (int32 Col=0; Col<Cols; ++Col)
  {
   Add(0,FVector(Left+Col*(TileW+JointWidth)+TileW/2,Mid,Mid*Slope+T/2+LiftRate*Length/2),
    FVector(TileW,Length,T),Slope-LiftRate,.28f);
   ++GeneratedTileCount;
  }
 }
 const double SideRun=D-(bTopStone?(bGable?TopDepth/2:TopDepth):0);
 for (int32 Side=0; Side<2; ++Side)
 {
  if (Side?!bRightStone:!bLeftStone) { continue; }
  const int32 Segments=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(SideRun/(StoneLength*Cos)));
  const double Step=SideRun/Segments;
  for (int32 I=0; I<Segments; ++I)
  {
   const double A=I*Step+(I>0?.18:0),B=(I+1)*Step-(I+1<Segments?.18:0),Y=(A+B)/2;
   Add(1,FVector(Side?W-EdgeWidth/2:EdgeWidth/2,Y,Y*Slope+(StoneTop+StoneBottom)/2),
    FVector(EdgeWidth,B-A,StoneTop-StoneBottom),Slope,.65f);
   ++GeneratedStoneCount;
  }
 }
 const double TopBottom=(D-TopDepth)*Slope-BackingThickness,CapThickness=6,Top=D*Slope+TopHeight;
 if (bTopStone && !bGable)
 {
  const int32 Segments=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(W/StoneLength));
  const double Step=W/Segments;
  for (int32 I=0; I<Segments; ++I)
  {
   const double A=I*Step+(I>0?.18:0),B=(I+1)*Step-(I+1<Segments?.18:0);
   Add(1,FVector((A+B)/2,D-TopDepth/2,(TopBottom+Top-CapThickness)/2),FVector(B-A,TopDepth,Top-CapThickness-TopBottom),0,.5f);
   Add(1,FVector((A+B)/2,D-(TopDepth+4)/2,Top-CapThickness/2),FVector(B-A,TopDepth+4,CapThickness),0,.6f);
   GeneratedStoneCount+=2;
  }
 }
 // 墙身藏在石沿下方，外面与内面均缩进；双坡两块墙身在屋脊下无缝相接。
 if (bCloseSides)
 {
  for (int32 Side=0; Side<2; ++Side)
  {
   if (Side?!bRightStone:!bLeftStone) { continue; }
   const double X0=(Side?W-EdgeWidth:0)+SideWallInset,X1=(Side?W:EdgeWidth)-SideWallInset;
   const double Bottom=-BackingThickness-SideWallDrop;
   Builder.Slot=1; Builder.Phase=Random.FRandRange(0,32);
   Builder.SideWall(X0,X1,D,Bottom,Slope,StoneBottom,FLinearColor(.96f,.96f,.96f,1));
   ++GeneratedStoneCount;
   FKConvexElem Wall;
   for (double X : {X0,X1}) for (double Y : {0.0,D})
   {
    const double MappedY=Builder.bMirror?FullD-Y:Y;
    Wall.VertexData.Add(FVector(X,MappedY,Bottom));
    Wall.VertexData.Add(FVector(X,MappedY,Y*Slope+StoneBottom));
   }
   Wall.UpdateElemBox(); Collision.ConvexElems.Add(MoveTemp(Wall));
  }
 }
 /** 斜薄板只占屋面厚度，禁止用整屋外包围盒封住下方空间。 */
 const auto AddSlopeCollision=[&](double X0,double X1,double Y0,double Y1,double Z0,double Z1)
 {
  FKConvexElem Convex;
  for (double X : {X0,X1}) for (double Y : {Y0,Y1}) for (double Z : {Z0,Z1})
  { Convex.VertexData.Add(FVector(X,Builder.bMirror?FullD-Y:Y,Y*Slope+Z)); }
  Convex.UpdateElemBox(); Collision.ConvexElems.Add(MoveTemp(Convex));
 };
 AddSlopeCollision(0,W,0,D,-BackingThickness,MaxTileTop);
 if (bLeftStone) { AddSlopeCollision(0,EdgeWidth,0,SideRun,StoneBottom,StoneTop); }
 if (bRightStone) { AddSlopeCollision(W-EdgeWidth,W,0,SideRun,StoneBottom,StoneTop); }
 if (bTopStone && !bGable)
 {
  FKBoxElem Box; Box.Center=FVector(W/2,D-(TopDepth+4)/2,(TopBottom+Top)/2);
  Box.X=W; Box.Y=TopDepth+4; Box.Z=Top-TopBottom; Collision.BoxElems.Add(Box);
 }
 }
 // 双坡只生成一条中央屋脊，覆盖两面瓦片收口，不叠放两道单坡高墙。
 if (bGable && bTopStone)
 {
  Builder.bMirror=false;
  const double Bottom=D*Slope-TopDepth*.5*Slope,Top=D*Slope+RidgeHeight;
  const int32 Segments=LxGeneratedMeshBuild::IsPreview(this)?1:FMath::Max(1,FMath::RoundToInt(W/StoneLength));
  const double Step=W/Segments;
  for (int32 I=0; I<Segments; ++I)
  {
   const double A=I*Step+(I>0?.18:0),B=(I+1)*Step-(I+1<Segments?.18:0);
   Add(1,FVector((A+B)/2,D,(Bottom+Top)/2),FVector(B-A,TopDepth,Top-Bottom),0,.65f);
   ++GeneratedStoneCount;
  }
  FKBoxElem Ridge; Ridge.Center=FVector(W/2,D,(Bottom+Top)/2);
  Ridge.X=W; Ridge.Y=TopDepth; Ridge.Z=Top-Bottom; Collision.BoxElems.Add(Ridge);
 }
 UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
 const FName Slots[]={TEXT("蓝灰瓦片"),TEXT("灰白石沿")};
 UMaterialInterface* Materials[]={TileMaterial.Get(),StoneMaterial.Get()};
 for (int32 I=0; I<2; ++I)
 {
  Mesh->GetStaticMaterials().Emplace(Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface),Slots[I]);
#if WITH_EDITOR
  if (I==0 || GeneratedStoneCount>0)
  { FMeshSectionInfo Info; Info.MaterialIndex=I; Info.bEnableCollision=true; Mesh->GetSectionInfoMap().Set(0,I,Info); }
#endif
 }
 Mesh->CreateBodySetup(); Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 if (!LxGeneratedMeshBuild::Build(Mesh,Builder.Description,bEnableNanite))
 { GeneratedTileCount=0; GeneratedStoneCount=0; GenerationDiagnostic=TEXT("屋顶静态网格构建失败。"); return; }
 UBodySetup* Body=Mesh->GetBodySetup(); Body->AggGeom=MoveTemp(Collision);
 Body->DefaultInstance.SetCollisionProfileName(TEXT("BlockAll")); Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes();
 GeneratedMeshCache=Mesh;
 RoofMesh->SetStaticMesh(Mesh); GeneratedRowCount=Rows*Slopes; GeneratedMeshCount=1;
 GeneratedTriangleCount=Builder.Description.Triangles().Num();
 CachedGeometrySettings=GetGeometrySettings(); bGeometryCached=true; RefreshMaterialsAndCollision();
}
void ALxTileRoofActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={TileMaterial.Get(),StoneMaterial.Get()};
 for (int32 I=0; I<2; ++I)
 {
  UMaterialInterface* M=Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface);
  if (RoofMesh->GetMaterial(I)!=M) { RoofMesh->SetMaterial(I,M); }
 }
 const ECollisionEnabled::Type Mode=bEnableCollision && RoofMesh->GetStaticMesh()?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
 if (RoofMesh->GetCollisionEnabled()!=Mode) { RoofMesh->SetCollisionEnabled(Mode); }
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(RoofMesh);
}
void ALxTileRoofActor::RestoreRoofRegistration()
{
 if (!IsTemplate() && GetWorld() && !IsActorBeingDestroyed() && !RoofMesh->IsRegistered()) { RegisterAllComponents(); }
}
void ALxTileRoofActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxTileRoofActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxTileRoofActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 宽深控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
 const bool bRoofValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerStruct()==StaticClass() && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bRoofValue)
 {
  UObject::PreEditChange(PropertyAboutToChange);
  return;
 }
 // 材质引用、继承属性与模板仍保留演员原有的完整编辑流程。
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxTileRoofActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxTileRoofActor,SizeHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxTileRoofActor,SizeHandle);
 if (!IsTemplate() && (bHandle || (Event.ChangeType&EPropertyChangeType::Interactive)!=0))
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  // AActor 的默认实现会逐次注销所有组件并重跑蓝图；连续操作发送属性通知，并定时更新简化预览。
  UObject::PostEditChangeProperty(Event);
  RestoreRoofRegistration();
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditChangeProperty(Event);
 UpdateRoofIfNeeded();
 RestoreRoofRegistration();
}

void ALxTileRoofActor::PostEditMove(bool bFinished)
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
 UpdateRoofIfNeeded();
}

void ALxTileRoofActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdateRoofIfNeeded();
}
#endif

void ALxTileRoofActor::SaveStaticMeshAsset()
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
 UpdateRoofIfNeeded();
 RestoreRoofRegistration();
 UStaticMesh* Preview=RoofMesh->GetStaticMesh();
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
 { Asset->GetStaticMaterials()[Slot].MaterialInterface=RoofMesh->GetMaterial(Slot); }
 FStaticMeshSourceModel& Source=Asset->AddSourceModel();
 Source.BuildSettings.bRecomputeNormals=false;
 Source.BuildSettings.bRecomputeTangents=false;
 Source.BuildSettings.bRemoveDegenerates=false;
 Source.BuildSettings.bGenerateLightmapUVs=true;
 Source.BuildSettings.SrcLightmapIndex=2;
 Source.BuildSettings.DstLightmapIndex=3;
 Asset->SetLightMapCoordinateIndex(3);
 // 完整资产构建会保留空多边形组；导出前移除关闭石沿留下的空组，避免多余渲染分区。
 FMeshDescription ExportDescription(*Preview->GetMeshDescription(0));
 TArray<FPolygonGroupID> EmptyGroups;
 for (FPolygonGroupID Group : ExportDescription.PolygonGroups().GetElementIDs())
 { if (ExportDescription.GetNumPolygonGroupPolygons(Group)==0) { EmptyGroups.Add(Group); } }
 for (FPolygonGroupID Group : EmptyGroups) { ExportDescription.DeletePolygonGroup(Group); }
 FElementIDRemappings Remappings;
 ExportDescription.Compact(Remappings);
 Asset->CreateMeshDescription(0,ExportDescription);
 Asset->CommitMeshDescription(0);
 // 屋面与石沿的简单形状共用唯一物理体，不复制三角面碰撞。
 Asset->CreateBodySetup();
 UBodySetup* Body=Asset->GetBodySetup();
 Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 Body->DefaultInstance.SetCollisionProfileName(bEnableCollision?TEXT("BlockAll"):TEXT("NoCollision"));
 Body->AggGeom=Preview->GetBodySetup()->AggGeom;
 Body->InvalidatePhysicsData();
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
  GenerationDiagnostic=TEXT("保存屋顶静态网格失败，请检查目标目录是否可写。");
  return;
 }
 FAssetRegistryModule::AssetCreated(Asset);
 Modify();
 SavedStaticMesh=Asset;
 GenerationDiagnostic=TEXT("已保存屋顶静态网格：")+PackageName;
#else
 GenerationDiagnostic=TEXT("保存静态网格资产仅在编辑器中可用；运行时仍可生成屋顶静态网格。");
#endif
}
