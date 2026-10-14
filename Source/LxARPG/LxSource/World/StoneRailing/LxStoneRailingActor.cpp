#include "LxStoneRailingActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "Algo/Reverse.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/Material.h"
#include "Misc/ScopeExit.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"

namespace LxStoneRailing
{
 /** 拐角采样与柱头适配算法版本，用于自动升级旧关卡缓存。 */
 constexpr int32 GeometryVersion=2;
 /** 批量生成一个材质分区，法线和纹理随实际几何写入。 */
 struct FSection
 {
  /** 顶点位置及三角形索引。 */
  TArray<FVector> Vertices;
  TArray<int32> Indices;
  /** 平面法线、连续石纹坐标和颜色。 */
  TArray<FVector> Normals;
  TArray<FVector2D> UV;
  TArray<FLinearColor> Colors;
  /** 沿首条边的切线。 */
  TArray<FProcMeshTangent> Tangents;
  /** 写入凸多边形，以 UE 顺时针朝向生成三角形。 */
  void Face(const TArray<FVector>& P)
  {
   const FVector N = FVector::CrossProduct(P[2]-P[0], P[1]-P[0]).GetSafeNormal();
   if (N.IsNearlyZero()) { return; }
   const FVector T = (P[1]-P[0]).GetSafeNormal();
   const FVector V = FVector::CrossProduct(N,T);
   const int32 Start = Vertices.Num();
   for (const FVector& Point : P)
   {
    Vertices.Add(Point); Normals.Add(N);
    UV.Add(FVector2D(FVector::DotProduct(Point,T), FVector::DotProduct(Point,V))/100);
    Colors.Add(FLinearColor::White); Tangents.Add(FProcMeshTangent(T,false));
   }
   for (int32 I=1; I+1<P.Num(); ++I) { Indices.Append({Start,Start+I,Start+I+1}); }
  }
  /** 连接等顶点数的轮廓；闭合路径不加重复端盖。 */
  void Loft(const TArray<TArray<FVector>>& Rings, bool bCaps)
  {
   const int32 N = Rings[0].Num();
   for (int32 R=0; R+1<Rings.Num(); ++R)
   for (int32 I=0; I<N; ++I)
   {
    const int32 J=(I+1)%N;
    // 每个四边形拆成两张面，弯曲路径的非共面四边形保留正确法线。
    Face({Rings[R][I],Rings[R+1][I],Rings[R+1][J]});
    Face({Rings[R][I],Rings[R+1][J],Rings[R][J]});
   }
   if (!bCaps) { return; }
   for (int32 End=0; End<2; ++End)
   {
    TArray<FVector> Cap = End ? Rings.Last() : Rings[0];
    if (End) { Algo::Reverse(Cap); }
    FVector Center=FVector::ZeroVector;
    for (const FVector& P : Cap) { Center+=P; }
    Center/=Cap.Num();
    for (int32 I=0; I<Cap.Num(); ++I) { Face({Center,Cap[I],Cap[(I+1)%Cap.Num()]}); }
   }
  }
  /** 提交无复杂三角面碰撞的网格分区。 */
  void Commit(UProceduralMeshComponent* Mesh, int32 Index)
  {
   Mesh->CreateMeshSection_LinearColor(Index,Vertices,Indices,Normals,UV,Colors,Tangents,false);
  }
 };

 /** 返回水平面八边形截面，让柱身竖边具备真实倒角。 */
 TArray<FVector> PostRing(const FVector& Center, const FVector& X, const FVector& Y, double A, double B, double Z)
 {
  const double C=FMath::Min(A,B)*.14;
  const FVector2D Corners[]={{A,-B+C},{A,B-C},{A-C,B},{-A+C,B},{-A,B-C},{-A,-B+C},{-A+C,-B},{A-C,-B}};
  TArray<FVector> Ring;
  for (const FVector2D& P : Corners) { Ring.Add(Center+X*P.X+Y*P.Y+FVector(0,0,Z)); }
  return Ring;
 }

 /** 用六边形细石线描出真实浮雕，端点收口，花纹不依赖贴图。 */
 void Stroke(FSection& Mesh, const TArray<FVector>& Points, double Radius, const FVector& SurfaceNormal)
 {
  TArray<TArray<FVector>> Rings;
  const bool bLoop=Points.Num()>3 && Points[0].Equals(Points.Last(),.0001);
  for (int32 I=0; I<Points.Num(); ++I)
  {
   const int32 Previous=bLoop && I==0 ? Points.Num()-2 : FMath::Max(0,I-1);
   const int32 Next=bLoop && I==Points.Num()-1 ? 1 : FMath::Min(I+1,Points.Num()-1);
   const FVector Tangent=(Points[Next]-Points[Previous]).GetSafeNormal();
   const FVector Side=FVector::CrossProduct(SurfaceNormal,Tangent).GetSafeNormal();
   TArray<FVector> Ring;
   for (int32 J=0; J<6; ++J)
   {
    const double Angle=2*PI*J/6;
    Ring.Add(Points[I]+Radius*(Side*FMath::Cos(Angle)+SurfaceNormal*FMath::Sin(Angle)));
   }
   Rings.Add(MoveTemp(Ring));
  }
  if (bLoop) { Rings.Last()=Rings[0]; }
  Mesh.Loft(Rings,!bLoop);
 }

 /** 柱身两侧的菱纹、四瓣花叶或双重波纹。 */
 void Ornament(FSection& Mesh, ELxStoneRailingStyle Style, const FVector& Center,
  const FVector& X, const FVector& Y, double W, double H)
 {
  for (int32 Side : {-1,1})
  {
   const FVector Origin=Center+Y*(Side*W*.278)+FVector(0,0,H*.44);
   const FVector Normal=Y*Side;
   const auto Draw=[&](const TArray<FVector2D>& Outline)
   {
    TArray<FVector> Points;
    for (const FVector2D& P : Outline) { Points.Add(Origin+X*(P.X*W)+FVector(0,0,P.Y*H)); }
    Stroke(Mesh,Points,W*.0125,Normal);
   };
   if (Style==ELxStoneRailingStyle::Harbour)
   {
    Draw({{0,-.13},{.145,0},{0,.13},{-.145,0},{0,-.13}});
    Draw({{0,-.065},{.07,0},{0,.065},{-.07,0},{0,-.065}});
   }
   else if (Style==ELxStoneRailingStyle::Floral)
   {
    for (int32 Petal=0; Petal<4; ++Petal)
    {
     TArray<FVector2D> Outline;
     for (int32 I=0; I<=20; ++I)
     {
      const double A=2*PI*I/20, R=.055*(1-FMath::Cos(A)), B=A*.5+Petal*PI/2;
      Outline.Add({R*FMath::Cos(B)*1.5,R*FMath::Sin(B)});
     }
     Draw(Outline);
    }
   }
   else
   {
    for (int32 Wave=0; Wave<2; ++Wave)
    {
     TArray<FVector2D> Outline;
     for (int32 I=0; I<=24; ++I)
     {
      const double T=I/24.0;
      Outline.Add({.105*FMath::Sin(T*2*PI+Wave*PI),-.15+T*.3});
     }
     Draw(Outline);
    }
   }
  }
 }
}

ALxStoneRailingActor::ALxStoneRailingActor()
{
 PrimaryActorTick.bCanEverTick=false;
 RailingSpline=CreateDefaultSubobject<USplineComponent>(TEXT("RailingSpline"));
 SetRootComponent(RailingSpline);
 RailingSpline->SetMobility(EComponentMobility::Movable);
 RailingSpline->ClearSplinePoints(false);
 for (int32 I=0; I<3; ++I)
 {
  RailingSpline->AddSplinePoint(FVector(I*400,0,0),ESplineCoordinateSpace::Local,false);
  RailingSpline->SetSplinePointType(I,ESplinePointType::Linear,false);
 }
 RailingSpline->UpdateSpline();
 RailingMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("RailingMesh"));
 RailingMesh->SetupAttachment(RailingSpline);
 RailingMesh->bUseComplexAsSimpleCollision=false;
 RailingMesh->bUseAsyncCooking=false;
 RailingMesh->SetCollisionProfileName(TEXT("BlockAll"));
 RailingMesh->SetGenerateOverlapEvents(false);
 OptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("OptimizedRailing"));
 OptimizedMesh->SetupAttachment(RailingMesh);
}

void ALxStoneRailingActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 if (GeneratedGeometryVersion!=LxStoneRailing::GeometryVersion || !OptimizedMesh->IsSourceCurrent()) { RebuildRailing(); }
}

void ALxStoneRailingActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if (!IsTemplate() && !HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad)
  && GeneratedGeometryVersion!=LxStoneRailing::GeometryVersion && RailingMesh && RailingMesh->IsRegistered())
 { RebuildRailing(); }
}

void ALxStoneRailingActor::RebuildRailing()
{
 using namespace LxStoneRailing;
 if (IsTemplate() || !RailingSpline || !RailingMesh || !OptimizedMesh) { return; }
 RailingSpline->SetClosedLoop(bClosedLoop,true);
 RailingMesh->SetRelativeTransform(FTransform::Identity);
 OptimizedMesh->SetRelativeTransform(FTransform::Identity);
 RailingMesh->ClearAllMeshSections(); RailingMesh->ClearCollisionConvexMeshes();
 OptimizedMesh->MarkSourceDirty();
 ON_SCOPE_EXIT { OptimizedMesh->Synchronize(RailingMesh); };
 GeneratedPostCount=0; GeneratedTriangleCount=0; GenerationDiagnostic.Reset(); ++GeometryBuildCount;
 GeneratedGeometryVersion=GeometryVersion;
 RailingMesh->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
 if (!FMath::IsFinite(Height) || !FMath::IsFinite(Width) || !FMath::IsFinite(Density))
 { GenerationDiagnostic=TEXT("高度、宽度与密度必须是有效数值。"); return; }
 const double H=FMath::Clamp(double(Height),40.0,300.0), W=FMath::Clamp(double(Width),20.0,100.0);
 const double RequestedSpacing=1000/FMath::Clamp(double(Density),2.0,30.0);
 const double Spacing=FMath::Max(RequestedSpacing,W*1.65);
 const int32 Count=RailingSpline->GetNumberOfSplinePoints();
 const double Length=RailingSpline->GetSplineLength();
 if (Count<(bClosedLoop?3:2) || Count>128 || !FMath::IsFinite(Length) || Length<1 || Length>80000)
 { GenerationDiagnostic=TEXT("请使用 2 至 128 个有效控制点（闭合至少 3 点），总长不超过 800 米。"); return; }
 if (Spacing>RequestedSpacing+.01) { GenerationDiagnostic=TEXT("密度过高，已根据护栏宽度限制间距，避免柱头相互重叠。"); }

 TArray<double> Distances, PostDistances;
 TArray<bool> ControlSamples;
 const int32 Spans=bClosedLoop?Count:Count-1;
 Distances.Add(0); PostDistances.Add(0);
 ControlSamples.Add(true);
 for (int32 Span=0; Span<Spans; ++Span)
 {
  const double Start=RailingSpline->GetDistanceAlongSplineAtSplinePoint(Span);
  const double End=Span+1<Count ? RailingSpline->GetDistanceAlongSplineAtSplinePoint(Span+1) : Length;
  const double Size=End-Start;
  if (!FMath::IsFinite(Size) || Size<1)
  { GenerationDiagnostic=TEXT("存在重合或过近的控制点，请将其分开至少 1 厘米。"); return; }
  // 直线只保留真实端点，不能在斜接内角的占用范围内插入直截面。
  const int32 Samples=RailingSpline->GetSplinePointType(Span)==ESplinePointType::Linear
   ? 1 : FMath::Max(1,FMath::CeilToInt(Size/20));
  const int32 Bays=FMath::Max(1,FMath::RoundToInt(Size/Spacing));
  for (int32 I=1; I<=Samples; ++I)
  { Distances.Add(FMath::Lerp(Start,End,double(I)/Samples)); ControlSamples.Add(I==Samples); }
  for (int32 I=1; I<=Bays; ++I)
  { if (!bClosedLoop || Span+1<Spans || I<Bays) { PostDistances.Add(FMath::Lerp(Start,End,double(I)/Bays)); } }
 }
 TArray<FVector> Centers, Across;
 for (double D : Distances)
 {
  const FVector P=RailingSpline->GetLocationAtDistanceAlongSpline(D,ESplineCoordinateSpace::Local);
  if (P.ContainsNaN()) { GenerationDiagnostic=TEXT("控制点包含无效坐标。"); return; }
  Centers.Add(P);
 }
 TArray<double> CornerClearance;
 for (int32 I=0; I<Centers.Num(); ++I)
 {
  const FVector Before=I>0 ? Centers[I]-Centers[I-1] : (bClosedLoop?Centers[0]-Centers[Centers.Num()-2]:Centers[1]-Centers[0]);
  const FVector After=I+1<Centers.Num()?Centers[I+1]-Centers[I]:(bClosedLoop?Centers[1]-Centers[0]:Before);
  if (Before.SizeSquared2D()<.01 || After.SizeSquared2D()<.01 || FMath::Abs(After.Z)>After.Size2D()*1.5)
  { GenerationDiagnostic=TEXT("路径包含竖直或过陡片段，请降低控制点高差。"); return; }
  const FVector A=Before.GetSafeNormal2D(), B=After.GetSafeNormal2D();
  if (FVector::DotProduct(A,B)<-.85)
  { GenerationDiagnostic=TEXT("路径出现接近折返的尖角，请增加过渡控制点。"); return; }
  const FVector Tangent=(A+B).GetSafeNormal();
  const FVector Y(-Tangent.Y,Tangent.X,0);
  Across.Add(Y/FMath::Max(.28,FVector::DotProduct(Y,FVector(-B.Y,B.X,0))));
  CornerClearance.Add(ControlSamples[I] && FVector::DotProduct(A,B)<.999
   ? W*.5*FMath::Sqrt((1-FVector::DotProduct(A,B))/(1+FVector::DotProduct(A,B)))+2 : 0);
 }
 // 混合曲线在折角附近也必须给斜接让位，保留真实控制点，仅移除过近的辅助采样。
 TArray<bool> KeepSamples; KeepSamples.Init(true,Centers.Num());
 for (int32 Corner=0; Corner<Centers.Num(); ++Corner)
 {
  if (CornerClearance[Corner]<=0) { continue; }
  for (int32 Sample=1; Sample+1<Centers.Num(); ++Sample)
  {
   double Delta=FMath::Abs(Distances[Sample]-Distances[Corner]);
   if (bClosedLoop) { Delta=FMath::Min(Delta,Length-Delta); }
   if (!ControlSamples[Sample] && Delta<CornerClearance[Corner]) { KeepSamples[Sample]=false; }
  }
 }
 for (int32 I=Centers.Num()-1; I>=0; --I)
 { if (!KeepSamples[I]) { Centers.RemoveAt(I); Across.RemoveAt(I); } }
 // 两侧边界都必须朝前。短段或曲率半径不足时给出说明，禁止输出自交的内侧面。
 for (int32 I=0; I+1<Centers.Num(); ++I)
 {
  const FVector Delta=Centers[I+1]-Centers[I], Forward=Delta.GetSafeNormal2D();
  const double Advance=FVector::DotProduct(Delta,Forward);
  const double Miter=FMath::Abs(FVector::DotProduct((Across[I+1]-Across[I])*(W*.5),Forward));
  if (Advance-Miter<.05)
  { GenerationDiagnostic=TEXT("拐角之间的距离或曲线半径不足，请拉开控制点、放缓转角或减小护栏宽度。"); return; }
 }
 if (bClosedLoop) { Across.Last()=Across[0]; Centers.Last()=Centers[0]; }
 else
 {
  // 开放端的扶手伸出柱头，确保端柱也有完整 T 形顶盖。
  const FVector Start=Centers[1]-Centers[0], End=Centers.Last()-Centers[Centers.Num()-2];
  const FVector First=Centers[0]-Start/Start.Size2D()*(W*.75);
  const FVector Last=Centers.Last()+End/End.Size2D()*(W*.75);
  const FVector FirstAcross=Across[0], LastAcross=Across.Last();
  Centers.Insert(First,0); Centers.Add(Last); Across.Insert(FirstAcross,0); Across.Add(LastAcross);
 }
 FSection Stone, Detail;
 // 顶梁截面包括下部斜托、细腰线、双倒角和上部平整扶手。
 const FVector2D Profile[]={{.39,.84},{.46,.855},{.46,.88},{.42,.895},{.46,.915},{.50,.94},
  {.50,.975},{.465,1},{-.465,1},{-.50,.975},{-.50,.94},{-.46,.915},{-.42,.895},{-.46,.88},{-.46,.855},{-.39,.84}};
 TArray<TArray<FVector>> BeamRings, Hulls;
 for (int32 I=0; I<Centers.Num(); ++I)
 {
  TArray<FVector> Ring;
  for (const FVector2D& P : Profile) { Ring.Add(Centers[I]+Across[I]*(P.X*W)+FVector(0,0,P.Y*H)); }
  BeamRings.Add(MoveTemp(Ring));
 }
 Detail.Loft(BeamRings,!bClosedLoop);
 for (int32 I=0; I+1<BeamRings.Num(); ++I)
 {
  TArray<FVector> Hull=BeamRings[I]; Hull.Append(BeamRings[I+1]); Hulls.Add(MoveTemp(Hull));
 }
 for (int32 Index=0; Index<PostDistances.Num(); ++Index)
 {
  const double D=PostDistances[Index];
  const FVector Center=RailingSpline->GetLocationAtDistanceAlongSpline(D,ESplineCoordinateSpace::Local);
  // 自动曲线的末端导数可能为零；用邻近位置求方向，避免末柱被压成零体积平面。
  const double BeforeD=bClosedLoop?FMath::Fmod(D-.5+Length,Length):FMath::Max(0.0,D-.5);
  const double AfterD=bClosedLoop?FMath::Fmod(D+.5,Length):FMath::Min(Length,D+.5);
  const FVector Direction=(RailingSpline->GetLocationAtDistanceAlongSpline(AfterD,ESplineCoordinateSpace::Local)
   -RailingSpline->GetLocationAtDistanceAlongSpline(BeforeD,ESplineCoordinateSpace::Local)).GetSafeNormal();
  const FVector Incoming=(Center-RailingSpline->GetLocationAtDistanceAlongSpline(BeforeD,ESplineCoordinateSpace::Local)).GetSafeNormal2D();
  const FVector Outgoing=(RailingSpline->GetLocationAtDistanceAlongSpline(AfterD,ESplineCoordinateSpace::Local)-Center).GetSafeNormal2D();
  const bool bCorner=!Incoming.IsNearlyZero() && !Outgoing.IsNearlyZero() && FVector::DotProduct(Incoming,Outgoing)<.999;
  const FVector X=Direction.GetSafeNormal2D();
  const FVector Y(-X.Y,X.X,0);
  // 短段按可用间距压缩柱头的沿线宽度，避免两个端柱叠在一起。
  double Gap=Spacing;
  if (Index>0) { Gap=FMath::Min(Gap,D-PostDistances[Index-1]); }
  if (Index+1<PostDistances.Num()) { Gap=FMath::Min(Gap,PostDistances[Index+1]-D); }
  if (bClosedLoop && (Index==0 || Index+1==PostDistances.Num())) { Gap=FMath::Min(Gap,Length-PostDistances.Last()); }
  const double SX=FMath::Min(1.0,Gap/(W*1.6));
  const double Flare=Style==ELxStoneRailingStyle::Floral?1.08:(Style==ELxStoneRailingStyle::Wave?.93:1.0);
  // Z、沿线半宽、横向半宽；连续截面形成底座、收腰柱身和曲线外展托臂。
  const FVector Shape[]={{0,.33,.34},{.018,.38,.38},{.055,.38,.38},{.085,.34,.34},{.11,.28,.29},
   {.15,.25,.27},{.64,.235,.27},{.68,.27,.29},{.70,.31,.32},{.73,.35,.34},
   {.765,.44*Flare,.37},{.80,.56*Flare,.40},{.84,.67*Flare,.43},{.855,.67*Flare,.43}};
  TArray<TArray<FVector>> Rings;
  for (const FVector& S : Shape)
  {
   // 转角改用紧凑倒角柱头，宽托臂不能按角平分线穿出 L 形扶手的内角。
   const double A=bCorner && S.X>=.68 ? FMath::Min(S.Y,.29) : S.Y;
   const double B=bCorner && S.X>=.68 ? FMath::Min(S.Z,.29) : S.Z;
   TArray<FVector> Ring=PostRing(Center,X,Y,A*W*SX,B*W,S.X*H);
   // 托臂逐渐随坡度倾斜，柱身仍竖直，顶端与斜向扶手保持接触。
   const double Slope=Direction.Z/FMath::Max(.01,Direction.Size2D());
   for (FVector& P : Ring) { P.Z+=FVector::DotProduct(P-Center,X)*Slope*FMath::Clamp((S.X-.68)/.16,0.0,1.0); }
   Rings.Add(MoveTemp(Ring));
  }
  Stone.Loft(Rings,true);
  // 将柱脚、柱身、柱头分别包围，不能让宽柱头把中部空隙也堵住。
  const int32 Cuts[]={0,4,7,13};
  for (int32 Part=0; Part<3; ++Part)
  {
   TArray<FVector> Hull;
   for (int32 R=Cuts[Part]; R<=Cuts[Part+1]; ++R) { Hull.Append(Rings[R]); }
   Hulls.Add(MoveTemp(Hull));
  }
  if (bOrnaments && SX>.7) { Ornament(Detail,Style,Center,X,Y,W,H); }
  ++GeneratedPostCount;
 }
 Stone.Commit(RailingMesh,0); Detail.Commit(RailingMesh,1);
 RailingMesh->SetMaterial(0,StoneMaterial?StoneMaterial.Get():UMaterial::GetDefaultMaterial(MD_Surface));
 RailingMesh->SetMaterial(1,OrnamentMaterial?OrnamentMaterial.Get():RailingMesh->GetMaterial(0));
 if (bEnableCollision) { RailingMesh->SetCollisionConvexMeshes(Hulls); }
 GeneratedTriangleCount=(Stone.Indices.Num()+Detail.Indices.Num())/3;
}
