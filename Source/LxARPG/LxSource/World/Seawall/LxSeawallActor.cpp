#include "LxSeawallActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "Components/SplineComponent.h"
#include "ProceduralMeshComponent.h"
#include "Misc/ScopeExit.h"
#include "Algo/BinarySearch.h"

namespace LxSeawall
{
 /** 一个材质槽的硬边网格数据；岩板倒角使用真实几何。 */
 struct FMesh
 {
  /** 局部顶点。 */
  TArray<FVector> Vertices;
  /** 顺时针索引。 */
  TArray<int32> Indices;
  /** 独立面法线。 */
  TArray<FVector> Normals;
  /** 按厘米转换的纹理坐标。 */
  TArray<FVector2D> UV;
  /** 与纹理横轴一致的切线。 */
  TArray<FProcMeshTangent> Tangents;
  /** 石块逐块色差，白色保持材质原色。 */
  TArray<FColor> Colors;
  /** 当前石块的顶点颜色。 */
  FColor StoneColor=FColor::White;
  /** 按指定外侧方向修正绕序，略过退化面。 */
  void Triangle(FVector A,FVector B,FVector C,FVector2D UA,FVector2D UB,FVector2D UC,FVector Out)
  {
   FVector N=FVector::CrossProduct(C-A,B-A).GetSafeNormal();
   if(N.IsNearlyZero()) { return; }
   if(FVector::DotProduct(N,Out)<0) { Swap(B,C); Swap(UB,UC); N=-N; }
   const FVector2D DU=UB-UA,DV=UC-UA;
   const double Det=DU.X*DV.Y-DU.Y*DV.X;
   const FVector T=FMath::Abs(Det)>1.e-8 ? ((B-A)*DV.Y-(C-A)*DU.Y)/Det : B-A;
   const int32 Start=Vertices.Num();
   Vertices.Append({A,B,C}); UV.Append({UA,UB,UC}); Indices.Append({Start,Start+1,Start+2});
   for(int32 I=0;I<3;++I) { Normals.Add(N); Tangents.Emplace(T.GetSafeNormal(),false); Colors.Add(StoneColor); }
  }
  /** 用两三角形组成曲面小片，UV 的横向为沿堤距离、纵向为坡面距离。 */
  void Quad(FVector A,FVector B,FVector C,FVector D,FVector2D UA,FVector2D UB,FVector2D UC,FVector2D UD,FVector Out)
  { Triangle(A,B,C,UA,UB,UC,Out); Triangle(A,C,D,UA,UC,UD,Out); }
  /** 写入程序化网格，由单独的凸体承担碰撞。 */
  void Submit(UProceduralMeshComponent* Mesh,int32 Slot,UMaterialInterface* Material)
  {
   if(Vertices.IsEmpty()) { return; }
   Mesh->CreateMeshSection(Slot,Vertices,Indices,Normals,UV,Colors,Tangents,false);
   Mesh->SetMaterial(Slot,Material);
  }
 };
}

ALxSeawallActor::ALxSeawallActor()
{
 WallMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("SeawallMesh"));
 WallMesh->SetupAttachment(RoadSpline);
 WallMesh->SetMobility(EComponentMobility::Movable);
 WallMesh->bUseComplexAsSimpleCollision=false;
 WallMesh->SetCollisionProfileName(TEXT("BlockAll"));
 WallMesh->SetGenerateOverlapEvents(false);
 WallOptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("SeawallOptimizedMesh"));
 WallOptimizedMesh->SetupAttachment(WallMesh);
}

void ALxSeawallActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 if(WallOptimizedMesh && !WallOptimizedMesh->IsSourceCurrent()) { RebuildRoad(); }
}

void ALxSeawallActor::AcceptUnchangedRoadSource()
{
 Super::AcceptUnchangedRoadSource();
 if (WallOptimizedMesh) { WallOptimizedMesh->AcceptUnchangedSource(); }
}

void ALxSeawallActor::RebuildSeawall() { RebuildRoad(); }

void ALxSeawallActor::ClearRoad()
{
 Super::ClearRoad();
 GeneratedSlabCount=0; SeawallDiagnostic.Reset();
 if(WallMesh) { WallMesh->ClearAllMeshSections(); WallMesh->ClearCollisionConvexMeshes(); }
 if(WallOptimizedMesh) { WallOptimizedMesh->MarkSourceDirty(); WallOptimizedMesh->SetVisibility(false); }
}

void ALxSeawallActor::BuildRoadFoundation(const TArray<FLxRoadSampleFrame>& Frames,int32 MainVertexCount)
{
 if(!WallMesh) { return; }
 WallMesh->SetRelativeTransform(FTransform::Identity);
 WallOptimizedMesh->SetRelativeTransform(FTransform::Identity);
 ON_SCOPE_EXIT { WallOptimizedMesh->Synchronize(WallMesh); };
 const FProcMeshSection* Deck=RoadMesh->GetProcMeshSection(0);
 if(!Deck || Frames.Num()<2) { SeawallDiagnostic=GenerationDiagnostic; return; }
 /** 失败时清空整座海堤，防止留下看似可行走的悬空道路或旧碰撞。 */
 const auto Fail=[&](const TCHAR* Message)
 { ClearRoad(); SeawallDiagnostic=Message; GenerationDiagnostic=Message; };
 for(float V : {WallHeight,SeaSlopeAngle,LandBatterAngle,SlabLength,SlabRowHeight,SlabThickness,SlabGap})
 { if(!FMath::IsFinite(V)) { Fail(TEXT("海堤参数包含无效数值。")); return; } }
 const double Depth=FMath::Max(1.f,RoadDepth), Height=FMath::Clamp(WallHeight,50.f,5000.f);
 if(Height<=Depth+10) { Fail(TEXT("海堤高度必须大于路基厚度至少10厘米。")); return; }
 const double H=Height-Depth, Angle=FMath::DegreesToRadians(FMath::Clamp(SeaSlopeAngle,20.f,80.f));
 const double SeaRun=H/FMath::Tan(Angle), LandRun=H*FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(LandBatterAngle,0.f,10.f)));
 const double Sign=bSeaOnLeft ? -1. : 1.;
 const double W=FMath::Max(50.f,RoadWidth)*.5, CW=FMath::Max(5.f,CurbWidth);
 const double SeaWidth=W+((bSeaOnLeft ? bLeftCurb : bRightCurb) ? CW : 0);
 const double LandWidth=W+((bSeaOnLeft ? bRightCurb : bLeftCurb) ? CW : 0);
 for(int32 I=0;I+1<Frames.Num();++I)
 {
  const FVector Travel=(Frames[I+1].Position-Frames[I].Position)*FVector(1,1,0);
  for(double Offset : {Sign*(SeaWidth+SeaRun),-Sign*(LandWidth+LandRun)})
  {
   const FVector Edge=Travel+(Frames[I+1].Right-Frames[I].Right)*Offset;
   if(FVector::DotProduct(Travel,Edge)<=1.e-6)
   { Fail(TEXT("海堤坡脚在弯道内侧折叠，请增大弯曲半径、减小高度或增大海侧坡角。")); return; }
  }
 }
 const double Length=Frames.Last().Distance;
 const double SlopeLength=FMath::Sqrt(H*H+SeaRun*SeaRun);
 const double TileLength=FMath::Clamp(SlabLength,60.f,500.f);
 const int32 Rows=FMath::Max(1,FMath::RoundToInt(SlopeLength/FMath::Clamp(SlabRowHeight,60.f,500.f)));
 if((FMath::CeilToInt(Length/TileLength)+1)*Rows>12000)
 { Fail(TEXT("岩板超过12000块，请拆分海堤或增大岩板尺寸。")); return; }
 LxSeawall::FMesh Brick,Rock;
 TArray<TArray<FVector>> Hulls;
 /** 由道路同一截面得到陆顶、海顶、海底、陆底，避免折点开裂。 */
 const auto Section=[&](const FLxRoadSampleFrame& F)
 {
  return TArray<FVector>{F.Position-F.Right*(Sign*LandWidth)-FVector(0,0,Depth),
   F.Position+F.Right*(Sign*SeaWidth)-FVector(0,0,Depth),
   F.Position+F.Right*(Sign*(SeaWidth+SeaRun))-FVector(0,0,Height),
   F.Position-F.Right*(Sign*(LandWidth+LandRun))-FVector(0,0,Height)};
 };
 for(int32 I=0;I+1<Frames.Num();++I)
 {
  const auto A=Section(Frames[I]),B=Section(Frames[I+1]);
  const FVector Right=(Frames[I].Right+Frames[I+1].Right).GetSafeNormal()*Sign;
  const FVector Outs[4]={FVector::UpVector,Right*FMath::Sin(Angle)+FVector::UpVector*FMath::Cos(Angle),-FVector::UpVector,-Right};
  for(int32 K=0;K<4;++K)
  {
   const int32 J=(K+1)%4;
   const double V=FVector::Distance(A[K],A[J])/100.;
   const double U0=Frames[I].Distance/100.,U1=Frames[I+1].Distance/100.;
   (K==1 ? Rock : Brick).Quad(A[K],B[K],B[J],A[J],{U0,0},{U1,0},{U1,V},{U0,V},Outs[K]);
  }
  if(bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this)) { TArray<FVector> Hull=A; Hull.Append(B); Hulls.Add(MoveTemp(Hull)); }
 }
 if(!RoadSpline->IsClosedLoop())
 {
  for(int32 End : {0,1})
  {
   const auto P=Section(End ? Frames.Last() : Frames[0]);
   const FVector Out=RoadSpline->GetDirectionAtDistanceAlongSpline(End ? Length : 0,ESplineCoordinateSpace::Local)*(End ? 1 : -1);
   Brick.Quad(P[0],P[1],P[2],P[3],{0,0},{(LandWidth+SeaWidth)/100.,0},{(LandWidth+SeaWidth+SeaRun)/100.,H/100.},{-LandRun/100.,H/100.},Out);
  }
 }
 /** 用共享截面插值而非重新取样，确保每块岩板贴合曲面基层。 */
 const auto Surface=[&](double Distance,double V,double Lift)
 {
  const int32 Upper=FMath::Clamp(Algo::LowerBoundBy(Frames,Distance,[](const FLxRoadSampleFrame& F){ return F.Distance; }),1,Frames.Num()-1);
  const auto& A=Frames[Upper-1]; const auto& B=Frames[Upper];
  const double T=FMath::Clamp((Distance-A.Distance)/(B.Distance-A.Distance),0.,1.);
  const FVector Right=FMath::Lerp(A.Right,B.Right,T);
  const FVector Normal=Right.GetSafeNormal()*Sign*FMath::Sin(Angle)+FVector::UpVector*FMath::Cos(Angle);
  return FMath::Lerp(A.Position,B.Position,T)+Right*(Sign*(SeaWidth+SeaRun*V/SlopeLength))
   -FVector(0,0,Depth+H*V/SlopeLength)+Normal*Lift;
 };
 FRandomStream Random(StoneSeed);
 const double Gap=FMath::Clamp(SlabGap,.5f,6.f), Thickness=FMath::Clamp(SlabThickness,2.f,15.f);
 for(int32 Row=0;!LxGeneratedMeshBuild::IsPreview(this) && Row<Rows;++Row)
 {
  const double V0=SlopeLength*Row/Rows+Gap*.5,V1=SlopeLength*(Row+1)/Rows-Gap*.5;
  for(double Start=(Row%2 ? -TileLength*.5 : 0.);Start<Length;Start+=TileLength)
  {
   const double U0=FMath::Max(static_cast<double>(Frames[0].Distance),Start)+Gap*.5,U1=FMath::Min(Length,Start+TileLength)-Gap*.5;
   if(U1-U0<Gap+2) { continue; }
   const double Bevel=FMath::Min3(3.,(U1-U0)*.12,(V1-V0)*.12);
   const double Cut=Bevel*1.8, Lift=Thickness*Random.FRandRange(.85f,1.15f);
   const uint8 Tone=static_cast<uint8>(Random.RandRange(225,255));
   Rock.StoneColor=FColor(Tone,Tone,Tone);
   // 八边形切角底环和内缩顶环组成真实倒角，板厚略有变化但保持整洁。
   const FVector2D Ring[8]={{U0+Cut,V0},{U1-Cut,V0},{U1,V0+Cut},{U1,V1-Cut},
    {U1-Cut,V1},{U0+Cut,V1},{U0,V1-Cut},{U0,V0+Cut}};
   FVector Lower[8],Upper[8]; FVector2D TopUV[8];
   const FVector2D Center((U0+U1)*.5,(V0+V1)*.5);
   const FVector Normal=(Surface(Center.X,Center.Y,1)-Surface(Center.X,Center.Y,0)).GetSafeNormal();
   const FVector C=Surface(Center.X,Center.Y,Lift+Random.FRandRange(.2f,1.1f));
   for(int32 K=0;K<8;++K)
   {
    TopUV[K]=Ring[K]+(Center-Ring[K]).GetSafeNormal()*Bevel;
    Lower[K]=Surface(Ring[K].X,Ring[K].Y,.25);
    Upper[K]=Surface(TopUV[K].X,TopUV[K].Y,Lift+Random.FRandRange(-.4f,.4f));
   }
   for(int32 K=0;K<8;++K)
   {
    const int32 J=(K+1)%8;
    Rock.Triangle(C,Upper[K],Upper[J],Center/100.,TopUV[K]/100.,TopUV[J]/100.,Normal);
    const FVector Out=(Lower[K]+Lower[J])*.5-Surface(Center.X,Center.Y,0);
    Rock.Quad(Lower[K],Lower[J],Upper[J],Upper[K],Ring[K]/100.,Ring[J]/100.,TopUV[J]/100.,TopUV[K]/100.,Out);
   }
   ++GeneratedSlabCount;
  }
 }
 /** 连接片边界的焊接键；丢弃内部共边，避免隐藏的重叠侧面与多余三角形。 */
 using FEdgeKey=TPair<FIntVector,FIntVector>;
 /** 保存连接片单边的位置和出现次数。 */
 struct FSupportEdge
 {
  /** 边界起点。 */
  FVector A;
  /** 边界终点。 */
  FVector B;
  /** 从三角形内部指向外侧的向量。 */
  FVector Out;
  /** 共边出现次数。 */
  int32 Count=0;
 };
 TMap<FEdgeKey,FSupportEdge> SupportEdges;
 /** 毫米以下精度的顶点焊接坐标。 */
 const auto Weld=[](FVector P) { return FIntVector(FMath::RoundToInt(P.X*100),FMath::RoundToInt(P.Y*100),FMath::RoundToInt(P.Z*100)); };
 // 路口和端点连接片沿各自顶面向下生成砌石支撑，跟随邻路的移动、断开和改宽。
 for(int32 I=0;I+2<Deck->ProcIndexBuffer.Num();I+=3)
 {
  const int32 IA=Deck->ProcIndexBuffer[I],IB=Deck->ProcIndexBuffer[I+1],IC=Deck->ProcIndexBuffer[I+2];
  if(IA<MainVertexCount || Deck->ProcVertexBuffer[IA].Normal.Z<.5) { continue; }
  const FVector P[3]={FVector(Deck->ProcVertexBuffer[IA].Position),FVector(Deck->ProcVertexBuffer[IB].Position),FVector(Deck->ProcVertexBuffer[IC].Position)};
  TArray<FVector> Hull;
  for(FVector V : P) { Hull.Add(V-FVector(0,0,Depth)); Hull.Add(V-FVector(0,0,Height)); }
  if(bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this)) { Hulls.Add(Hull); }
  for(int32 K=0;K<3;++K)
  {
   const int32 J=(K+1)%3;
   FIntVector A=Weld(P[K]),B=Weld(P[J]);
   if(A.X>B.X || (A.X==B.X && (A.Y>B.Y || (A.Y==B.Y && A.Z>B.Z)))) { Swap(A,B); }
   FSupportEdge& Edge=SupportEdges.FindOrAdd({A,B});
   Edge.A=P[K]; Edge.B=P[J]; Edge.Out=(P[K]+P[J])*.5-(P[0]+P[1]+P[2])/3.; ++Edge.Count;
  }
  Brick.Triangle(Hull[1],Hull[3],Hull[5],{P[0].X/100.,P[0].Y/100.},{P[1].X/100.,P[1].Y/100.},{P[2].X/100.,P[2].Y/100.},-FVector::UpVector);
 }
 for(const auto& Pair : SupportEdges)
 {
  const FSupportEdge& E=Pair.Value;
  if(E.Count!=1) { continue; }
  const double U=FVector::Distance(E.A,E.B)/100.;
  Brick.Quad(E.A-FVector(0,0,Depth),E.B-FVector(0,0,Depth),E.B-FVector(0,0,Height),E.A-FVector(0,0,Height),
   {0,0},{U,0},{U,H/100.},{0,H/100.},E.Out);
 }
 Brick.Submit(WallMesh,0,LandBrickMaterial); Rock.Submit(WallMesh,1,SeaStoneMaterial);
 WallMesh->SetCastShadow(bCastShadow);
 WallMesh->SetCollisionEnabled(bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this) ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
 if(bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this)) { WallMesh->SetCollisionConvexMeshes(Hulls); }
}
