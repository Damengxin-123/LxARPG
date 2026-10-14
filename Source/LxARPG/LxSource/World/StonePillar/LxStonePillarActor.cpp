#include "LxStonePillarActor.h"
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


namespace LxStonePillar
{
 /** 将一块整石的连续截面直接写入单个静态网格，保留独立闭合端面。 */
 struct FBuilder
 {
  /** 整根柱子的网格描述。 */
  FMeshDescription Description;
  /** 固定底墩、柱身、顶托三个材质槽。 */
  FPolygonGroupID Groups[3];
  /** 每块石头的简单碰撞集合，共用一个物理体。 */
  FKAggregateGeom Collision;
  /** 初始化连续石纹、刻纹和独立光照展开通道。 */
  FBuilder()
  {
   FStaticMeshAttributes A(Description); A.Register();
   A.GetVertexInstanceUVs().SetNumChannels(4);
   const FName Names[]={TEXT("底墩"),TEXT("整根柱身"),TEXT("顶托")};
   for (int32 I=0; I<3; ++I)
   { Groups[I]=Description.CreatePolygonGroup(); A.GetPolygonGroupMaterialSlotNames()[Groups[I]]=Names[I]; }
  }
  /** 写入一张面；三维石纹在共享位置一致，刻纹沿周长闭合。 */
  void Face(int32 Slot,const TArray<FVector>& P,const TArray<FVector>& Normals,
   const TArray<FVector2f>& Ornament,float Phase,float Shade)
  {
   FStaticMeshAttributes A(Description);
   const FVector FaceNormal=FVector::CrossProduct(P[2]-P[0],P[1]-P[0]).GetSafeNormal();
   const FVector FaceU=(P[1]-P[0]).GetSafeNormal();
   const FVector FaceV=FVector::CrossProduct(FaceNormal,FaceU);
   TArray<FVertexInstanceID> Instances;
   for (int32 I=0; I<P.Num(); ++I)
   {
    const FVertexID V=Description.CreateVertex(); A.GetVertexPositions()[V]=FVector3f(P[I]);
    const FVertexInstanceID VI=Description.CreateVertexInstance(V); Instances.Add(VI);
    const FVector N=Normals[I];
    const FVector T=(FaceU-N*FVector::DotProduct(N,FaceU)).GetSafeNormal();
    A.GetVertexInstanceNormals()[VI]=FVector3f(N);
    A.GetVertexInstanceTangents()[VI]=FVector3f(T);
    A.GetVertexInstanceBinormalSigns()[VI]=1;
    A.GetVertexInstanceColors()[VI]=FVector4f(Shade,Shade,Shade,1);
    A.GetVertexInstanceUVs().Set(VI,0,FVector2f(P[I].X/100,P[I].Y/100));
    A.GetVertexInstanceUVs().Set(VI,1,FVector2f(P[I].Z/100,Phase));
    A.GetVertexInstanceUVs().Set(VI,2,Ornament[I]);
    A.GetVertexInstanceUVs().Set(VI,3,FVector2f(FVector::DotProduct(P[I],FaceU)/100,FVector::DotProduct(P[I],FaceV)/100));
   }
   for (int32 I=1; I+1<Instances.Num(); ++I)
   { const FVertexInstanceID T[]={Instances[0],Instances[I],Instances[I+1]}; Description.CreateTriangle(Groups[Slot],T); }
  }
  /** 沿高度扫掠完整石料轮廓；轮廓只形成倒角与收放，不产生额外石块。 */
  void Stone(int32 Slot,const TArray<FVector2D>& Profile,double Radius,double Bevel,
   bool bRound,int32 Segments,double Flute,float Phase,float Shade)
  {
   const int32 N=bRound?Segments:8;
   const double Start=Profile[0].X, Height=Profile.Last().X-Start;
   TArray<TArray<FVector>> Rings;
   double MaxRadius=0;
   for (const FVector2D& Sample : Profile)
   {
    const double R=Radius*Sample.Y, B=FMath::Min(Bevel,R*.2);
    MaxRadius=FMath::Max(MaxRadius,R);
    TArray<FVector> Ring;
    if (bRound)
    {
     const double Fade=FMath::Clamp(FMath::Min(Sample.X-Start,Start+Height-Sample.X)/(Height*.06),0.0,1.0);
     for (int32 I=0; I<N; ++I)
     {
      const double Angle=2*PI*I/N;
      const double Actual=R-Flute*FMath::Pow(.5+.5*FMath::Cos(Angle*12),3)*Fade;
      Ring.Add(FVector(Actual*FMath::Cos(Angle),Actual*FMath::Sin(Angle),Sample.X));
     }
    }
    else
    {
     Ring={FVector(R,-R+B,Sample.X),FVector(R,R-B,Sample.X),FVector(R-B,R,Sample.X),FVector(-R+B,R,Sample.X),
      FVector(-R,R-B,Sample.X),FVector(-R,-R+B,Sample.X),FVector(-R+B,-R,Sample.X),FVector(R-B,-R,Sample.X)};
    }
    Rings.Add(MoveTemp(Ring));
   }
   // 刻纹沿真实周长展开，方柱倒角不会拉伸成与主面同宽。
   TArray<double> U; U.Add(0);
   for (int32 I=0; I<N; ++I) { U.Add(U.Last()+FVector::Distance(Rings[0][I],Rings[0][(I+1)%N])); }
   const double Perimeter=U.Last(); for (double& Value : U) { Value/=Perimeter; }
   for (int32 R=0; R+1<Rings.Num(); ++R)
   {
    const float V0=(Profile[R].X-Start)/Height,V1=(Profile[R+1].X-Start)/Height;
    for (int32 I=0; I<N; ++I)
    {
     const int32 J=(I+1)%N, Prev=(I+N-1)%N, Next=(I+2)%N;
     const FVector A=Rings[R][I],B=Rings[R+1][I],C=Rings[R+1][J],D=Rings[R][J];
     const FVector Flat=FVector::CrossProduct(C-A,B-A).GetSafeNormal();
     const FVector NI=bRound?FVector::CrossProduct(Rings[R][J]-Rings[R][Prev],B-A).GetSafeNormal():Flat;
     const FVector NJ=bRound?FVector::CrossProduct(Rings[R][Next]-Rings[R][I],C-D).GetSafeNormal():Flat;
     Face(Slot,{A,B,C,D},{NI,NI,NJ,NJ},
      {FVector2f(U[I],V0),FVector2f(U[I],V1),FVector2f(U[I+1],V1),FVector2f(U[I+1],V0)},Phase,Shade);
    }
   }
   // 底面使用正序，顶面反序，遵循 UE 的三角形朝向。
   for (int32 Side=0; Side<2; ++Side)
   {
    const TArray<FVector>& Ring=Side?Rings.Last():Rings[0];
    const FVector Normal(0,0,Side?1:-1),Center(0,0,Ring[0].Z);
    for (int32 I=0; I<N; ++I)
    {
     const FVector A=Ring[I],B=Ring[(I+1)%N];
     Face(Slot,Side?TArray<FVector>{Center,B,A}:TArray<FVector>{Center,A,B},
      {Normal,Normal,Normal},{FVector2f(0,Side),FVector2f(0,Side),FVector2f(0,Side)},Phase,Shade);
    }
   }
   if (bRound)
   {
    // 一个凸包对应一块石料，柱身碰撞半径不会被加宽端部放大。
    FKConvexElem Convex;
    for (const FVector2D& Sample : Profile)
    {
     for (int32 I=0; I<24; ++I)
     {
      const double Angle=2*PI*(I+.5)/24;
      const double R=Radius*Sample.Y/FMath::Cos(PI/24);
      Convex.VertexData.Add(FVector(R*FMath::Cos(Angle),R*FMath::Sin(Angle),Sample.X));
     }
    }
    Convex.UpdateElemBox(); Collision.ConvexElems.Add(MoveTemp(Convex));
   }
   else
   {
    FKBoxElem Box; Box.Center=FVector(0,0,Start+Height/2);
    Box.X=MaxRadius*2; Box.Y=MaxRadius*2; Box.Z=Height; Collision.BoxElems.Add(Box);
   }
  }
 };
}

ALxStonePillarActor::ALxStonePillarActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 PillarMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StonePillarMesh"));
 SetRootComponent(PillarMesh);
 PillarMesh->SetMobility(EComponentMobility::Movable);
 PillarMesh->SetCollisionProfileName(TEXT("BlockAll"));
 PillarMesh->SetGenerateOverlapEvents(false);
}

void ALxStonePillarActor::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); UpdatePillarIfNeeded(); }
void ALxStonePillarActor::PostRegisterAllComponents()
{ Super::PostRegisterAllComponents(); if (PillarMesh) { UpdatePillarIfNeeded(); } }
void ALxStonePillarActor::BeginPlay()
{ Super::BeginPlay(); UpdatePillarIfNeeded(); }
void ALxStonePillarActor::RebuildPillar()
{ CancelDeferredUpdate(); bGeometryCached=false; UpdatePillarIfNeeded(); RestorePillarRegistration(); }

TArray<double> ALxStonePillarActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,HeightHandle.X,HeightHandle.Y,HeightHandle.Z,Radius,static_cast<double>(Shape),bBase?1.0:0.0,bCapital?1.0:0.0,
  BaseHeight,CapitalHeight,BaseScale,CapitalScale,JointWidth,Bevel,FluteDepth,static_cast<double>(RoundSegments),static_cast<double>(RandomSeed)};
}

void ALxStonePillarActor::UpdatePillarIfNeeded()
{
 if (IsTemplate()) { return; }
 if (bGeometryCached && PillarMesh->GetStaticMesh() && CachedGeometrySettings==GetGeometrySettings())
 { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false; ++GeometryBuildCount;
 GeneratedMeshCache=nullptr;
 PillarMesh->SetStaticMesh(nullptr);
 GeneratedStoneCount=0; GeneratedMeshCount=0; GeneratedTriangleCount=0; GeneratedShaftHeight=0;
 GenerationDiagnostic.Reset(); RefreshMaterialsAndCollision();
 for (double Value : GetGeometrySettings())
 { if (!FMath::IsFinite(Value)) { GenerationDiagnostic=TEXT("柱子参数包含无效数值，未生成网格。"); return; } }
 HeightHandle=FVector(0,0,FMath::Clamp(HeightHandle.Z,40.0,3000.0));
 Radius=FMath::Clamp(Radius,5.f,200.f); BaseHeight=FMath::Clamp(BaseHeight,10.f,160.f); CapitalHeight=FMath::Clamp(CapitalHeight,10.f,160.f);
 BaseScale=FMath::Clamp(BaseScale,1.1f,2.2f); CapitalScale=FMath::Clamp(CapitalScale,1.1f,2.2f);
 JointWidth=FMath::Clamp(JointWidth,.1f,2.f); Bevel=FMath::Clamp(Bevel,.1f,4.f); FluteDepth=FMath::Clamp(FluteDepth,0.f,FMath::Min(2.f,Radius*.15f));
 RoundSegments=FMath::Clamp(RoundSegments,24,128);
 const double H=HeightHandle.Z, Requested=(bBase?BaseHeight:0)+(bCapital?CapitalHeight:0);
 const double Ratio=Requested>0?FMath::Min(1.0,(H-20)/Requested):1;
 const double BH=bBase?BaseHeight*Ratio:0,CH=bCapital?CapitalHeight*Ratio:0;
 const double Low=bBase?BH+JointWidth/2:0,High=bCapital?H-CH-JointWidth/2:H;
 const double Shaft=High-Low, Edge=FMath::Min<double>(Bevel,Shaft*.025);
 const double EndRadius=1-FMath::Min<double>(Bevel,Radius*.2)/Radius;
 LxStonePillar::FBuilder Builder; FRandomStream Random(RandomSeed);
 /** 各石块各用一个稳定相位，柱身不会被分段染色或添加横向砖缝。 */
 const auto Add=[&](int32 Slot,const TArray<FVector2D>& Profile,double Flute)
 {
  Builder.Stone(Slot,Profile,Radius,Bevel,Shape==ELxPillarShape::Round,LxGeneratedMeshBuild::IsPreview(this)?12:RoundSegments,LxGeneratedMeshBuild::IsPreview(this)?0:Flute,
   Random.FRandRange(0,32),Random.FRandRange(.96f,1.f));
  ++GeneratedStoneCount;
 };
 if (bBase)
 {
  const double B=BH-JointWidth/2;
  Add(0,{{0,BaseScale-.035},{B*.06,BaseScale},{B*.64,BaseScale},{B*.78,1+(BaseScale-1)*.65},{B*.94,1.035},{B,1.02}},0);
 }
 Add(1,{{Low,EndRadius},{Low+Edge,1},{Low+Shaft*.06,1},{High-Shaft*.06,1},{High-Edge,1},{High,EndRadius}},FluteDepth);
 if (bCapital)
 {
  const double Start=H-CH+JointWidth/2,C=H-Start;
  Add(2,{{Start,1.02},{Start+C*.08,1.035},{Start+C*.3,1+(CapitalScale-1)*.38},
   {Start+C*.66,CapitalScale},{H-C*.06,CapitalScale},{H,CapitalScale-.035}},0);
 }
 UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
 const FName Slots[]={TEXT("底墩"),TEXT("整根柱身"),TEXT("顶托")};
 UMaterialInterface* Materials[]={EndMaterial.Get(),ShaftMaterial.Get(),EndMaterial.Get()};
 int32 Section=0;
 for (int32 I=0; I<3; ++I)
 {
  Mesh->GetStaticMaterials().Emplace(Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface),Slots[I]);
#if WITH_EDITOR
  if ((I==0 && !bBase) || (I==2 && !bCapital)) { continue; }
  FMeshSectionInfo Info; Info.MaterialIndex=I; Info.bEnableCollision=true;
  Mesh->GetSectionInfoMap().Set(0,Section++,Info);
#endif
 }
 Mesh->CreateBodySetup(); Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 if (!LxGeneratedMeshBuild::Build(Mesh,Builder.Description,bEnableNanite))
 { GeneratedStoneCount=0; GenerationDiagnostic=TEXT("石柱静态网格构建失败。"); return; }
 UBodySetup* Body=Mesh->GetBodySetup();
 Body->AggGeom=MoveTemp(Builder.Collision); Body->DefaultInstance.SetCollisionProfileName(TEXT("BlockAll"));
 Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes();
 GeneratedMeshCache=Mesh;
 PillarMesh->SetStaticMesh(Mesh);
 GeneratedMeshCount=1; GeneratedTriangleCount=Builder.Description.Triangles().Num(); GeneratedShaftHeight=Shaft;
 CachedGeometrySettings=GetGeometrySettings(); bGeometryCached=true;
 RefreshMaterialsAndCollision();
}

void ALxStonePillarActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={EndMaterial.Get(),ShaftMaterial.Get(),EndMaterial.Get()};
 for (int32 I=0; I<3; ++I)
 {
  UMaterialInterface* M=Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface);
  if (PillarMesh->GetMaterial(I)!=M) { PillarMesh->SetMaterial(I,M); }
 }
 const ECollisionEnabled::Type Mode=bEnableCollision && PillarMesh->GetStaticMesh()?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
 if (PillarMesh->GetCollisionEnabled()!=Mode) { PillarMesh->SetCollisionEnabled(Mode); }
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(PillarMesh);
}

void ALxStonePillarActor::RestorePillarRegistration()
{
 if (!IsTemplate() && GetWorld() && !IsActorBeingDestroyed() && !PillarMesh->IsRegistered()) { RegisterAllComponents(); }
}
void ALxStonePillarActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxStonePillarActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxStonePillarActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 高度控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
 const bool bPillarValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerStruct()==StaticClass() && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bPillarValue)
 {
  UObject::PreEditChange(PropertyAboutToChange);
  return;
 }
 // 材质引用、继承属性与模板仍保留演员原有的完整编辑流程。
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxStonePillarActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxStonePillarActor,HeightHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxStonePillarActor,HeightHandle);
 if (!IsTemplate() && (bHandle || (Event.ChangeType&EPropertyChangeType::Interactive)!=0))
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  // AActor 的默认实现会逐次注销所有组件并重跑蓝图；连续操作发送属性通知，并定时更新简化预览。
  UObject::PostEditChangeProperty(Event);
  RestorePillarRegistration();
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditChangeProperty(Event);
 UpdatePillarIfNeeded();
 RestorePillarRegistration();
}

void ALxStonePillarActor::PostEditMove(bool bFinished)
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
 UpdatePillarIfNeeded();
}

void ALxStonePillarActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdatePillarIfNeeded();
}
#endif

void ALxStonePillarActor::SaveStaticMeshAsset()
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
 UpdatePillarIfNeeded();
 RestorePillarRegistration();
 UStaticMesh* Preview=PillarMesh->GetStaticMesh();
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
 { Asset->GetStaticMaterials()[Slot].MaterialInterface=PillarMesh->GetMaterial(Slot); }
 FStaticMeshSourceModel& Source=Asset->AddSourceModel();
 Source.BuildSettings.bRecomputeNormals=false;
 Source.BuildSettings.bRecomputeTangents=false;
 Source.BuildSettings.bRemoveDegenerates=false;
 Source.BuildSettings.bGenerateLightmapUVs=true;
 Source.BuildSettings.SrcLightmapIndex=3;
 Source.BuildSettings.DstLightmapIndex=4;
 Asset->SetLightMapCoordinateIndex(4);
 // 完整资产构建会保留空多边形组；导出前移除关闭端部留下的空组，避免多余渲染分区。
 FMeshDescription ExportDescription(*Preview->GetMeshDescription(0));
 TArray<FPolygonGroupID> EmptyGroups;
 for (FPolygonGroupID Group : ExportDescription.PolygonGroups().GetElementIDs())
 { if (ExportDescription.GetNumPolygonGroupPolygons(Group)==0) { EmptyGroups.Add(Group); } }
 for (FPolygonGroupID Group : EmptyGroups) { ExportDescription.DeletePolygonGroup(Group); }
 FElementIDRemappings Remappings;
 ExportDescription.Compact(Remappings);
 Asset->CreateMeshDescription(0,ExportDescription);
 Asset->CommitMeshDescription(0);
 // 三块整石的简单形状共用唯一物理体，不复制三角面碰撞。
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
  GenerationDiagnostic=TEXT("保存石柱静态网格失败，请检查目标目录是否可写。");
  return;
 }
 FAssetRegistryModule::AssetCreated(Asset);
 Modify();
 SavedStaticMesh=Asset;
 GenerationDiagnostic=TEXT("已保存石柱静态网格：")+PackageName;
#else
 GenerationDiagnostic=TEXT("保存静态网格资产仅在编辑器中可用；运行时仍可生成石柱静态网格。");
#endif
}
