#include "LxRoadConnectionSolver.h"
#include "LxSplineRoadActor.h"
#include "../StoneTerrace/LxStoneTerraceActor.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"

bool FLxRoadEndpoint::operator==(const FLxRoadEndpoint& Other) const
{
 return FMath::IsNearlyEqual(TerraceTrimMeters,Other.TerraceTrimMeters,.00001f) && bConnected==Other.bConnected && Position.Equals(Other.Position,0.001)
  && Outward.Equals(Other.Outward,0.00001) && FMath::IsNearlyEqual(HalfWidth,Other.HalfWidth,0.001f);
}

bool FLxRoadAutoJunction::operator==(const FLxRoadAutoJunction& Other) const
{
 return Side==Other.Side && Interval.Equals(Other.Interval,0.001) && Before==Other.Before && After==Other.After
  && bConnected==Other.bConnected && FMath::IsNearlyEqual(Distance,Other.Distance,0.001f)
  && FMath::IsNearlyEqual(HalfWidth,Other.HalfWidth,0.001f) && FMath::IsNearlyEqual(Radius,Other.Radius,0.001f)
  && FMath::IsNearlyEqual(Length,Other.Length,0.001f) && Outlet.Equals(Other.Outlet,0.001)
  && Outward.Equals(Other.Outward,0.00001) && EndDirection.Equals(Other.EndDirection,0.00001);
}

namespace LxRoadConnections
{
 /** 一次求解中的道路快照和输出，避免求解过程中修改源路径。 */
 struct FRoad
 {
  /** 所属道路。 */
  ALxSplineRoadActor* Actor=nullptr;
  /** 是否满足吸附条件。 */
  bool bEligible=false;
  /** 用于排除远处道路，避免对所有曲线执行最近点搜索。 */
  FBox Bounds=FBox(ForceInit);
  /** 起终点世界位置。 */
  FVector Points[2];
  /** 端点向外切线。 */
  FVector Outward[2];
  /** 求解后的端点。 */
  FLxRoadEndpoint Ends[2];
  /** 求解后的左右开口。 */
  TArray<FLxRoadAutoJunction> Sides;
  /** 双向可编辑记录。 */
  TArray<FLxRoadConnectionSettings> Settings;
 };
 /** 可用连接候选，按距离从近到远分配端口。 */
 struct FCandidate
 {
  /** 起始道路和端点。 */
  int32 A=0, EndA=0;
  /** 目标道路和端口，二三表示左右侧。 */
  int32 B=0, PortB=0;
  /** 水平间隙。 */
  double Gap=0;
  /** 侧面连接沿目标道路的距离。 */
  float Distance=0;
  /** 主路中心和朝外方向。 */
  FVector Center=FVector::ZeroVector, Normal=FVector::ZeroVector;
 };

 /** 查找已有共享参数，保留重建前的用户设置。 */
 FLxRoadConnectionSettings SettingsFor(ALxSplineRoadActor* A, int32 PortA, ALxSplineRoadActor* B, int32 PortB)
 {
  const FLxRoadConnectionSettings* Found=A->RoadConnections.FindByPredicate([&](const FLxRoadConnectionSettings& S)
  { return S.OtherRoad==B && S.LocalPort==PortA && S.OtherPort==PortB; });
  FLxRoadConnectionSettings Result=Found ? *Found : FLxRoadConnectionSettings();
  Result.OtherRoad=B; Result.RoadName=B->GetActorNameOrLabel(); Result.LocalPort=PortA; Result.OtherPort=PortB;
  Result.bJunction=PortA>=2 || PortB>=2;
  if(!FMath::IsFinite(Result.Width)) { Result.Width=0; }
  if(!FMath::IsFinite(Result.Radius)) { Result.Radius=150; }
  if(!FMath::IsFinite(Result.Length)) { Result.Length=0; }
  Result.Width=FMath::Max(0,Result.Width); Result.Radius=FMath::Max(0,Result.Radius); Result.Length=FMath::Max(0,Result.Length);
  return Result;
 }

 /** 在两条射线之间构造圆角，按可用长度限制半径，保留实际端点。 */
 TArray<FVector> CornerArc(FVector Corner,FVector First,FVector Second,double Radius,double Available)
 {
  TArray<FVector> Result;
  const double Cos=FMath::Clamp(FVector::DotProduct(First,Second),-0.9999,0.9999);
  const double HalfAngle=FMath::Acos(Cos)*0.5;
  const double Offset=FMath::Min(Radius/FMath::Tan(HalfAngle),FMath::Max(0.,Available));
  Radius=Offset*FMath::Tan(HalfAngle);
  if(Radius<0.01) { Result.Add(Corner); return Result; }
  const FVector Center=Corner+(First+Second).GetSafeNormal2D()*(Radius/FMath::Sin(HalfAngle));
  const FVector A=Corner+First*Offset-Center, B=Corner+Second*Offset-Center;
  const double Angle=FMath::Atan2(FVector::CrossProduct(A,B).Z,FVector::DotProduct(A,B));
  const int32 Steps=FMath::Clamp(FMath::CeilToInt(FMath::Abs(Angle)*Radius/20),8,48);
  for(int32 I=0; I<=Steps; ++I) { Result.Add(Center+A.RotateAngleAxis(FMath::RadiansToDegrees(Angle*I/Steps),FVector::UpVector)); }
  return Result;
 }

 /** 以接入道路的真实端面构造斜向路口轮廓，全程不移动或旋转道路。 */
 bool JunctionBoundary(const FRoad& A,const FRoad& B,const FCandidate& C,const FLxRoadConnectionSettings& Settings,FLxRoadAutoJunction& J)
 {
  USplineComponent* S=B.Actor->RoadSpline;
  const FVector T=S->GetDirectionAtDistanceAlongSpline(C.Distance,ESplineCoordinateSpace::World).GetSafeNormal2D();
  const FVector D=A.Outward[C.EndA], E=A.Points[C.EndA];
  const double Sin=FVector::DotProduct(D,-C.Normal);
  if(Sin<0.173648) { return false; } // 与主路近乎平行时不生成退化开口。
  const double W=FMath::Max(50.f,B.Actor->RoadWidth)*0.5, HW=FMath::Max(50.f,A.Actor->RoadWidth)*0.5;
  const FVector Edge=C.Center+C.Normal*W;
  FVector Across(-D.Y,D.X,0);
  if(FVector::DotProduct(Across,T)<0) { Across=-Across; }
  const double Gap=FVector::DotProduct(E-Edge,C.Normal)/Sin;
  if(Gap<0.1) { return false; }
  const double Half=Settings.Width>0 ? FMath::Max(50.f,Settings.Width)*0.5 : HW;
  // 自定义开口宽度在连接片内部过渡，支路端面宽度和位置保持不变。
  const double Transition=FMath::Abs(Half-HW)>0.01 ? FMath::Min(Gap*0.4,Settings.Length>0 ? Settings.Length : Gap*0.4) : 0;
  const FVector Inner=E+D*Transition;
  const FVector Low=Inner-Across*Half, High=Inner+Across*Half;
  const double LowRun=FVector::DotProduct(Low-Edge,C.Normal)/Sin;
  const double HighRun=FVector::DotProduct(High-Edge,C.Normal)/Sin;
  if(FMath::Min(LowRun,HighRun)<0.1) { return false; }
  const FVector LowCorner=Low+D*LowRun, HighCorner=High+D*HighRun;
  const double Limit=Settings.Length>0 ? Settings.Length : UE_BIG_NUMBER;
  J.Side=C.PortB-2; J.Distance=C.Distance; J.HalfWidth=Half; J.Radius=Settings.Radius;
  J.bConnected=true; J.Outlet=E; J.Outward=-D;
  J.Before=CornerArc(LowCorner,-T,-D,Settings.Radius,FMath::Min(LowRun,Limit));
  J.Before.AddUnique(Low); J.Before.AddUnique(E-Across*HW);
  J.After.Add(E+Across*HW); J.After.AddUnique(High);
  const TArray<FVector> AfterArc=CornerArc(HighCorner,-D,T,Settings.Radius,FMath::Min(HighRun,Limit));
  for(FVector P : AfterArc) { J.After.AddUnique(P); }
  J.Interval=FVector2D(C.Distance+FVector::DotProduct(J.Before[0]-Edge,T),C.Distance+FVector::DotProduct(J.After.Last()-Edge,T));
  const double Margin=FMath::Max(5.f,B.Actor->CurbWidth)*2+1;
  if(J.Interval.X<Margin || J.Interval.Y>S->GetSplineLength()-Margin || J.Interval.Y-J.Interval.X<1) { return false; }
  // 曲线主路端点精确投到真实边缘，高度沿轮廓逐段过渡到支路端面。
  for(int32 End=0; End<2; ++End)
  {
   const float Distance=End ? J.Interval.Y : J.Interval.X;
   const FVector Center=S->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);
   const FVector Tangent=S->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World).GetSafeNormal2D();
   const FVector Anchor=Center+FVector(-Tangent.Y,Tangent.X,0)*((J.Side==0 ? -1 : 1)*W);
   TArray<FVector>& Path=End ? J.After : J.Before;
   (End ? Path.Last() : Path[0])=Anchor;
   for(int32 I=1; I+1<Path.Num(); ++I)
   {
    // 将圆角靠主路的部分随真实曲线偏移，避免切线平面上的圆角侵入弯道路面。
    const double Along=FVector::DotProduct(Path[I]-Edge,T);
    const float SampleDistance=FMath::Clamp(static_cast<float>(C.Distance+Along),0.f,S->GetSplineLength());
    const FVector CurveCenter=S->GetLocationAtDistanceAlongSpline(SampleDistance,ESplineCoordinateSpace::World);
    const FVector CurveT=S->GetDirectionAtDistanceAlongSpline(SampleDistance,ESplineCoordinateSpace::World).GetSafeNormal2D();
    const FVector CurveEdge=CurveCenter+FVector(-CurveT.Y,CurveT.X,0)*((J.Side==0 ? -1 : 1)*W);
    const double Fade=1-FMath::Clamp(FVector::DotProduct(Path[I]-Edge,C.Normal)/FMath::Max(1.,Gap*Sin),0.,1.);
    Path[I]+=(CurveEdge-(Edge+T*Along))*Fade*Fade;
    const double Alpha=FVector::Dist2D(Path[I],Anchor)/FMath::Max(0.01,FVector::Dist2D(End ? Path[0] : Path.Last(),Anchor));
    Path[I].Z=FMath::Lerp(Anchor.Z,E.Z,FMath::Clamp(Alpha,0.,1.));
   }
  }
  return true;
 }
}

void FLxRoadConnectionSolver::Refresh(AActor& Changed, bool bRemoving)
{
 using namespace LxRoadConnections;
 // 构造、移动、撤销、保存均走同一入口；禁止邻居递归触发全世界重建。
 static bool bRefreshing=false;
 if(bRefreshing || !Changed.GetWorld() || Changed.GetWorld()->bIsTearingDown || Changed.IsTemplate()) { return; }
 TGuardValue<bool> Guard(bRefreshing,true);
 TArray<ALxSplineRoadActor*> Actors;
 for(TActorIterator<ALxSplineRoadActor> It(Changed.GetWorld()); It; ++It)
 { if(IsValid(*It) && !It->IsActorBeingDestroyed() && !(bRemoving && *It==&Changed)) { Actors.Add(*It); } }
 Actors.Sort([](const ALxSplineRoadActor& A,const ALxSplineRoadActor& B)
 {
  if(A.GetRoadConnectionPriority()!=B.GetRoadConnectionPriority())
  { return A.GetRoadConnectionPriority()>B.GetRoadConnectionPriority(); }
  return A.GetPathName()<B.GetPathName();
 });
 TArray<FRoad> Roads;
 for(ALxSplineRoadActor* Actor : Actors)
 {
  FRoad& R=Roads.AddDefaulted_GetRef(); R.Actor=Actor;
  USplineComponent* S=Actor->RoadSpline;
  Actor->ConnectionDiagnostic.Reset();
  if(!Actor->bAutoConnect || !S || S->GetNumberOfSplinePoints()<2) { continue; }
  if(!S->GetComponentScale().Equals(FVector::OneVector,0.001)
   || FVector::DotProduct(S->GetUpVector(),FVector::UpVector)<0.9999)
  { Actor->ConnectionDiagnostic=TEXT("自动连接需要单位缩放和水平旋转；坡度请通过样条控制点高度设置。"); continue; }
  if(!FMath::IsFinite(Actor->RoadWidth) || !FMath::IsFinite(Actor->AutoConnectDistance)
   || !FMath::IsFinite(Actor->AutoConnectHeight) || !FMath::IsFinite(S->GetSplineLength()) || S->GetSplineLength()<1) { continue; }
  R.bEligible=true;
  R.Bounds=S->CalcBounds(S->GetComponentTransform()).GetBox();
  for(int32 End=0; End<2; ++End)
  {
   const int32 Index=End ? S->GetNumberOfSplinePoints()-1 : 0;
   R.Points[End]=S->GetLocationAtSplinePoint(Index,ESplineCoordinateSpace::World);
   R.Outward[End]=S->GetDirectionAtSplinePoint(Index,ESplineCoordinateSpace::World).GetSafeNormal2D()*(End ? 1 : -1);
  }
 }
 TArray<FCandidate> Candidates;
 constexpr double MinCos=0.173648; // 允许斜向接入，仅排除近乎平行的退化连接。
 for(int32 A=0; A<Roads.Num(); ++A)
 {
  const FRoad& RA=Roads[A];
  if(!RA.bEligible || RA.Actor->RoadSpline->IsClosedLoop()) { continue; }
  for(int32 B=0; B<Roads.Num(); ++B)
  {
   const FRoad& RB=Roads[B];
   if(A==B || !RB.bEligible) { continue; }
   const double Range=FMath::Max(0.f,FMath::Min(RA.Actor->AutoConnectDistance,RB.Actor->AutoConnectDistance));
   const double Height=FMath::Max(0.f,FMath::Min(RA.Actor->AutoConnectHeight,RB.Actor->AutoConnectHeight));
   for(int32 EA=0; EA<2; ++EA)
   {
    const FBox SearchBounds=RB.Bounds.ExpandBy(FVector(Range+FMath::Max(50.f,RB.Actor->RoadWidth)*0.5,
     Range+FMath::Max(50.f,RB.Actor->RoadWidth)*0.5,Height));
    if(!SearchBounds.IsInsideOrOn(RA.Points[EA])) { continue; }
    if(A<B && !RB.Actor->RoadSpline->IsClosedLoop())
    {
     for(int32 EB=0; EB<2; ++EB)
     {
      const double Gap=FVector::Dist2D(RA.Points[EA],RB.Points[EB]);
      if(Gap<=Range && FMath::Abs(RA.Points[EA].Z-RB.Points[EB].Z)<=Height
       && FVector::DotProduct(RA.Outward[EA],-RB.Outward[EB])>=MinCos)
      { Candidates.Add({A,EA,B,EB,Gap}); }
     }
    }
    USplineComponent* S=RB.Actor->RoadSpline;
    float Distance=S->GetDistanceAlongSplineAtSplineInputKey(S->FindInputKeyClosestToWorldLocation(RA.Points[EA]));
    FVector Center,N,T; int32 Side=0;
    double Gap=0;
    for(int32 Iteration=0; Iteration<5; ++Iteration)
    {
     Center=S->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World);
     T=S->GetDirectionAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::World).GetSafeNormal2D();
     N=FVector(-T.Y,T.X,0);
     const double Lateral=FVector::DotProduct(RA.Points[EA]-Center,N);
     Side=Lateral<0 ? 0 : 1; N*=Side==0 ? -1 : 1;
     Gap=FMath::Abs(Lateral)-FMath::Max(50.f,RB.Actor->RoadWidth)*0.5;
     const double Into=FVector::DotProduct(RA.Outward[EA],-N);
     if(Into<MinCos || Iteration==4) { break; }
     const FVector Intersection=RA.Points[EA]+RA.Outward[EA]*(Gap/Into);
     Distance=S->GetDistanceAlongSplineAtSplineInputKey(S->FindInputKeyClosestToWorldLocation(Intersection));
    }
    if(Gap<0.1 || Gap>Range || FMath::Abs(RA.Points[EA].Z-Center.Z)>Height
     || FVector::DotProduct(RA.Outward[EA],-N)<MinCos) { continue; }
    Candidates.Add({A,EA,B,Side+2,FMath::Abs(Gap),Distance,Center,N});
   }
  }
 }
 Candidates.StableSort([](const FCandidate& A,const FCandidate& B){ return A.Gap<B.Gap; });
 for(const FCandidate& C : Candidates)
 {
  FRoad& A=Roads[C.A]; FRoad& B=Roads[C.B];
  if(A.Ends[C.EndA].bConnected) { continue; }
  if(C.PortB<2 && B.Ends[C.PortB].bConnected) { continue; }
  const int32 Side=C.PortB-2;
  if(C.PortB>=2 && (Side==0 ? B.Actor->bLeftJunction : B.Actor->bRightJunction)) { continue; }
  FLxRoadConnectionSettings Shared=SettingsFor(A.Actor,C.EndA,B.Actor,C.PortB);
  FLxRoadAutoJunction Junction;
  if(C.PortB<2)
  {
   // 端点连接只填补两条道路之间的空隙，双方原端面与切线均不变化。
   const FVector Delta=B.Points[C.PortB]-A.Points[C.EndA];
   if(FVector::DotProduct(Delta,A.Outward[C.EndA])<-0.1
    || FVector::DotProduct(-Delta,B.Outward[C.PortB])<-0.1) { continue; }
   const FVector AcrossA(-A.Outward[C.EndA].Y,A.Outward[C.EndA].X,0);
   FVector AcrossB(-B.Outward[C.PortB].Y,B.Outward[C.PortB].X,0);
   if(FVector::DotProduct(AcrossA,AcrossB)<0) { AcrossB=-AcrossB; }
   const double HA=FMath::Max(50.f,A.Actor->RoadWidth)*0.5, HB=FMath::Max(50.f,B.Actor->RoadWidth)*0.5;
   Junction.Side=-1; Junction.bConnected=true; Junction.Outward=A.Outward[C.EndA];
   Junction.EndDirection=-B.Outward[C.PortB];
   const double Span=Delta.Size();
   // 切线长度使用间隙在道路方向上的投影，避免横向错位较大时连接曲线倒退。
   const FVector StartTangent=Junction.Outward*FMath::Max(0.,FVector::DotProduct(Delta,Junction.Outward));
   const FVector EndTangent=Junction.EndDirection*FMath::Max(0.,FVector::DotProduct(Delta,Junction.EndDirection));
   const int32 Steps=FMath::Clamp(FMath::CeilToInt((Span+FMath::Abs(HA-HB))*0.2),24,128);
   /** 平滑宽度变化，两端导数为零，使路边与原道路相切。 */
   const auto Smooth=[](double T){ return T*T*T*(10+T*(-15+6*T)); };
   for(int32 I=0; I<=Steps; ++I)
   {
    // 端部加密采样，避免有限分段重新产生明显折角。
    const double T=(1-FMath::Cos(PI*I/Steps))*0.5, Blend=Smooth(T);
    // 中心线与宽度共用五次插值：单侧对齐时横向偏移精确抵消，直边不再起伏。
    // 两端保留道路切向并令二阶导数为零，另一侧仍然平滑变宽。
    const double T3=T*T*T, T4=T3*T, T5=T4*T;
    const FVector Center=A.Points[C.EndA]+Delta*Blend
     +StartTangent*(T-6*T3+8*T4-3*T5)+EndTangent*(-4*T3+7*T4-3*T5);
    const FVector Across=FMath::Lerp(AcrossA,AcrossB,Blend).GetSafeNormal2D();
    double Half=FMath::Lerp(HA,HB,Blend);
    if(Shared.Width>0)
    {
     const double MiddleHalf=FMath::Max(50.f,Shared.Width)*0.5;
     Half=T<=0.5 ? FMath::Lerp(HA,MiddleHalf,Smooth(T*2)) : FMath::Lerp(MiddleHalf,HB,Smooth(T*2-1));
    }
    Junction.Before.Add(Center-Across*Half);
    Junction.After.Insert(Center+Across*Half,0);
   }
   A.Sides.Add(Junction);
   B.Ends[C.PortB]={true,B.Points[C.PortB],B.Outward[C.PortB],static_cast<float>(HB)};
  }
  else
  {
   if(!JunctionBoundary(A,B,C,Shared,Junction))
   { A.Actor->ConnectionDiagnostic=TEXT("斜向路口空间不足，请将支路端点移到主路外侧或减小开口宽度、圆角。"); continue; }
   const double Spacing=FMath::Max(5.f,B.Actor->CurbWidth)*2;
   if(B.Sides.ContainsByPredicate([&](const FLxRoadAutoJunction& Existing)
    { return Existing.Side==Side && Junction.Interval.X<Existing.Interval.Y+Spacing && Junction.Interval.Y>Existing.Interval.X-Spacing; }))
   { A.Actor->ConnectionDiagnostic=TEXT("路口与已有开口重叠，请沿主路移开一些。"); continue; }
   B.Sides.Add(Junction);
  }
  A.Ends[C.EndA]={true,A.Points[C.EndA],A.Outward[C.EndA],FMath::Max(50.f,A.Actor->RoadWidth)*0.5f};
  A.Settings.Add(Shared);
  Shared.OtherRoad=A.Actor; Shared.RoadName=A.Actor->GetActorNameOrLabel(); Shared.LocalPort=C.PortB; Shared.OtherPort=C.EndA;
  B.Settings.Add(Shared);
 }
 // 台地复用未占用端口；道路和海堤均保留原始样条，连接片由台地生成。
 ALxStoneTerraceActor::SolveConnections(Changed,bRemoving,[&](ALxSplineRoadActor& Actor,int32 End,bool bClaim,float TrimMeters)
 {
  FRoad* Road=Roads.FindByPredicate([&](const FRoad& R){return R.Actor==&Actor;});
  if(!Road || !Road->bEligible || Road->Ends[End].bConnected) { return false; }
  if(bClaim) { Road->Ends[End]={true,Road->Points[End],Road->Outward[End],FMath::Max(50.f,Actor.RoadWidth)*.5f,TrimMeters}; }
  return true;
 });
 for(FRoad& R : Roads)
 {
  bool bChanged=R.Actor==&Changed || R.Sides!=R.Actor->ConnectedJunctions;
  for(int32 I=0; I<2; ++I)
  {
   bChanged|=!(R.Ends[I]==R.Actor->ConnectedEndpoints[I]);
  }
#if WITH_EDITOR
  // 相邻道路可能属于独立外部演员包，也必须参与撤销与后续保存。
  if(bChanged && !Changed.GetWorld()->IsGameWorld() && !UE::IsSavingPackage()) { R.Actor->Modify(); }
#endif
  for(int32 I=0; I<2; ++I)
  {
   R.Actor->ConnectedEndpoints[I]=R.Ends[I];
  }
  R.Actor->ConnectedJunctions=MoveTemp(R.Sides);
  R.Actor->RoadConnections=MoveTemp(R.Settings);
  if(bChanged)
  {
   if(R.Actor!=&Changed && LxGeneratedMeshBuild::IsPreview(&Changed)) { LxGeneratedMeshBuild::RequestPreview(R.Actor); }
   R.Actor->BuildRoadGeometry();
  }
 }
}
