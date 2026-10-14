#include "LxStoneTerraceActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxStoneTerraceHandleComponent.h"
#include "../SplineRoad/LxSplineRoadActor.h"
#include "../SplineRoad/LxRoadConnectionSolver.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Algo/Reverse.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

namespace LxTerrace
{
 /** 返回逆时针矩形角点，原点位于底面中心。 */
 TArray<FVector> Corners(const ALxStoneTerraceActor& A)
 {
  const double X=FMath::Max(100.,A.Size.X)*.5,Y=FMath::Max(100.,A.Size.Y)*.5,Z=FMath::Max(1.f,A.TerraceHeight);
  return {{-X,-Y,Z},{X,-Y,Z},{X,Y,Z},{-X,Y,Z}};
 }
 /** 有限参数检测，避免脚本传入非法数值导致无限细分。 */
 bool Valid(const ALxStoneTerraceActor& A)
 {
  return !A.Size.ContainsNaN() && !A.BrickSize.ContainsNaN() && FMath::IsFinite(A.TerraceHeight)
   && FMath::IsFinite(A.CornerRadius) && FMath::IsFinite(A.BorderWidth) && FMath::IsFinite(A.BorderHeight)
   && FMath::IsFinite(A.BorderLength) && FMath::IsFinite(A.JointWidth)
   && FMath::IsFinite(A.BorderBevelMeters) && FMath::IsFinite(A.AutoConnectDistance) && FMath::IsFinite(A.AutoConnectHeight);
 }
 /** 连接只支持单位缩放及水平旋转；独立台地仍允许整体变换。 */
 bool Eligible(const ALxStoneTerraceActor& A)
 {
  return Valid(A) && A.bAutoConnect && A.GetActorScale3D().Equals(FVector::OneVector,.001)
   && FVector::DotProduct(A.GetActorUpVector(),FVector::UpVector)>.9999;
 }
 /** 检测新开口是否与已经占用的边段重叠。 */
 bool Free(const ALxStoneTerraceActor& A,int32 Side,double Low,double High)
 {
  return !A.Connections.ContainsByPredicate([&](const FLxTerraceOpening& C)
   { return C.Side==Side && Low<C.Interval.Y-.01 && High>C.Interval.X+.01; });
 }
 /** 接缝到达角点时取消对应圆角，保证两台地之间没有缺口。 */
 TArray<FVector> Outline(const ALxStoneTerraceActor& A)
 {
  const TArray<FVector> C=Corners(A);
  TArray<FVector> Result;
  for(int32 I=0;I<4;++I)
  {
   const int32 Prev=(I+3)%4,Next=(I+1)%4;
   double R=FMath::Clamp(static_cast<double>(A.CornerRadius),0.,FMath::Min(FMath::Max(100.,A.Size.X),FMath::Max(100.,A.Size.Y))*.5);
   for(const FLxTerraceOpening& O:A.Connections)
   { if((O.Side==I && O.Interval.X<R+.1) || (O.Side==Prev && O.Interval.Y>FVector::Distance(C[Prev],C[I])-R-.1)) { R=0; } }
   if(R<.01) { Result.Add(C[I]); continue; }
   const FVector U=(C[Prev]-C[I]).GetSafeNormal(),V=(C[Next]-C[I]).GetSafeNormal(),Center=C[I]+(U+V)*R;
   const int32 Steps=FMath::Clamp(FMath::CeilToInt(R/20),4,24);
   for(int32 S=0;S<=Steps;++S)
   { const double T=HALF_PI*S/Steps; Result.Add(Center-V*(R*FMath::Cos(T))-U*(R*FMath::Sin(T))); }
  }
  for(int32 I=Result.Num()-1;I>=0;--I) { if(Result.Num()>1 && Result[I].Equals(Result[(I+1)%Result.Num()],.001)) { Result.RemoveAt(I); } }
  return Result;
 }
 /** 用凸多边形裁剪石砖，使圆角处的砖不会突出外轮廓。 */
 TArray<FVector> Clip(TArray<FVector> P,const TArray<FVector>& Boundary)
 {
  for(int32 E=0;E<Boundary.Num() && P.Num()>=3;++E)
  {
   const FVector A=Boundary[E],B=Boundary[(E+1)%Boundary.Num()],D=B-A;
   TArray<FVector> Out;
   for(int32 I=0;I<P.Num();++I)
   {
    const FVector U=P[I],V=P[(I+1)%P.Num()];
    const double DU=D.X*(U.Y-A.Y)-D.Y*(U.X-A.X),DV=D.X*(V.Y-A.Y)-D.Y*(V.X-A.X);
    if(DU>=-.0001) { Out.Add(U); }
    if((DU>=0)!=(DV>=0)) { Out.Add(FMath::Lerp(U,V,DU/(DU-DV))); }
   }
   P=MoveTemp(Out);
  }
  return P;
 }
 /** 单材质分区的显式硬边网格。 */
 struct FMesh
 {
  /** 顶点位置。 */
  TArray<FVector> V;
  /** 三角索引。 */
  TArray<int32> I;
  /** 法线。 */
  TArray<FVector> N;
  /** 物理尺寸纹理坐标。 */
  TArray<FVector2D> UV;
  /** 铺砖坐标和砖缝比例，供轻量预览材质复现真实砖块。 */
  TArray<FVector2D> BrickUV,JointUV;
  /** 铺砖物理尺寸和左下起点，零尺寸表示普通石材面。 */
  FVector2D PatternSize=FVector2D::ZeroVector,PatternOrigin=FVector2D::ZeroVector;
  /** 铺砖半缝比例，和真实几何使用相同数值。 */
  FVector2D PatternJoint=FVector2D::ZeroVector;
  /** 顶点颜色用于克制的块间色差。 */
  TArray<FLinearColor> Colors;
  /** 各面切线。 */
  TArray<FProcMeshTangent> Tangents;
  /** 按给定朝向添加凸平面。 */
  void Face(TArray<FVector> P,FVector Normal,FLinearColor Color)
  {
   if(P.Num()<3) { return; }
   if(FVector::DotProduct(FVector::CrossProduct(P[2]-P[0],P[1]-P[0]),Normal)<0) { Algo::Reverse(P); }
   const int32 Base=V.Num();
   const FVector T=FMath::Abs(Normal.Z)>.9 ? FVector::ForwardVector : FVector::CrossProduct(FVector::UpVector,Normal).GetSafeNormal();
   const FVector B=FVector::CrossProduct(Normal,T);
   for(FVector Point:P)
   { V.Add(Point);N.Add(Normal);UV.Emplace(FVector::DotProduct(Point,T)/100,FVector::DotProduct(Point,B)/100);Colors.Add(Color);Tangents.Emplace(T,false);
    BrickUV.Add(PatternSize.X>0 ? FVector2D((Point.X-PatternOrigin.X)/PatternSize.X,(Point.Y-PatternOrigin.Y)/PatternSize.Y) : FVector2D::ZeroVector);
    JointUV.Add(PatternJoint); }
   for(int32 K=1;K+1<P.Num();++K) { I.Append({Base,Base+K,Base+K+1}); }
  }
  /** 为凸轮廓构造厚度和简化凸碰撞；顶面可为斜面。 */
  void Prism(const TArray<FVector>& Boundary,double Depth,FLinearColor Color,TArray<TArray<FVector>>* Hulls=nullptr,bool bFlatBottom=false)
  {
   // 斜向接入可在一角恰好贴边，四边连接片因此退化成三角形。
   TArray<FVector> Top=Boundary;
   for(int32 K=Top.Num()-1;K>=0;--K)
   { if(Top.Num()>1 && Top[K].Equals(Top[(K+1)%Top.Num()],.001)) { Top.RemoveAt(K); } }
   if(Top.Num()<3) { return; }
   TArray<FVector> Bottom;
   for(FVector P:Top) { Bottom.Add(FVector(P.X,P.Y,(bFlatBottom?Top[0].Z:P.Z)-Depth)); }
   FVector Normal=FVector::CrossProduct(Top[1]-Top[0],Top[2]-Top[0]).GetSafeNormal();
   if(Normal.IsNearlyZero()) { return; }
   if(Normal.Z<0) { Normal=-Normal; }
   Face(Top,Normal,Color);Face(Bottom,-Normal,Color);
   for(int32 K=0;K<Top.Num();++K)
   {
    const int32 Next=(K+1)%Top.Num(); const FVector D=Top[Next]-Top[K];
    Face({Top[K],Bottom[K],Bottom[Next],Top[Next]},FVector(D.Y,-D.X,0).GetSafeNormal(),Color);
   }
   if(Hulls) { TArray<FVector> Hull=Top;Hull.Append(Bottom);Hulls->Add(MoveTemp(Hull)); }
  }
  /** 对凸石块的顶面周圈做真实倒角；限制倒角宽度，避免小块或锐角翻折。 */
  void BeveledPrism(const TArray<FVector>& Boundary,double Depth,FLinearColor Color,double Amount)
  {
   if(Amount<.001 || Boundary.Num()<3) { Prism(Boundary,Depth,Color); return; }
   TArray<FVector> Top=Boundary;
   for(int32 K=Top.Num()-1;K>=0;--K)
   { if(Top.Num()>1 && Top[K].Equals(Top[(K+1)%Top.Num()],.001)) { Top.RemoveAt(K); } }
   if(Top.Num()<3) { return; }
   double Area=0;FVector Center=FVector::ZeroVector;
   for(int32 K=0;K<Top.Num();++K) { Area+=FVector::CrossProduct(Top[K],Top[(K+1)%Top.Num()]).Z;Center+=Top[K]/Top.Num(); }
   if(Area<0) { Algo::Reverse(Top); }
   const FVector Normal=FVector::CrossProduct(Top[1]-Top[0],Top[2]-Top[0]).GetSafeNormal();
   if(Normal.Z<.1) { Prism(Top,Depth,Color);return; }
   double B=FMath::Min(Amount,Depth*.4);
   for(int32 K=0;K<Top.Num();++K)
   {
    const FVector D=(Top[(K+1)%Top.Num()]-Top[K]).GetSafeNormal2D();
    B=FMath::Min(B,FMath::Abs(FVector::CrossProduct(D,Center-Top[K]).Z)*.25);
   }
   TArray<FVector> Inner,Shoulder,Bottom;
   for(int32 K=0;K<Top.Num();++K)
   {
    const FVector Prev=(Top[K]-Top[(K+Top.Num()-1)%Top.Num()]).GetSafeNormal2D();
    const FVector Next=(Top[(K+1)%Top.Num()]-Top[K]).GetSafeNormal2D();
    const FVector N0(-Prev.Y,Prev.X,0),N1(-Next.Y,Next.X,0),Bisector=(N0+N1).GetSafeNormal();
    const FVector Offset=Bisector*(B/FMath::Max(.1,FVector::DotProduct(Bisector,N0)));
    FVector Point=Top[K]+Offset;Point.Z=Top[K].Z-(Normal.X*Offset.X+Normal.Y*Offset.Y)/Normal.Z;
    Inner.Add(Point);Shoulder.Add(Top[K]-FVector(0,0,B));Bottom.Add(Top[K]-FVector(0,0,Depth));
   }
   Face(Inner,Normal,Color);Face(Bottom,-Normal,Color);
   for(int32 K=0;K<Top.Num();++K)
   {
    const int32 Next=(K+1)%Top.Num();const FVector D=Top[Next]-Top[K],Out=FVector(D.Y,-D.X,0).GetSafeNormal();
    Face({Shoulder[K],Bottom[K],Bottom[Next],Shoulder[Next]},Out,Color);
    const FVector BevelNormal=FVector::CrossProduct(Shoulder[Next]-Shoulder[K],Inner[Next]-Shoulder[K]).GetSafeNormal();
    Face({Shoulder[K],Shoulder[Next],Inner[Next],Inner[K]},BevelNormal,Color);
   }
  }
  /** 提交程序化分区，碰撞由调用方单独提交。 */
  void Submit(UProceduralMeshComponent& Mesh,int32 Slot,UMaterialInterface* Material)
  { Mesh.CreateMeshSection_LinearColor(Slot,V,I,N,UV,BrickUV,JointUV,TArray<FVector2D>(),Colors,Tangents,false);Mesh.SetMaterial(Slot,Material); }
 };
 /** 沿一段外边生成梯形大块石，边石向内部收缩并保留细接缝。 */
 void Border(FMesh& Mesh,FVector A,FVector B,FVector InnerA,FVector InnerB,const ALxStoneTerraceActor& Actor,TArray<TArray<FVector>>& Hulls)
 {
  const double Length=FVector::Dist2D(A,B);
  if(Length<.05) { return; }
  const int32 Count=LxGeneratedMeshBuild::IsPreview(&Actor)?1:FMath::Clamp(FMath::CeilToInt(Length/FMath::Max(30.f,Actor.BorderLength)),1,1024);
  const double Gap=FMath::Min(FMath::Max(0.f,Actor.JointWidth)*.5,Length/Count*.05)/Length;
  for(int32 I=0;I<Count;++I)
  {
   const double U=static_cast<double>(I)/Count+(I?Gap:0),V=static_cast<double>(I+1)/Count-(I+1<Count?Gap:0);
   TArray<FVector> P={FMath::Lerp(A,B,U),FMath::Lerp(A,B,V),FMath::Lerp(InnerA,InnerB,V),FMath::Lerp(InnerA,InnerB,U)};
   for(FVector& Point:P) { Point.Z+=FMath::Max(0.f,Actor.BorderHeight); }
   Mesh.BeveledPrism(P,FMath::Max(0.f,Actor.BorderHeight)+8,FLinearColor(.93f+I%3*.025f,.95f+I%3*.02f,.96f+I%3*.015f),LxGeneratedMeshBuild::IsPreview(&Actor)?0:FMath::Clamp(Actor.BorderBevelMeters,0.f,.1f)*100);
   if(Actor.bEnableCollision && !LxGeneratedMeshBuild::IsPreview(&Actor))
   {
    // 碰撞跨过视觉砖缝，角色沿边石移动时不会陷入细槽。
    const double C0=static_cast<double>(I)/Count,C1=static_cast<double>(I+1)/Count;
    TArray<FVector> Hull={FMath::Lerp(A,B,C0),FMath::Lerp(A,B,C1),FMath::Lerp(InnerA,InnerB,C1),FMath::Lerp(InnerA,InnerB,C0)};
    for(int32 K=0;K<4;++K) { Hull[K].Z+=FMath::Max(0.f,Actor.BorderHeight);Hull.Add(Hull[K]-FVector(0,0,FMath::Max(0.f,Actor.BorderHeight)+8)); }
    Hulls.Add(MoveTemp(Hull));
   }
  }
 }
}

ALxStoneTerraceActor::ALxStoneTerraceActor()
{
 PrimaryActorTick.bCanEverTick=false;
 DimensionHandles=CreateDefaultSubobject<ULxStoneTerraceHandleComponent>(TEXT("DimensionHandles"));
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 TerraceMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("TerraceMesh"));SetRootComponent(TerraceMesh);
 TerraceMesh->bUseComplexAsSimpleCollision=false;TerraceMesh->SetCollisionProfileName(TEXT("BlockAll"));
 OptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("OptimizedMesh"));OptimizedMesh->SetupAttachment(TerraceMesh);
}
void ALxStoneTerraceActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform);RebuildTerrace(); }
void ALxStoneTerraceActor::Destroyed() { FLxRoadConnectionSolver::Refresh(*this,true);Super::Destroyed(); }
#if WITH_EDITOR
void ALxStoneTerraceActor::PostEditMove(bool bFinished)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 if(!bFinished)
 {
  if(GEngine) { GEngine->BroadcastOnActorMoving(this); }
  if(RootComponent) { RootComponent->PostEditComponentMove(false); }
  return;
 }
 Super::PostEditMove(true);
 RebuildTerrace();
}
void ALxStoneTerraceActor::PreEditChange(FProperty* Property)
{
 if(!IsTemplate()) { UObject::PreEditChange(Property); return; }
 Super::PreEditChange(Property);
}
void ALxStoneTerraceActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 const FName Name=Event.GetMemberPropertyName();
 if(Name==GET_MEMBER_NAME_CHECKED(ALxStoneTerraceActor,LengthMeters) || Name==GET_MEMBER_NAME_CHECKED(ALxStoneTerraceActor,WidthMeters))
 {
  if(FMath::IsFinite(LengthMeters) && FMath::IsFinite(WidthMeters))
  { Size=FVector2D(FMath::Max(100.,FMath::RoundToDouble(LengthMeters*100.)),FMath::Max(100.,FMath::RoundToDouble(WidthMeters*100.))); }
  SynchronizeDimensions();
 }
 if(FMath::IsFinite(BorderBevelMeters)) { BorderBevelMeters=FMath::RoundToFloat(FMath::Clamp(BorderBevelMeters,0.f,.1f)*100)/100; }
 LxGeneratedMeshBuild::RequestPreview(this);
 if(!IsTemplate() && (Event.ChangeType & EPropertyChangeType::Interactive)!=0)
 { UObject::PostEditChangeProperty(Event); return; }
 Super::PostEditChangeProperty(Event);
 RebuildTerrace();
}
void ALxStoneTerraceActor::PostEditUndo() { Super::PostEditUndo();RebuildTerrace(); }
#endif
void ALxStoneTerraceActor::RebuildTerrace()
{ SynchronizeDimensions();if(IsTemplate()) { return; } if(GetWorld()) { FLxRoadConnectionSolver::Refresh(*this); } else { BuildGeometry(); } }
void ALxStoneTerraceActor::SynchronizeDimensions()
{
 if(!Size.ContainsNaN() && (SynchronizedSize.IsNearlyZero() || !Size.Equals(SynchronizedSize,.001)))
 { LengthMeters=Size.X/100;WidthMeters=Size.Y/100; }
 if(!FMath::IsFinite(LengthMeters) || !FMath::IsFinite(WidthMeters)) { return; }
 LengthMeters=FMath::Max(1.f,FMath::RoundToFloat(LengthMeters*100)/100);
 WidthMeters=FMath::Max(1.f,FMath::RoundToFloat(WidthMeters*100)/100);
 Size=FVector2D(FMath::RoundToDouble(LengthMeters*100.),FMath::RoundToDouble(WidthMeters*100.));SynchronizedSize=Size;
}
void ALxStoneTerraceActor::SetFootprintMeters(float Length,float Width)
{
 if(!FMath::IsFinite(Length) || !FMath::IsFinite(Width)) { return; }
 Size=FVector2D(FMath::Max(100.,FMath::RoundToDouble(Length*100.)),FMath::Max(100.,FMath::RoundToDouble(Width*100.)));RebuildTerrace();
}
void ALxStoneTerraceActor::SetTerraceDimensions(float Length,float Width,float Height)
{ Size=FVector2D(Length,Width);TerraceHeight=Height;RebuildTerrace(); }

void ALxStoneTerraceActor::BuildGeometry()
{
 using namespace LxTerrace;
 if(!TerraceMesh || !OptimizedMesh) { return; }
 TArray<uint8> Inputs=LxGeneratedMeshBuild::CaptureInputs(this,false);
 FMemoryWriter Writer(Inputs);Writer.Seek(Inputs.Num());FObjectAndNameAsStringProxyArchive Archive(Writer,false);
 int32 GeometryVersion=2;Archive << GeometryVersion;
 bool bEligible=Eligible(*this);Archive << bEligible;
 for(FLxTerraceOpening& O:Connections)
 {
  FLxTerraceOpening::StaticStruct()->SerializeItem(Archive,&O,nullptr);
  if(const ALxSplineRoadActor* Road=Cast<ALxSplineRoadActor>(O.Other))
  { float W=Road->CurbWidth,H=Road->CurbHeight;Archive << W << H; }
 }
 if(!GeometryInputs.IsEmpty() && GeometryInputs==Inputs) { OptimizedMesh->AcceptUnchangedSource();return; }
 GeometryInputs=MoveTemp(Inputs);GenerationDiagnostic.Reset();
 TerraceMesh->ClearAllMeshSections();TerraceMesh->ClearCollisionConvexMeshes();OptimizedMesh->MarkSourceDirty();
 if(!Valid(*this)) { GenerationDiagnostic=TEXT("尺寸必须为有限数值。");OptimizedMesh->Synchronize(TerraceMesh);return; }
 if(bAutoConnect && !Eligible(*this)) { GenerationDiagnostic=TEXT("自动连接需要单位缩放及水平旋转；请通过台地长宽和高度调整尺寸。"); }
 const TArray<FVector> P=Outline(*this),C=Corners(*this);
 FMesh Paving,Stone;TArray<TArray<FVector>> Hulls;
 const bool bPreview=LxGeneratedMeshBuild::IsPreview(this);
 const bool bBuildCollision=bEnableCollision && !bPreview;
 Stone.Prism(P,FMath::Max(1.f,TerraceHeight),FLinearColor(.90f,.92f,.94f),bBuildCollision?&Hulls:nullptr);
 // 顶面砖缝为浅凹槽，碰撞使用连续顶面，角色不会卡在砖缝中。
 const double X=FMath::Max(100.,Size.X),Y=FMath::Max(100.,Size.Y);
 double BX=FMath::Max(20.,BrickSize.X),BY=FMath::Max(20.,BrickSize.Y);
 const double Scale=FMath::Max(1.,FMath::Sqrt((X/BX+2)*(Y/BY+2)/16000.));BX=FMath::Max(BX*Scale,X/120);BY=FMath::Max(BY*Scale,Y/120);
 if(Scale>1.001) { GenerationDiagnostic+=TEXT("超大台地已自动加大砖块以限制几何数量。"); }
 const double Gap=FMath::Clamp(static_cast<double>(JointWidth)*.5,0.,FMath::Min(BX,BY)*.1);
 const int32 Rows=FMath::CeilToInt(Y/BY),Columns=FMath::CeilToInt(X/BX)+1;
 Paving.PatternSize={BX,BY};Paving.PatternOrigin={-X*.5,-Y*.5};
 Paving.PatternJoint={FMath::Max(.000001,Gap/BX),FMath::Max(.000001,Gap/BY)};
 // 连续底面为浅冷灰砖缝；预览依靠相同铺砖坐标显示砖缝和逐砖色差。
 TArray<FVector> Bed=P;for(FVector& Point:Bed) { Point.Z+=bPreview?.15:.04; }
 Paving.Face(Bed,FVector::UpVector,FLinearColor(1,1,1,0));
 for(int32 Row=0;!bPreview && Row<Rows;++Row) for(int32 Col=0;Col<Columns;++Col)
 {
  const double Left=-X*.5+Col*BX-(Row%2)*BX*.5,Bottom=-Y*.5+Row*BY,Z=FMath::Max(1.f,TerraceHeight)+.15;
  TArray<FVector> Tile=Clip({{Left+Gap,Bottom+Gap,Z},{Left+BX-Gap,Bottom+Gap,Z},{Left+BX-Gap,Bottom+BY-Gap,Z},{Left+Gap,Bottom+BY-Gap,Z}},P);
  const float Tone=static_cast<float>((Row*17+Col*13)%11),Shade=.80f+Tone*.024f;
  const FLinearColor Hue=FMath::Lerp(FLinearColor(.96f,.98f,1.02f),FLinearColor(1.04f,1.02f,.98f),FMath::Frac(Tone*.381966f));
  Paving.Prism(Tile,.8,FLinearColor(Hue.R*Shade,Hue.G*Shade,Hue.B*Shade));
 }
 const double Width=FMath::Clamp(static_cast<double>(BorderWidth),5.,FMath::Min(X,Y)*.2);
 TArray<FVector> Inner;
 for(int32 I=0;I<P.Num();++I)
 {
  const FVector Prev=(P[I]-P[(I+P.Num()-1)%P.Num()]).GetSafeNormal2D(),Next=(P[(I+1)%P.Num()]-P[I]).GetSafeNormal2D();
  const FVector N0(-Prev.Y,Prev.X,0),N1(-Next.Y,Next.X,0),Bisector=(N0+N1).GetSafeNormal();
  FVector Inset=P[I]+Bisector*(Width/FMath::Max(.1,FVector::DotProduct(Bisector,N0)));
  Inset.X=FMath::Clamp(Inset.X,-X*.5+Width,X*.5-Width);Inset.Y=FMath::Clamp(Inset.Y,-Y*.5+Width,Y*.5-Width);
  for(int32 Corner=0;Corner<4;++Corner) if(P[I].Equals(C[Corner],.01))
  {
   const int32 PrevSide=(Corner+3)%4;
   const bool bPrevOpen=Connections.ContainsByPredicate([&](const FLxTerraceOpening& O){return O.Side==PrevSide && O.Interval.Y>FVector::Distance(C[PrevSide],C[Corner])-.1;});
   const bool bNextOpen=Connections.ContainsByPredicate([&](const FLxTerraceOpening& O){return O.Side==Corner && O.Interval.X<.1;});
   // 共用边被去掉时，外边石延伸到接缝，不能保留原矩形斜接留下三角缺口。
   if(bPrevOpen || bNextOpen) { Inset=P[I]+(bPrevOpen?FVector::ZeroVector:N0*Width)+(bNextOpen?FVector::ZeroVector:N1*Width); }
  }
  Inner.Add(Inset);
 }
 for(int32 I=0;I<P.Num();++I)
 {
  const int32 Next=(I+1)%P.Num();const FVector A=P[I],B=P[Next],D=B-A;
  TArray<double> Cuts={0,1};int32 Side=INDEX_NONE;
  for(int32 S=0;S<4;++S)
  {
   const FVector T=(C[(S+1)%4]-C[S]).GetSafeNormal2D();
   if(FMath::Abs(FVector::CrossProduct(T,A-C[S]).Z)<.01 && FMath::Abs(FVector::CrossProduct(T,B-C[S]).Z)<.01)
   {
    Side=S;const double Start=FVector::DotProduct(A-C[S],T),Length=D.Size();
    for(const FLxTerraceOpening& O:Connections) if(O.Side==S)
    { Cuts.Add(FMath::Clamp((O.Interval.X-Start)/Length,0.,1.));Cuts.Add(FMath::Clamp((O.Interval.Y-Start)/Length,0.,1.)); }
    break;
   }
  }
  Cuts.Sort();
  for(int32 K=0;K+1<Cuts.Num();++K)
  {
   const double U=Cuts[K],V=Cuts[K+1];if(V-U<.000001) { continue; }
   if(Side!=INDEX_NONE)
   {
    const double Mid=FVector::Distance(C[Side],FMath::Lerp(A,B,(U+V)*.5));
    if(Connections.ContainsByPredicate([&](const FLxTerraceOpening& O){return O.Side==Side && Mid>O.Interval.X-.01 && Mid<O.Interval.Y+.01;})) { continue; }
   }
   const FVector Start=FMath::Lerp(A,B,U),End=FMath::Lerp(A,B,V),Normal=FVector(-D.Y,D.X,0).GetSafeNormal();
   const FVector InnerStart=Side!=INDEX_NONE && U>.000001 ? Start+Normal*Width : FMath::Lerp(Inner[I],Inner[Next],U);
   const FVector InnerEnd=Side!=INDEX_NONE && V<.999999 ? End+Normal*Width : FMath::Lerp(Inner[I],Inner[Next],V);
   Border(Stone,Start,End,InnerStart,InnerEnd,*this,Hulls);
  }
 }
 for(const FLxTerraceOpening& O:Connections) if(O.Bridge.Num()==4)
 {
  const auto& B=O.Bridge;
  if(FVector::Dist2D(B[0],B[3])<.01 && FVector::Dist2D(B[1],B[2])<.01) { continue; }
  Paving.Prism(B,FMath::Max(1.f,TerraceHeight),FLinearColor::White,bBuildCollision?&Hulls:nullptr,true);
  const FVector Across=(B[1]-B[0]).GetSafeNormal2D();
  if(const ALxSplineRoadActor* Road=Cast<ALxSplineRoadActor>(O.Other))
  {
   // 道路包边位于通行宽度之外，并平滑衔接其实际宽度与高度。
   const double RoadWidth=FMath::Max(5.f,Road->CurbWidth);
   const FVector Raise(0,0,FMath::Max(1.f,Road->CurbHeight)-FMath::Max(0.f,BorderHeight));
   Border(Stone,B[3]+Raise-Across*RoadWidth,B[0]-Across*Width,B[3]+Raise,B[0],*this,Hulls);
   Border(Stone,B[1]+Across*Width,B[2]+Raise+Across*RoadWidth,B[1],B[2]+Raise,*this,Hulls);
  }
  else
  {
   // 台地拼接包边保留在统一矩形外轮廓以内。
   Border(Stone,B[3],B[0],B[3]+Across*Width,B[0]+Across*Width,*this,Hulls);
   Border(Stone,B[1],B[2],B[1]-Across*Width,B[2]-Across*Width,*this,Hulls);
  }
 }
 Paving.Submit(*TerraceMesh,0,PavingMaterial);Stone.Submit(*TerraceMesh,1,StoneMaterial);
 TerraceMesh->SetCollisionEnabled(bBuildCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 TerraceMesh->SetCollisionConvexMeshes(Hulls);OptimizedMesh->Synchronize(TerraceMesh);
}

void ALxStoneTerraceActor::SolveConnections(AActor& Changed,bool bRemoving,TFunctionRef<bool(ALxSplineRoadActor&,int32,bool,float)> RoadPort)
{
 using namespace LxTerrace;
 UWorld* World=Changed.GetWorld();if(!World) { return; }
 TArray<ALxStoneTerraceActor*> Terraces;
 for(TActorIterator<ALxStoneTerraceActor> It(World);It;++It)
 { if(IsValid(*It) && !It->IsActorBeingDestroyed() && !(bRemoving && *It==&Changed)) { Terraces.Add(*It); } }
 Terraces.Sort([](const ALxStoneTerraceActor& A,const ALxStoneTerraceActor& B){return A.GetPathName()<B.GetPathName();});
 TArray<TArray<FLxTerraceOpening>> Previous;
 for(ALxStoneTerraceActor* A:Terraces) { A->SynchronizeDimensions();Previous.Add(A->Connections);A->Connections.Reset(); }
 // 台地仅拼接相向的直边，允许部分边重合形成阶梯或丁字布局。
 for(int32 IA=0;IA<Terraces.Num();++IA) for(int32 IB=IA+1;IB<Terraces.Num();++IB)
 {
  ALxStoneTerraceActor& A=*Terraces[IA];ALxStoneTerraceActor& B=*Terraces[IB];if(!Eligible(A)||!Eligible(B)) { continue; }
  const TArray<FVector> CA=Corners(A),CB=Corners(B);const FTransform TA=A.GetActorTransform(),TB=B.GetActorTransform();
  if(FMath::Abs(TA.TransformPosition(CA[0]).Z-TB.TransformPosition(CB[0]).Z)>.1) { continue; }
  for(int32 SA=0;SA<4;++SA) for(int32 SB=0;SB<4;++SB)
  {
   const FVector A0=TA.TransformPosition(CA[SA]),A1=TA.TransformPosition(CA[(SA+1)%4]),B0=TB.TransformPosition(CB[SB]),B1=TB.TransformPosition(CB[(SB+1)%4]);
   const FVector T=(A1-A0).GetSafeNormal2D(),U=(B1-B0).GetSafeNormal2D(),N(T.Y,-T.X,0);
   if(FVector::DotProduct(T,U)>-.99999) { continue; }
   const double Gap=FVector::DotProduct(B0-A0,N);
   if(Gap<-.01 || Gap>FMath::Max(0.f,FMath::Min(A.AutoConnectDistance,B.AutoConnectDistance))) { continue; }
   const double L=(A1-A0).Size(),BL=(B1-B0).Size(),BStart=FVector::DotProduct(B0-A0,T),BEnd=FVector::DotProduct(B1-A0,T);
   const double Low=FMath::Max(0.,BEnd),High=FMath::Min(L,BStart);
   if(High-Low<FMath::Max(A.BorderWidth,B.BorderWidth)*2+1 || !Free(A,SA,Low,High)) { continue; }
   const double OtherLow=FMath::Clamp(BStart-High,0.,BL),OtherHigh=FMath::Clamp(BStart-Low,0.,BL);
   if(!Free(B,SB,OtherLow,OtherHigh)) { continue; }
   FLxTerraceOpening OA;OA.Other=&B;OA.Side=SA;OA.Interval={Low,High};
   OA.Bridge={TA.InverseTransformPosition(A0+T*Low),TA.InverseTransformPosition(A0+T*High),TA.InverseTransformPosition(A0+T*High+N*Gap),TA.InverseTransformPosition(A0+T*Low+N*Gap)};
   // 外侧填缝轮廓的绕序与台地轮廓一致。

   // 保持第一条边为台地入口，供统一包边算法使用。
   OA.Bridge={TA.InverseTransformPosition(A0+T*High),TA.InverseTransformPosition(A0+T*Low),TA.InverseTransformPosition(A0+T*Low+N*Gap),TA.InverseTransformPosition(A0+T*High+N*Gap)};
   A.Connections.Add(OA);FLxTerraceOpening OB;OB.Other=&A;OB.Side=SB;OB.Interval={OtherLow,OtherHigh};B.Connections.Add(OB);
  }
 }
 /** 道路候选按间隙排序，保证端点选择最近且不重叠的入口。 */
 struct FCandidate
 {
  /** 目标台地和接入道路。 */
  ALxStoneTerraceActor* Terrace=nullptr;ALxSplineRoadActor* Road=nullptr;
  /** 矩形边和道路端点编号。 */
  int32 Side=0,End=0;
  /** 端点到边的距离和开口区间。 */
  double Gap=0,Low=0,High=0;
  /** 插入部分的裁切长度，单位米。 */
  float TrimMeters=0;
  /** 台地侧和道路侧的四点局部轮廓。 */
  TArray<FVector> Bridge;
 };
 TArray<FCandidate> Candidates;
 for(TActorIterator<ALxSplineRoadActor> It(World);It;++It)
 {
  ALxSplineRoadActor* R=*It;USplineComponent* S=R->RoadSpline;
  if(!IsValid(R)||R->IsActorBeingDestroyed()||(bRemoving && R==&Changed)||!R->bAutoConnect||!S||S->IsClosedLoop()||S->GetNumberOfSplinePoints()<2
   ||!S->GetComponentScale().Equals(FVector::OneVector,.001)||FVector::DotProduct(S->GetUpVector(),FVector::UpVector)<.9999
   ||!FMath::IsFinite(R->RoadWidth)||!FMath::IsFinite(R->AutoConnectDistance)||!FMath::IsFinite(R->AutoConnectHeight)) { continue; }
  for(int32 End=0;End<2;++End)
  {
   if(!RoadPort(*R,End,false,0)) { continue; }
   const int32 Index=End?S->GetNumberOfSplinePoints()-1:0;
   const FVector E=S->GetLocationAtSplinePoint(Index,ESplineCoordinateSpace::World),D=S->GetDirectionAtSplinePoint(Index,ESplineCoordinateSpace::World).GetSafeNormal2D()*(End?1:-1);
   const FVector Across(-D.Y,D.X,0);const double HW=FMath::Max(50.f,R->RoadWidth)*.5;
   for(ALxStoneTerraceActor* A:Terraces)
   {
    if(!Eligible(*A)) { continue; }
    const FTransform T=A->GetActorTransform();const TArray<FVector> C=Corners(*A);
    for(int32 Side=0;Side<4;++Side)
    {
     const FVector P=T.TransformPosition(C[Side]),Q=T.TransformPosition(C[(Side+1)%4]),Along=(Q-P).GetSafeNormal2D(),N(Along.Y,-Along.X,0);
     const double Into=-FVector::DotProduct(D,N),Gap=FVector::DotProduct(E-P,N);
     if(Into<.5 || FMath::Abs(Gap)>FMath::Max(0.f,FMath::Min(A->AutoConnectDistance,R->AutoConnectDistance))
      || FMath::Abs(E.Z-P.Z)>FMath::Max(0.f,FMath::Min(A->AutoConnectHeight,R->AutoConnectHeight))) { continue; }
     // 内外距离使用同一范围；插入时沿真实样条回退到完整路幅露出台地的位置。
     FVector Join=E,Direction=D,Cross=Across;
     double Trim=0;
     const double SplineLength=S->GetSplineLength();
     const double Clearance=FMath::Abs(E.Z-P.Z)>.1 ? 10. : 0.;
     /** 计算指定端部回退长度处的路幅到台地边界的最小有符号距离。 */
     const auto SampleGap=[&](double Retraction)
     {
      const double Distance=End ? SplineLength-Retraction : Retraction;
      Join=S->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);
      Direction=S->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World).GetSafeNormal2D()*(End?1:-1);
      Cross=FVector(-Direction.Y,Direction.X,0);
      return FVector::DotProduct(Join-P,N)-FMath::Abs(FVector::DotProduct(Cross*HW,N))-Clearance;
     };
     if(SampleGap(0)<-.001)
     {
      double LowTrim=0,HighTrim=0;
      const double Limit=FMath::Max(0.,SplineLength-1.);
      // 固定上限搜索，不按道路长度产生无界开销。
      for(int32 Step=1;Step<=128;++Step)
      {
       HighTrim=Limit*Step/128.;
       if(SampleGap(HighTrim)>=0) { break; }
       LowTrim=HighTrim;
      }
      if(HighTrim<=LowTrim) { continue; }
      for(int32 Step=0;Step<24;++Step)
      { const double Mid=(LowTrim+HighTrim)*.5; if(SampleGap(Mid)<0) { LowTrim=Mid; } else { HighTrim=Mid; } }
      Trim=HighTrim;
      SampleGap(Trim);
     }
     const double JoinInto=-FVector::DotProduct(Direction,N);
     if(JoinInto<.5 || FMath::Abs(Join.Z-P.Z)>FMath::Max(0.f,FMath::Min(A->AutoConnectHeight,R->AutoConnectHeight))) { continue; }
     FVector Left=Join-Cross*HW,Right=Join+Cross*HW;
     if(FVector::DotProduct(Right-Left,Along)<0) { Swap(Left,Right); }
     const double G0=FVector::DotProduct(Left-P,N),G1=FVector::DotProduct(Right-P,N);
     FVector L=Left+Direction*(G0/JoinInto),H=Right+Direction*(G1/JoinInto);L.Z=P.Z;H.Z=P.Z;
     const double Low=FVector::DotProduct(L-P,Along),High=FVector::DotProduct(H-P,Along),Length=(Q-P).Size();
     const double Margin=FMath::Clamp(static_cast<double>(A->CornerRadius),0.,FMath::Min(A->Size.X,A->Size.Y)*.5)+FMath::Max(5.f,A->BorderWidth);
     if(Low<Margin || High>Length-Margin || !Free(*A,Side,Low,High)) { continue; }
     FCandidate Candidate;Candidate.Terrace=A;Candidate.Road=R;Candidate.Side=Side;Candidate.End=End;Candidate.Gap=FMath::Abs(Gap);Candidate.TrimMeters=Trim/100.;Candidate.Low=Low;Candidate.High=High;
     Candidate.Bridge={T.InverseTransformPosition(H),T.InverseTransformPosition(L),T.InverseTransformPosition(Left),T.InverseTransformPosition(Right)};
     Candidates.Add(MoveTemp(Candidate));
    }
   }
  }
 }
 Candidates.StableSort([](const FCandidate& A,const FCandidate& B){return A.Gap<B.Gap;});
 for(const FCandidate& C:Candidates)
 {
  if(!Free(*C.Terrace,C.Side,C.Low,C.High)||!RoadPort(*C.Road,C.End,true,C.TrimMeters)) { continue; }
  FLxTerraceOpening O;O.Other=C.Road;O.Side=C.Side;O.Interval={C.Low,C.High};O.Bridge=C.Bridge;C.Terrace->Connections.Add(O);
 }
 for(int32 I=0;I<Terraces.Num();++I)
 {
  ALxStoneTerraceActor* A=Terraces[I];
#if WITH_EDITOR
  // 在修改记录前恢复旧值，让跨对象撤销保存正确的连接快照。
  if(A->Connections!=Previous[I] && !World->IsGameWorld() && !UE::IsSavingPackage())
  { TArray<FLxTerraceOpening> Current=MoveTemp(A->Connections);A->Connections=Previous[I];A->Modify();A->Connections=MoveTemp(Current); }
#endif
  if(LxGeneratedMeshBuild::IsPreview(&Changed) && (A!=&Changed && A->Connections!=Previous[I])) { LxGeneratedMeshBuild::RequestPreview(A); }
  A->BuildGeometry();
 }
}
