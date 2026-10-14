// Copyright Epic Games, Inc. All Rights Reserved.
#include "LxSplineRoadActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "Misc/ScopeExit.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Engine/Engine.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Algo/Reverse.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace LxRoad
{
 /** 已约束到道路范围内的单侧路口。 */
 struct FJunction
 {
  /** 是否参与生成。 */
  bool bEnabled = false;
  /** 中心线距离。 */
  float Center = 0;
  /** 支路半宽。 */
  float HalfWidth = 0;
  /** 有效圆弧半径。 */
  float Radius = 0;
  /** 从主路边缘起算的支路长度。 */
  float Length = 0;
  /** 自动连接使用平直出口，与接入道路共享精确接缝。 */
  bool bStraightOutlet = false;
  /** 出口中心的道路局部坐标。 */
  FVector Outlet = FVector::ZeroVector;
  /** 沿主路正向的出口横轴。 */
  FVector OutletAxis = FVector::ForwardVector;
  /** 判断某个截面间隔是否位于开口内部。 */
  bool Contains(float Distance) const { return bEnabled && FMath::Abs(Distance-Center)<HalfWidth+Radius; }
  /** 圆角填充在指定截面向外延伸的距离。 */
  float ArcExtent(float Distance) const
  {
   // 在支路侧边精确闭合圆弧，避免平方根放大距离相减的浮点误差。
   if(FMath::IsNearlyEqual(Distance,Center-HalfWidth,0.0001f)
    || FMath::IsNearlyEqual(Distance,Center+HalfWidth,0.0001f)) { return Radius; }
   const float X=FMath::Clamp(HalfWidth+Radius-FMath::Abs(Distance-Center),0.0f,Radius);
   return Radius-FMath::Sqrt(FMath::Max(0.0f,Radius*Radius-X*X));
  }
 };
 /** 单个采样截面的位置、横向斜接向量和中心线累计距离。 */
 using FFrame=FLxRoadSampleFrame;
 /** 一套材质槽的顶点、绕序、UV及显式硬边法线。 */
 struct FMesh
 {
  /** 网格顶点。 */
  TArray<FVector> Vertices;
  /** 三角形顺时针索引。 */
  TArray<int32> Triangles;
  /** 顶点法线。 */
  TArray<FVector> Normals;
  /** 顶点UV。 */
  TArray<FVector2D> UV;
  /** 沿UV横轴的切线。 */
  TArray<FProcMeshTangent> Tangents;

  /** 添加非退化三角形，圆弧与主路相切处允许四边形退化成三角形。 */
  void Triangle(const FVector& A, const FVector& B, const FVector& C,
   const FVector2D& UA, const FVector2D& UB, const FVector2D& UC)
  {
   const FVector Cross=FVector::CrossProduct(C-A,B-A);
   if(Cross.SizeSquared()<1.e-12) { return; }
   const int32 Base=Vertices.Num();
   const FVector N=Cross.GetSafeNormal();
   const FVector2D DU=UB-UA, DV=UC-UA;
   const double Det=DU.X*DV.Y-DU.Y*DV.X;
   FVector T=FMath::Abs(Det)>UE_SMALL_NUMBER ? ((B-A)*DV.Y-(C-A)*DU.Y)/Det : B-A;
   T=(T-N*FVector::DotProduct(T,N)).GetSafeNormal();
   const FVector Bitangent=FMath::Abs(Det)>UE_SMALL_NUMBER ? ((C-A)*DU.X-(B-A)*DV.X)/Det : C-A;
   Vertices.Append({A,B,C}); UV.Append({UA,UB,UC}); Triangles.Append({Base,Base+1,Base+2});
   for(int32 I=0; I<3; ++I)
   { Normals.Add(N); Tangents.Emplace(T,FVector::DotProduct(FVector::CrossProduct(N,T),Bitangent)<0); }
  }

  /** 添加顺时针四边形，直接计算硬边法线，避免重复顶点的全局平方级搜索。 */
  void Quad(const FVector& A, const FVector& B, const FVector& C, const FVector& D,
   const FVector2D& UA, const FVector2D& UB, const FVector2D& UC, const FVector2D& UD)
  {
   const int32 Base = Vertices.Num();
   Vertices.Append({A, B, C, D});
   UV.Append({UA, UB, UC, UD});
   Triangles.Append({Base, Base+1, Base+2, Base, Base+2, Base+3});
   const FVector N = FVector::CrossProduct(C-A, B-A).GetSafeNormal();
   const FVector2D DU = UB-UA, DV = UC-UA;
   const double Det = DU.X*DV.Y - DU.Y*DV.X;
   FVector T = FMath::Abs(Det) > UE_SMALL_NUMBER
    ? ((B-A)*DV.Y - (C-A)*DU.Y)/Det : (D-A);
   T = (T - N*FVector::DotProduct(T,N)).GetSafeNormal();
   const FVector Bitangent = FMath::Abs(Det) > UE_SMALL_NUMBER
    ? ((C-A)*DU.X-(B-A)*DV.X)/Det : (B-A);
   const bool bFlip = FVector::DotProduct(FVector::CrossProduct(N,T),Bitangent) < 0;
   for(int32 I=0; I<4; ++I) { Normals.Add(N); Tangents.Emplace(T,bFlip); }
  }

  /** 将一个槽提交到程序化网格体，碰撞由调用方统一提交。 */
  void Submit(UProceduralMeshComponent* Mesh, int32 Slot, UMaterialInterface* Material)
  {
   Mesh->CreateMeshSection_LinearColor(Slot,Vertices,Triangles,Normals,UV,TArray<FLinearColor>(),Tangents,false);
   Mesh->SetMaterial(Slot,Material);
  }
 };

 /** 自适应采样单段曲线；保留控制点边界并限制总采样量。 */
 bool Sample(const USplineComponent* Spline, float A, float B, float Spacing, int32 Depth, TArray<float>& Distances)
 {
  if (Distances.Num() >= 4096) { return false; }
  const float Mid = (A+B)*0.5f;
  const FVector PA = Spline->GetLocationAtDistanceAlongSpline(A,ESplineCoordinateSpace::Local);
  const FVector PB = Spline->GetLocationAtDistanceAlongSpline(B,ESplineCoordinateSpace::Local);
  const FVector PM = Spline->GetLocationAtDistanceAlongSpline(Mid,ESplineCoordinateSpace::Local);
  const FVector DA = (PM-PA).GetSafeNormal(), DB = (PB-PM).GetSafeNormal();
  const bool bSplit = B-A > Spacing || FVector::Distance(PM,(PA+PB)*0.5) > 2.0
   || FVector::DotProduct(DA,DB) < 0.9986;
  if (bSplit && Depth < 12 && B-A > 1.0f)
  {
   return Sample(Spline,A,Mid,Spacing,Depth+1,Distances) && Sample(Spline,Mid,B,Spacing,Depth+1,Distances);
  }
  Distances.Add(B);
  return true;
 }

 /** 沿统一截面扫掠实体；共线且截面相同的连续路段共享凸碰撞体。 */
 void Sweep(const TArray<FFrame>& Frames, const TArray<FVector2D>& Profile, bool bClosed,
  float TileSize, FVector2D Offset, FMesh& Mesh, TArray<TArray<FVector>>& Convex, bool bCollision,
  const FJunction* Junctions=nullptr, const TArray<FLxRoadAutoJunction>* Connections=nullptr)
 {
  const int32 N = Profile.Num();
  TArray<float> Perimeter;
  Perimeter.Add(0);
  for(int32 J=0; J<N; ++J) { Perimeter.Add(Perimeter.Last()+FVector2D::Distance(Profile[J],Profile[(J+1)%N])); }
  // 闭环强制完整纹理周期，防止首尾UV出现跳变；尺寸修正最多半个周期。
  const float Length = Frames.Last().Distance;
  const float LongScale = bClosed ? FMath::Max(1,FMath::RoundToInt(Length/TileSize))/Length : 1.0f/TileSize;
  /** 将截面坐标转换到道路局部空间。 */
  const auto Point = [](const FFrame& F, const FVector2D& P) { return F.Position + F.Right*(P.X+FMath::Sign(P.X)*F.WidthOffset) + FVector::UpVector*P.Y; };
  for(int32 I=0; I+1<Frames.Num(); ++I)
  {
   const FFrame& A=Frames[I]; const FFrame& B=Frames[I+1];
   for(int32 J=0; J<N; ++J)
   {
    if(Connections && (J==1 || J==3))
    {
     const int32 Side=J==3 ? 0 : 1;
     const float Mid=(A.Distance+B.Distance)*0.5f;
     if(Connections->ContainsByPredicate([&](const FLxRoadAutoJunction& C)
      { return C.Side==Side && Mid>C.Interval.X && Mid<C.Interval.Y; })) { continue; }
    }
    // 路基左右侧壁在开口处移除，避免内部竖面穿过支路。
    if(Junctions && ((J==3 && Junctions[0].Contains((A.Distance+B.Distance)*0.5f))
     || (J==1 && Junctions[1].Contains((A.Distance+B.Distance)*0.5f)))) { continue; }
    const int32 K=(J+1)%N;
    const FVector P0=Point(A,Profile[J]), P1=Point(B,Profile[J]), P2=Point(B,Profile[K]), P3=Point(A,Profile[K]);
    Mesh.Quad(P0,P1,P2,P3,
     FVector2D(Perimeter[J]/TileSize,A.Distance*LongScale)+Offset,
     FVector2D(Perimeter[J]/TileSize,B.Distance*LongScale)+Offset,
     FVector2D(Perimeter[J+1]/TileSize,B.Distance*LongScale)+Offset,
     FVector2D(Perimeter[J+1]/TileSize,A.Distance*LongScale)+Offset);
   }
  }
  if(bCollision)
  {
   for(int32 Start=0; Start+1<Frames.Num();)
   {
    int32 End=Start+1;
    while(End+1<Frames.Num())
    {
     const FFrame& A=Frames[Start]; const FFrame& B=Frames[End]; const FFrame& C=Frames[End+1];
     const FVector Direction=(C.Position-A.Position).GetSafeNormal();
     // 只合并误差小于0.01厘米的直线，不跨越坡折、转角或宽度斜接变化。
     if(!A.Right.Equals(B.Right,1.e-5) || !A.Right.Equals(C.Right,1.e-5)
      || !FMath::IsNearlyEqual(A.WidthOffset,B.WidthOffset,0.001f) || !FMath::IsNearlyEqual(A.WidthOffset,C.WidthOffset,0.001f)
      || FMath::PointDistToLine(B.Position,Direction,A.Position)>0.01
      || FVector::DotProduct(B.Position-A.Position,C.Position-B.Position)<=0) { break; }
     ++End;
    }
    TArray<FVector> Hull;
    Hull.Reserve(N*2);
    for(const FVector2D& P : Profile) { Hull.Add(Point(Frames[Start],P)); Hull.Add(Point(Frames[End],P)); }
    Convex.Add(MoveTemp(Hull));
    Start=End;
   }
  }
  if(!bClosed)
  {
   // 凸截面端盖采用三角扇，四边形接口的末点与第三点重合将产生冗余三角形，故直接添加三角形。
   for(int32 End=0; End<2; ++End)
   {
    const FFrame& F = End ? Frames.Last() : Frames[0];
    const FVector Nrm = (End ? Frames.Last().Position-Frames[Frames.Num()-2].Position : Frames[0].Position-Frames[1].Position).GetSafeNormal();
    for(int32 J=1; J+1<N; ++J)
    {
     const int32 Base=Mesh.Vertices.Num();
     const int32 Indices[3]={0,End ? J+1 : J,End ? J : J+1};
     for(int32 K : Indices)
     {
      Mesh.Vertices.Add(Point(F,Profile[K]));
      Mesh.UV.Add(Profile[K]/TileSize+Offset);
      Mesh.Normals.Add(Nrm);
      Mesh.Tangents.Emplace(F.Right.GetSafeNormal(),false);
     }
     Mesh.Triangles.Append({Base,Base+1,Base+2});
    }
   }
  }
 }

 /** 沿支路外围重新建立路缘石截面，使直角斜接和圆弧纹理连续。 */
 void SweepCurbPath(const TArray<FVector>& InputPoints, const TArray<FVector2D>& Profile,
  float Tile, FVector2D Offset, FMesh& Mesh, TArray<TArray<FVector>>& Convex, bool bCollision,
  float CornerRadius, const TMap<FVector,FVector>& CornerTangents, bool bLimitCurvature=false)
 {
  // 去除同一直线上的密集采样，避免直角斜接超出紧邻的短截面。
  TArray<FVector> Points;
  for(const FVector& Point : InputPoints)
  {
   if(!Points.IsEmpty() && Points.Last().Equals(Point,1.e-5)) { continue; }
   while(Points.Num()>1)
   {
    const FVector A=Points[Points.Num()-2], B=Points.Last();
    if(FMath::PointDistToLine(B,(Point-A).GetSafeNormal(),A)>1.e-5
     || FVector::DotProduct(B-A,Point-B)<=0) { break; }
    Points.Pop(EAllowShrinking::No);
   }
   Points.Add(Point);
  }
  if(Points.Num()<2) { return; }
  TArray<FFrame> Path;
  float Distance=0;
  for(int32 I=0; I<Points.Num(); ++I)
  {
   if(I>0) { Distance+=FVector::Distance(Points[I-1],Points[I]); }
   const FVector In=(Points[I]-Points[FMath::Max(0,I-1)]).GetSafeNormal2D();
   const FVector Out=(Points[FMath::Min(I+1,Points.Num()-1)]-Points[I]).GetSafeNormal2D();
   FVector Direction=I==0 ? Out : (I==Points.Num()-1 ? In : (In+Out).GetSafeNormal());
   float Denom=1;
   if(I>0 && I+1<Points.Num())
   {
    if(FVector::DotProduct(In,Out)<0.5)
    { Denom=FMath::Max(0.25f,static_cast<float>(FVector::DotProduct(Direction,In))); }
    else
    {
     // 按两侧弦长求圆弧切向，消除不等间距采样造成的斜接跳变。
     const double Before=FVector::Dist2D(Points[I],Points[I-1]), After=FVector::Dist2D(Points[I+1],Points[I]);
     Direction=(In*After+Out*Before).GetSafeNormal();
    }
   }
   for(const TPair<FVector,FVector>& Corner : CornerTangents)
   { if(Points[I].Equals(Corner.Key,1.e-5)) { Direction=Corner.Value; Denom=1; break; } }
   const FVector Right(-Direction.Y,Direction.X,0);
   float WidthScale=1;
   if(I>0 && I+1<Points.Num())
   {
    // 根据实际圆角半径统一限制极小圆角的厚度；不能用相邻采样间距估算，
    // 否则主路与圆弧的非均匀采样会让正常路缘石出现锯齿状收窄。
    const float Turn=FVector::CrossProduct(In,Out).Z;
    const float Cos=FVector::DotProduct(In,Out);
    const float InnerWidth=Turn<0 ? FMath::Max(0.0,-Profile[0].X) : FMath::Max(0.0,Profile[1].X);
    if(CornerRadius>0 && Cos>0 && FMath::Abs(Turn)>1.e-5f && InnerWidth>0)
    {
     WidthScale=FMath::Min(1.0f,CornerRadius*0.5f/InnerWidth);
    }
    if(bLimitCurvature && FMath::Abs(Turn)>1.e-6f && InnerWidth>0)
    {
     // 大小路短距离变宽时，内弯厚度不能超过曲率半径。用三点外接圆消除
     // 非均匀采样影响，连续收窄内弯而不产生反折；直线及两端接缝保持原宽。
     const double Radius=FVector::Dist2D(Points[I-1],Points[I+1])/(2*FMath::Abs(Turn));
     const double Ratio=InnerWidth/Radius;
     // 曲率接近零时使用二阶衰减，外缘也与原道路相切，避免接缝突然收窄。
     WidthScale=FMath::Min(WidthScale,static_cast<float>(1/FMath::Sqrt(1+Ratio*Ratio)));
    }
   }
   Path.Add({Points[I],Right*(WidthScale/Denom),Distance});
  }
  Sweep(Path,Profile,false,Tile,Offset,Mesh,Convex,bCollision);
 }

 /** 从主路边缘向外铺设无重叠支路，保持坡高并旋转路面纹理坐标。 */
 bool BuildJunction(const TArray<FFrame>& Frames, const FJunction& Junction, float Sign,
  float W, float Depth, float Tile, FVector2D Offset, FMesh& Mesh,
  TArray<TArray<FVector>>& Convex, bool bCollision, TArray<FVector>& Before, TArray<FVector>& After)
 {
  const FVector Down(0,0,-Depth);
  /** 提交四边形并按左右侧修正绕序；相切端点不生成退化三角形。 */
  const auto Face=[&](FVector A,FVector B,FVector C,FVector D,FVector2D UA,FVector2D UB,FVector2D UC,FVector2D UD)
  {
   if(Sign>0) { Swap(B,D); Swap(UB,UD); }
   Mesh.Triangle(A,B,C,UA,UB,UC); Mesh.Triangle(A,C,D,UA,UC,UD);
  };
  for(int32 I=0; I+1<Frames.Num(); ++I)
  {
   const FFrame& A=Frames[I]; const FFrame& B=Frames[I+1];
   const float Mid=(A.Distance+B.Distance)*0.5f;
   const FVector P=A.Position+A.Right*(Sign*(W+A.WidthOffset)), Q=B.Position+B.Right*(Sign*(W+B.WidthOffset));
   if(!Junction.Contains(Mid))
   {
    TArray<FVector>& Path=Mid<Junction.Center ? Before : After;
    Path.AddUnique(P); Path.AddUnique(Q);
    continue;
   }
   const bool bStem=FMath::Abs(Mid-Junction.Center)<Junction.HalfWidth;
   const float EA=bStem ? Junction.Length : Junction.ArcExtent(A.Distance);
   const float EB=bStem ? Junction.Length : Junction.ArcExtent(B.Distance);
   const FVector R=bStem && Junction.bStraightOutlet ? Junction.Outlet+Junction.OutletAxis*(B.Distance-Junction.Center) : Q+B.Right*(Sign*EB);
   const FVector S=bStem && Junction.bStraightOutlet ? Junction.Outlet+Junction.OutletAxis*(A.Distance-Junction.Center) : P+A.Right*(Sign*EA);
   // 长支路落在急弯内侧时可能反折；拒绝该配置，避免生成交叉路面。
   if(FVector::DotProduct((R-S).GetSafeNormal2D(),(Q-P).GetSafeNormal2D())<=0)
   { return false; }
   const FVector2D UP((A.Distance-Junction.Center)/Tile,0), UQ((B.Distance-Junction.Center)/Tile,0);
   const FVector2D UR(UQ.X,EB/Tile), US(UP.X,EA/Tile);
   if(bStem && Junction.bStraightOutlet && Junction.Radius>0)
   {
    // 曲线主路先沿原截面接到圆角末端，再过渡到平直出口，防止弧线末端出现三角缺口。
    const FVector IP=P+A.Right*(Sign*Junction.Radius), IQ=Q+B.Right*(Sign*Junction.Radius);
    const FVector2D UIP(UP.X,Junction.Radius/Tile), UIQ(UQ.X,Junction.Radius/Tile);
    Face(P,Q,IQ,IP,UP+Offset,UQ+Offset,UIQ+Offset,UIP+Offset);
    Face(IP,IQ,R,S,UIP+Offset,UIQ+Offset,UR+Offset,US+Offset);
    Face(IP+Down,IQ+Down,Q+Down,P+Down,UIP+Offset,UIQ+Offset,UQ+Offset,UP+Offset);
    Face(S+Down,R+Down,IQ+Down,IP+Down,US+Offset,UR+Offset,UIQ+Offset,UIP+Offset);
    if(bCollision)
    {
     Convex.Add({P,Q,IQ,IP,P+Down,Q+Down,IQ+Down,IP+Down});
     Convex.Add({IP,IQ,R,S,IP+Down,IQ+Down,R+Down,S+Down});
    }
   }
   else
   {
    Face(P,Q,R,S,UP+Offset,UQ+Offset,UR+Offset,US+Offset);
    Face(S+Down,R+Down,Q+Down,P+Down,US+Offset,UR+Offset,UQ+Offset,UP+Offset);
    if(bCollision) { Convex.Add({P,Q,R,S,P+Down,Q+Down,R+Down,S+Down}); }
   }
   Face(S,R,R+Down,S+Down,US+Offset,UR+Offset,UR+FVector2D(0,Depth/Tile)+Offset,US+FVector2D(0,Depth/Tile)+Offset);
   if(!bStem)
   {
    TArray<FVector>& Path=Mid<Junction.Center ? Before : After;
    Path.AddUnique(S); Path.AddUnique(R);
   }
  }
  // 支路两侧从圆弧末端接到支路末端；不在支路出口横铺路缘石。
  for(int32 End=0; End<2; ++End)
  {
   const float Distance=Junction.Center+(End==0 ? -Junction.HalfWidth : Junction.HalfWidth);
   const FFrame* Frame=Frames.FindByPredicate([&](const FFrame& F){ return FMath::IsNearlyEqual(F.Distance,Distance,0.0001f); });
   if(!Frame) { return false; }
   const FVector Inner=Frame->Position+Frame->Right*(Sign*(W+Junction.Radius));
   const FVector Outer=Junction.bStraightOutlet ? Junction.Outlet+Junction.OutletAxis*(Distance-Junction.Center) : Frame->Position+Frame->Right*(Sign*(W+Junction.Length));
   const FVector2D UI(0,Junction.Radius/Tile), UO(0,Junction.Length/Tile);
   if(End==0)
   {
    Before.AddUnique(Inner); Before.AddUnique(Outer);
    Face(Inner,Outer,Outer+Down,Inner+Down,UI,UO,UO+FVector2D(Depth/Tile,0),UI+FVector2D(Depth/Tile,0));
   }
   else
   {
    if(After.IsEmpty() || !After[0].Equals(Inner)) { After.Insert(Inner,0); }
    if(!Outer.Equals(Inner)) { After.Insert(Outer,0); }
    Face(Outer,Inner,Inner+Down,Outer+Down,UO,UI,UI+FVector2D(Depth/Tile,0),UO+FVector2D(Depth/Tile,0));
   }
  }
  return true;
 }
}

namespace LxRoad
{
 /** 将独立连接轮廓填充为实体；耳切三角化支持斜接产生的凹多边形。 */
 bool BuildConnection(const FLxRoadAutoJunction& J,const TArray<FFrame>& Frames,const FTransform& Transform,
  float W,float Depth,float Tile,FVector2D Offset,FMesh& Mesh,TArray<TArray<FVector>>& Convex,bool bCollision)
 {
  TArray<FVector> Polygon;
  for(FVector P : J.Before) { Polygon.Add(Transform.InverseTransformPosition(P)); }
  for(FVector P : J.After) { Polygon.Add(Transform.InverseTransformPosition(P)); }
  if(J.Side>=0)
  {
   for(int32 I=Frames.Num()-1; I>=0; --I)
   {
    const FFrame& F=Frames[I];
    if(F.Distance>J.Interval.X+0.001 && F.Distance<J.Interval.Y-0.001)
    { Polygon.Add(F.Position+F.Right*((J.Side==0 ? -1 : 1)*W)); }
   }
  }
  /** 计算平面有向面积，避免空间坡高影响拓扑判断。 */
  const auto Cross=[](FVector A,FVector B,FVector C){ return (B.X-A.X)*(C.Y-A.Y)-(B.Y-A.Y)*(C.X-A.X); };
  // 圆角相切点与主路采样可能重复；先移除零长边和共线中点。
  for(int32 Pass=0; Pass<2; ++Pass)
  {
   for(int32 I=Polygon.Num()-1; I>=0 && Polygon.Num()>=3; --I)
   {
    const FVector A=Polygon[(I+Polygon.Num()-1)%Polygon.Num()], B=Polygon[I], C=Polygon[(I+1)%Polygon.Num()];
    if(B.Equals(A,0.001) || (FMath::Abs(Cross(A,B,C))<0.001 && FVector::DotProduct(B-A,C-B)>=0)) { Polygon.RemoveAt(I); }
   }
  }
  if(Polygon.Num()<3) { return true; } // 无间隙端点不需要重复铺面。
  double Area=0;
  for(int32 I=0; I<Polygon.Num(); ++I) { Area+=Polygon[I].X*Polygon[(I+1)%Polygon.Num()].Y-Polygon[(I+1)%Polygon.Num()].X*Polygon[I].Y; }
  if(FMath::Abs(Area)<0.001) { return true; }
  const bool bReverseSides=Area<0;
  if(bReverseSides) { Algo::Reverse(Polygon); }
  TArray<int32> Remaining;
  for(int32 I=0; I<Polygon.Num(); ++I) { Remaining.Add(I); }
  TArray<FIntVector> Triangles;
  while(Remaining.Num()>2)
  {
   bool bFound=false;
   for(int32 I=0; I<Remaining.Num(); ++I)
   {
    const int32 A=Remaining[(I+Remaining.Num()-1)%Remaining.Num()], B=Remaining[I], C=Remaining[(I+1)%Remaining.Num()];
    if(Cross(Polygon[A],Polygon[B],Polygon[C])<0.00001) { continue; }
    bool bInside=false;
    for(int32 P : Remaining)
    {
     if(P==A || P==B || P==C) { continue; }
     if(Cross(Polygon[A],Polygon[B],Polygon[P])>=-0.00001 && Cross(Polygon[B],Polygon[C],Polygon[P])>=-0.00001
      && Cross(Polygon[C],Polygon[A],Polygon[P])>=-0.00001) { bInside=true; break; }
    }
    if(bInside) { continue; }
    Triangles.Add(FIntVector(A,B,C)); Remaining.RemoveAt(I); bFound=true; break;
   }
   if(!bFound) { return false; }
  }
  const FVector Direction=Transform.InverseTransformVectorNoScale(J.Outward).GetSafeNormal2D();
  const FVector Across(-Direction.Y,Direction.X,0), Down(0,0,-Depth);
  /** 连接片纹理沿接入道路方向铺设。 */
  const auto UV=[&](FVector P){ return FVector2D(FVector::DotProduct(P,Across),FVector::DotProduct(P,Direction))/Tile+Offset; };
  for(FIntVector Tri : Triangles)
  {
   const FVector A=Polygon[Tri.X],B=Polygon[Tri.Y],C=Polygon[Tri.Z];
   Mesh.Triangle(A,C,B,UV(A),UV(C),UV(B));
   Mesh.Triangle(A+Down,B+Down,C+Down,UV(A),UV(B),UV(C));
   if(bCollision) { Convex.Add({A,B,C,A+Down,B+Down,C+Down}); }
  }
  for(const TArray<FVector>* Boundary : {&J.Before,&J.After})
  {
   for(int32 I=0; I+1<Boundary->Num(); ++I)
   {
    FVector A=Transform.InverseTransformPosition((*Boundary)[I]), B=Transform.InverseTransformPosition((*Boundary)[I+1]);
    if(A.Equals(B,0.001)) { continue; }
    if(bReverseSides) { Swap(A,B); }
    Mesh.Quad(A,B,B+Down,A+Down,UV(A),UV(B),UV(B)+FVector2D(0,Depth/Tile),UV(A)+FVector2D(0,Depth/Tile));
   }
  }
  return true;
 }
}

ALxSplineRoadActor::ALxSplineRoadActor()
{
 PrimaryActorTick.bCanEverTick=false;
 RoadSpline=CreateDefaultSubobject<USplineComponent>(TEXT("RoadSpline"));
 SetRootComponent(RoadSpline);
 RoadSpline->ClearSplinePoints(false);
 RoadSpline->AddSplinePoint(FVector::ZeroVector,ESplineCoordinateSpace::Local,false);
 RoadSpline->AddSplinePoint(FVector(2000,0,0),ESplineCoordinateSpace::Local,false);
 RoadSpline->UpdateSpline();
 RoadMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("RoadMesh"));
 RoadMesh->SetupAttachment(RoadSpline);
 RoadMesh->SetMobility(EComponentMobility::Movable);
 RoadMesh->bEditableWhenInherited=false;
 RoadMesh->bUseAsyncCooking=false;
 RoadMesh->bUseComplexAsSimpleCollision=false;
 RoadMesh->CanCharacterStepUpOn=ECB_Yes;
 RoadMesh->SetCollisionProfileName(TEXT("BlockAll"));
 OptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("OptimizedMesh"));
 OptimizedMesh->SetupAttachment(RoadMesh);
 OptimizedMesh->bEditableWhenInherited=false;
}

void ALxSplineRoadActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if(IsTemplate()) { return; }
 RestoreRoadComponentTransforms();
 for(USceneComponent* Component : {static_cast<USceneComponent*>(RoadMesh.Get()),static_cast<USceneComponent*>(OptimizedMesh.Get())})
 {
  if(!Component) { continue; }
  Component->TransformUpdated.RemoveAll(this);
  Component->TransformUpdated.AddWeakLambda(this,[this](USceneComponent*,EUpdateTransformFlags,ETeleportType)
  { RestoreRoadComponentTransforms(); });
 }
}

void ALxSplineRoadActor::RestoreRoadComponentTransforms()
{
 if(bRestoringRoadComponents || !RoadSpline) { return; }
 TGuardValue<bool> Guard(bRestoringRoadComponents,true);
 for(USceneComponent* Component : {static_cast<USceneComponent*>(RoadMesh.Get()),static_cast<USceneComponent*>(OptimizedMesh.Get())})
 {
  if(!Component) { continue; }
  USceneComponent* Parent=Component==RoadMesh ? static_cast<USceneComponent*>(RoadSpline.Get()) : static_cast<USceneComponent*>(RoadMesh.Get());
  if(!Parent) { continue; }
  if(Component->GetAttachParent()!=Parent) { Component->AttachToComponent(Parent,FAttachmentTransformRules::KeepRelativeTransform); }
  if(Component->IsUsingAbsoluteLocation() || Component->IsUsingAbsoluteRotation() || Component->IsUsingAbsoluteScale())
  { Component->SetAbsolute(false,false,false); }
  if(!Component->GetRelativeTransform().Equals(FTransform::Identity,1.e-6))
  { Component->SetRelativeTransform(FTransform::Identity,false,nullptr,ETeleportType::TeleportPhysics); }
 }
}

void ALxSplineRoadActor::OnConstruction(const FTransform& Transform)
{
 CancelDeferredRoadUpdate();
 Super::OnConstruction(Transform);
 if (!OptimizedMesh || !OptimizedMesh->IsSourceCurrent()
  || (!RoadGeometryInputs.IsEmpty() && RoadGeometryInputs!=CaptureRoadGeometryInputs())) { RebuildRoad(); }
}

void ALxSplineRoadActor::CancelDeferredRoadUpdate()
{
 if (DeferredRoadUpdateHandle.IsValid())
 { FTSTicker::RemoveTicker(DeferredRoadUpdateHandle); DeferredRoadUpdateHandle.Reset(); }
}

void ALxSplineRoadActor::BeginDestroy()
{
 CancelDeferredRoadUpdate();
 Super::BeginDestroy();
}

void ALxSplineRoadActor::ScheduleDeferredRoadUpdate()
{
#if WITH_EDITOR
 LxGeneratedMeshBuild::RequestPreview(this);
#endif
}

#if WITH_EDITOR
void ALxSplineRoadActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 const bool bRoadValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerClass() && PropertyAboutToChange->GetOwnerClass()->IsChildOf(StaticClass())
   && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bRoadValue) { UObject::PreEditChange(PropertyAboutToChange); return; }
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxSplineRoadActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 if(Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxSplineRoadActor,RoadConnections))
 {
  const int32 Index=Event.GetArrayIndex(TEXT("RoadConnections"));
  if(RoadConnections.IsValidIndex(Index)) { SynchronizeConnectionSettings(Index); }
  else
  {
   // 多选编辑和嵌套结构属性事件可能不携带数组下标，此时同步本路所有连接。
   for(int32 I=0; I<RoadConnections.Num(); ++I) { SynchronizeConnectionSettings(I); }
  }
 }
 if(RoadSpline)
 {
  const FName Name=Event.GetMemberPropertyName();
  const bool bLeft=Name==GET_MEMBER_NAME_CHECKED(ALxSplineRoadActor,LeftJunctionHandle);
  const bool bRight=Name==GET_MEMBER_NAME_CHECKED(ALxSplineRoadActor,RightJunctionHandle);
  if(bLeft || bRight)
  {
   const FVector Handle=bLeft ? LeftJunctionHandle : RightJunctionHandle;
   const float Key=RoadSpline->FindInputKeyClosestToWorldLocation(RoadSpline->GetComponentTransform().TransformPosition(Handle));
   (bLeft ? LeftJunctionDistance : RightJunctionDistance)=RoadSpline->GetDistanceAlongSplineAtSplineInputKey(Key);
  }
 }
 if (!IsTemplate() && (Event.ChangeType&EPropertyChangeType::Interactive)!=0)
 {
  ScheduleDeferredRoadUpdate();
  UObject::PostEditChangeProperty(Event);
  return;
 }
 CancelDeferredRoadUpdate();
 Super::PostEditChangeProperty(Event);
 OnConstruction(GetActorTransform());
}

void ALxSplineRoadActor::PostEditMove(bool bFinished)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 if (!bFinished)
 {
  // 与其他自动化类型一致，拖动时定时更新简化预览，避免蓝图构造与同步碰撞反复执行。
  if (GEngine) { GEngine->BroadcastOnActorMoving(this); }
  if (RootComponent) { RootComponent->PostEditComponentMove(false); }
  return;
 }
 CancelDeferredRoadUpdate();
 Super::PostEditMove(true);
 OnConstruction(GetActorTransform());
}

void ALxSplineRoadActor::PostEditUndo()
{
 CancelDeferredRoadUpdate();
 RoadGeometryInputs.Reset();
 Super::PostEditUndo();
 RebuildRoad();
}
#endif

void ALxSplineRoadActor::SynchronizeConnectionSettings(int32 Index)
{
 if(!RoadConnections.IsValidIndex(Index)) { return; }
 const FLxRoadConnectionSettings& S=RoadConnections[Index];
 if(!IsValid(S.OtherRoad)) { return; }
 for(FLxRoadConnectionSettings& Other : S.OtherRoad->RoadConnections)
 {
  if(Other.OtherRoad==this && Other.LocalPort==S.OtherPort && Other.OtherPort==S.LocalPort)
  {
#if WITH_EDITOR
   S.OtherRoad->Modify();
#endif
   Other.Width=S.Width; Other.Radius=S.Radius; Other.Length=S.Length;
  }
 }
}

void ALxSplineRoadActor::SetRoadConnectionParameters(int32 ConnectionIndex,float Width,float Radius,float Length)
{
 if(!RoadConnections.IsValidIndex(ConnectionIndex) || !FMath::IsFinite(Width)
  || !FMath::IsFinite(Radius) || !FMath::IsFinite(Length)) { return; }
#if WITH_EDITOR
 Modify();
#endif
 FLxRoadConnectionSettings& S=RoadConnections[ConnectionIndex];
 S.Width=FMath::Max(0.f,Width); S.Radius=FMath::Max(0.f,Radius); S.Length=FMath::Max(0.f,Length);
 SynchronizeConnectionSettings(ConnectionIndex);
 RebuildRoad();
}

void ALxSplineRoadActor::Destroyed()
{
 FLxRoadConnectionSolver::Refresh(*this,true);
 Super::Destroyed();
}

void ALxSplineRoadActor::ClearLegacyJunctions()
{
#if WITH_EDITOR
 Modify();
#endif
 bLeftJunction=false; bRightJunction=false;
 RebuildRoad();
}

void ALxSplineRoadActor::ClearRoad()
{
 RoadGeometryInputs.Reset();
 if(IsValid(RoadMesh)) { RoadMesh->ClearAllMeshSections(); RoadMesh->ClearCollisionConvexMeshes(); }
 GenerationDiagnostic.Reset();
 CollisionDataVersion=0;
 if (OptimizedMesh) { OptimizedMesh->MarkSourceDirty(); OptimizedMesh->SetVisibility(false); }
}

void ALxSplineRoadActor::BeginPlay()
{
 Super::BeginPlay();
 if(CollisionDataVersion<1) { RebuildRoad(); }
}

void ALxSplineRoadActor::RebuildRoad()
{
 CancelDeferredRoadUpdate();
 if(GetWorld() && !IsTemplate()) { FLxRoadConnectionSolver::Refresh(*this); }
 else { BuildRoadGeometry(); }
}

TArray<uint8> ALxSplineRoadActor::CaptureRoadGeometryInputs() const
{
 TArray<uint8> Inputs=LxGeneratedMeshBuild::CaptureInputs(this,false);
 FMemoryWriter Writer(Inputs);
 Writer.Seek(Inputs.Num());
 FObjectAndNameAsStringProxyArchive Archive(Writer,false);
 // 旧版支路虽已隐藏编辑入口，旧关卡与兼容接口仍可修改，必须参与形态判断。
 for (bool Enabled : {bLeftJunction,bRightJunction}) { Archive << Enabled; }
 for (float Value : {LeftJunctionDistance,LeftJunctionLength,LeftJunctionRadius,
  RightJunctionDistance,RightJunctionLength,RightJunctionRadius}) { Archive << Value; }
 int32 CollisionVersion=CollisionDataVersion;
 Archive << CollisionVersion;
 // 连接轮廓采用世界坐标；有连接时还必须比较演员变换，以保持局部接缝正确。
 if (ConnectedEndpoints[0].bConnected || ConnectedEndpoints[1].bConnected || !ConnectedJunctions.IsEmpty())
 {
  FTransform Transform=GetActorTransform();
  Archive << Transform;
 }
 for (const FLxRoadEndpoint& Endpoint : ConnectedEndpoints)
 { FLxRoadEndpoint::StaticStruct()->SerializeItem(Archive,const_cast<FLxRoadEndpoint*>(&Endpoint),nullptr); }
 int32 Count=ConnectedJunctions.Num();
 Archive << Count;
 for (const FLxRoadAutoJunction& Junction : ConnectedJunctions)
 { FLxRoadAutoJunction::StaticStruct()->SerializeItem(Archive,const_cast<FLxRoadAutoJunction*>(&Junction),nullptr); }
 return Inputs;
}

void ALxSplineRoadActor::AcceptUnchangedRoadSource()
{
 if (OptimizedMesh) { OptimizedMesh->AcceptUnchangedSource(); }
}

void ALxSplineRoadActor::BuildRoadGeometry()
{
 const bool bBuildCollision=bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this);
 RestoreRoadComponentTransforms();
 if (!RoadGeometryInputs.IsEmpty() && RoadGeometryInputs==CaptureRoadGeometryInputs())
 {
  AcceptUnchangedRoadSource();
  return;
 }
 TArray<LxRoad::FFrame> Frames;
 // 主路顶点之后是连接片及端部圆角，供派生堤身补充支撑。
 int32 MainVertexCount=MAX_int32;
 // 包括无效参数和空几何在内的所有出口都同步渲染缓存。
 ON_SCOPE_EXIT
 {
  BuildRoadFoundation(Frames,MainVertexCount);
  if (OptimizedMesh) { OptimizedMesh->Synchronize(RoadMesh); }
  RoadGeometryInputs=CaptureRoadGeometryInputs();
 };
 ClearRoad();
 if(!IsValid(RoadSpline) || !IsValid(RoadMesh) || RoadSpline->GetNumberOfSplinePoints()<2) { return; }
 // 连接片单独生成，主路与支路始终使用原始样条。
 USplineComponent* Spline=RoadSpline;
 for(float V : {RoadWidth,RoadDepth,SampleSpacing,CurbWidth,CurbHeight,CurbBevel,RoadTextureSize,CurbTextureSize,StartCurbRadius,EndCurbRadius})
 {
  if(!FMath::IsFinite(V)) { GenerationDiagnostic=TEXT("尺寸参数含有无效数值。"); return; }
 }
 const float Length=Spline->GetSplineLength();
 if(!FMath::IsFinite(Length) || Length<1) { GenerationDiagnostic=TEXT("道路中心线过短。"); return; }
 const bool bClosed=Spline->IsClosedLoop();
 const float W=FMath::Max(50.0f,RoadWidth)*0.5f, D=FMath::Max(1.0f,RoadDepth);
 LxRoad::FJunction Junctions[2];
 for(int32 Side=0; Side<2; ++Side)
 {
  LxRoad::FJunction& J=Junctions[Side];
  J.bEnabled=Side==0 ? bLeftJunction : bRightJunction;
  if(!J.bEnabled) { continue; }
  float& Position=Side==0 ? LeftJunctionDistance : RightJunctionDistance;
  const float Radius=Side==0 ? LeftJunctionRadius : RightJunctionRadius;
  const float BranchLength=Side==0 ? LeftJunctionLength : RightJunctionLength;
  const float Margin=(Side==0 ? bLeftCurb : bRightCurb) ? FMath::Max(5.0f,CurbWidth)*2+1 : 1;
  if(!FMath::IsFinite(Position) || !FMath::IsFinite(Radius) || !FMath::IsFinite(BranchLength))
  { GenerationDiagnostic=TEXT("路口参数含有无效数值。"); return; }
  // 闭环同样将路口保留在首尾接缝以内，避免跨接缝的特殊拓扑。
  if(Length<2*(W+Margin)) { GenerationDiagnostic=TEXT("道路长度不足以容纳路口，请延长中心线或减小路宽。"); return; }
  J.HalfWidth=W;
  J.Length=FMath::Max(50.0f,BranchLength);
  J.Radius=FMath::Clamp(Radius,0.0f,FMath::Min(J.Length,(Length-2*(W+Margin))*0.5f));
  if(J.Radius<0.01f) { J.Radius=0; }
  J.Center=FMath::Clamp(Position,W+J.Radius+Margin,Length-W-J.Radius-Margin);
  Position=J.Center;
  const FVector P=Spline->GetLocationAtDistanceAlongSpline(J.Center,ESplineCoordinateSpace::Local);
  const FVector T=Spline->GetDirectionAtDistanceAlongSpline(J.Center,ESplineCoordinateSpace::Local).GetSafeNormal2D();
  (Side==0 ? LeftJunctionHandle : RightJunctionHandle)=P+FVector(-T.Y,T.X,0)*(Side==0 ? -W : W);
 }
 const int32 Count=Spline->GetNumberOfSplinePoints();
 const int32 Sections=bClosed ? Count : Count-1;
 TArray<float> Distances; Distances.Add(0);
 for(int32 I=0; I<Sections; ++I)
 {
  const float A=Spline->GetDistanceAlongSplineAtSplinePoint(I);
  const float B=I+1<Count ? Spline->GetDistanceAlongSplineAtSplinePoint(I+1) : Length;
  if(B-A<0.01f) { GenerationDiagnostic=TEXT("存在重复控制点，请移除重合点。"); return; }
  if(!LxRoad::Sample(Spline,A,B,FMath::Max(10.0f,SampleSpacing),0,Distances))
  { GenerationDiagnostic=TEXT("超过4096个采样截面，请拆分道路或增大采样间距。"); return; }
 }
 // 开口端点、支路侧边和圆弧采样必须与主路共用截面，防止接缝裂缝。
 for(const LxRoad::FJunction& J : Junctions)
 {
  if(!J.bEnabled) { continue; }
  for(float Sign : {-1.0f,1.0f})
  {
   Distances.Add(J.Center+Sign*J.HalfWidth);
   if(J.Radius>0)
   {
    const int32 Steps=FMath::Clamp(FMath::CeilToInt(HALF_PI*J.Radius/FMath::Max(10.0f,SampleSpacing)),8,64);
    for(int32 I=0; I<Steps; ++I)
    { Distances.Add(J.Center+Sign*(J.HalfWidth+J.Radius*(1-FMath::Sin(HALF_PI*I/Steps)))); }
   }
  }
 }
 for(const FLxRoadAutoJunction& J : ConnectedJunctions)
 { if(J.Side>=0) { Distances.Add(J.Interval.X); Distances.Add(J.Interval.Y); } }
 // 只裁切派生网格，用户控制点、切线和原始道路长度保持不变。
 const float VisibleStart=bClosed?0:ConnectedEndpoints[0].TerraceTrimMeters*100;
 const float VisibleEnd=Length-(bClosed?0:ConnectedEndpoints[1].TerraceTrimMeters*100);
 if(VisibleEnd-VisibleStart<1) { GenerationDiagnostic=TEXT("台地连接裁切后道路过短。"); return; }
 Distances.RemoveAll([&](float Distance){return Distance<VisibleStart || Distance>VisibleEnd;});
 Distances.Add(VisibleStart);Distances.Add(VisibleEnd);
 Distances.Sort();
 for(int32 I=Distances.Num()-1; I>0; --I)
 { if(FMath::IsNearlyEqual(Distances[I],Distances[I-1],0.0001f)) { Distances.RemoveAt(I); } }
 if(Distances.Num()>4096) { GenerationDiagnostic=TEXT("包含路口后超过4096个采样截面，请拆分道路。"); return; }
 for(float Distance : Distances)
 {
  Frames.Add({Spline->GetLocationAtDistanceAlongSpline(Distance,ESplineCoordinateSpace::Local),FVector::ZeroVector,Distance});
 }
 const int32 Last=Frames.Num()-1;
 for(int32 I=0; I<=Last; ++I)
 {
  const FVector P=Frames[I].Position;
  const FVector Before=I>0 ? P-Frames[I-1].Position : (bClosed ? P-Frames[Last-1].Position : Frames[1].Position-P);
  const FVector After=I<Last ? Frames[I+1].Position-P : (bClosed ? Frames[1].Position-P : P-Frames[I-1].Position);
  if(P.ContainsNaN() || Before.SizeSquared2D()<1.e-10 || After.SizeSquared2D()<1.e-10)
  { GenerationDiagnostic=TEXT("道路包含垂直或重合路段，请使用具有水平长度的坡道。"); return; }
  FVector In=Before.GetSafeNormal2D(), Out=After.GetSafeNormal2D();
  {
   // 路口增加了不等间距截面，使用样条真实切向消除弦方向在密集采样处的抖动。
   // 两侧分别取值，以保留线性控制点的斜接和闭环接缝。
   const float Epsilon=0.01f;
   const float Previous=Frames[I].Distance-Epsilon, Next=Frames[I].Distance+Epsilon;
   In=Spline->GetDirectionAtDistanceAlongSpline(bClosed ? FMath::Fmod(Previous+Length,Length) : FMath::Max(0.0f,Previous),ESplineCoordinateSpace::Local).GetSafeNormal2D();
   Out=Spline->GetDirectionAtDistanceAlongSpline(bClosed ? FMath::Fmod(Next,Length) : FMath::Min(Length,Next),ESplineCoordinateSpace::Local).GetSafeNormal2D();
  }
  const FVector R0(-In.Y,In.X,0), R1(-Out.Y,Out.X,0);
  const FVector Bisector=(R0+R1).GetSafeNormal();
  const double Denom=FVector::DotProduct(Bisector,R0);
  if(Denom<0.5) { GenerationDiagnostic=TEXT("转角过急，请增加圆滑过渡控制点。"); return; }
  Frames[I].Right=Bisector/Denom;

 }
 const float CW=FMath::Max(5.0f,CurbWidth), CH=FMath::Max(1.0f,CurbHeight);
 const float Bevel=FMath::Clamp(CurbBevel,0.0f,FMath::Min(CW*0.49f,CH*0.99f));
 for(int32 I=0; I<Last; ++I)
 {
  const FVector Travel=Frames[I+1].Position-Frames[I].Position;
  for(float Side : {-W-(bLeftCurb ? CW : 0),W+(bRightCurb ? CW : 0)})
  {
   const FVector Edge=Travel+Frames[I+1].Right*(Side+FMath::Sign(Side)*Frames[I+1].WidthOffset)
    -Frames[I].Right*(Side+FMath::Sign(Side)*Frames[I].WidthOffset);
   if(FVector::DotProduct(FVector(Travel.X,Travel.Y,0),FVector(Edge.X,Edge.Y,0))<=1.e-10)
   { GenerationDiagnostic=TEXT("弯道内侧发生折叠，请增大转弯半径或减小道路宽度。"); return; }
  }
 }
 TArray<TArray<FVector>> Convex;
 LxRoad::FMesh Road;
 LxRoad::Sweep(Frames,{{-W,-D},{W,-D},{W,0},{-W,0}},bClosed,FMath::Max(1.0f,RoadTextureSize),TextureOffset,Road,Convex,bBuildCollision,Junctions,&ConnectedJunctions);
 MainVertexCount=Road.Vertices.Num();
 for(const FLxRoadAutoJunction& J : ConnectedJunctions)
 {
  if(!LxRoad::BuildConnection(J,Frames,Spline->GetComponentTransform(),W,D,FMath::Max(1.f,RoadTextureSize),TextureOffset,Road,Convex,bBuildCollision))
  { GenerationDiagnostic=TEXT("连接轮廓发生交叠，请增大道路间距或减小路口圆角、宽度。"); return; }
 }
 LxRoad::FMesh Curbs[2];
 for(int32 Side=0; Side<2; ++Side)
 {
  const bool bCurb=Side==0 ? bLeftCurb : bRightCurb;
  TArray<FVector> Before,After;
  if(Junctions[Side].bEnabled && !LxRoad::BuildJunction(Frames,Junctions[Side],Side==0 ? -1.0f : 1.0f,
   W,D,FMath::Max(1.0f,RoadTextureSize),TextureOffset,Road,Convex,bBuildCollision,Before,After))
  { GenerationDiagnostic=TEXT("支路在弯道内侧发生折叠，请移动路口、缩短支路或增大主路转弯半径。"); return; }
  if(!bCurb) { continue; }
  const float L=Side==0 ? -W-CW : W, R=L+CW;
  TArray<FVector2D> Profile;
  if(Bevel>0.01f) { Profile={{L,-D},{R,-D},{R,CH-Bevel},{R-Bevel,CH},{L+Bevel,CH},{L,CH-Bevel}}; }
  else { Profile={{L,-D},{R,-D},{R,CH},{L,CH}}; }
  LxRoad::FMesh& Curb=Curbs[Side];
  const float Tile=FMath::Max(1.0f,CurbTextureSize);
  const FVector2D Offset=TextureOffset+FVector2D(Side*0.37,0.23);
  TArray<const FLxRoadAutoJunction*> AutoSide;
  for(const FLxRoadAutoJunction& J : ConnectedJunctions) { if(J.Side==Side) { AutoSide.Add(&J); } }
  AutoSide.Sort([](const FLxRoadAutoJunction& A,const FLxRoadAutoJunction& B){ return A.Interval.X<B.Interval.X; });
  if(!AutoSide.IsEmpty())
  {
   for(FVector2D& P : Profile) { P.X-=Side==0 ? -W : W; }
   TArray<FVector> Path;
   TMap<FVector,FVector> Tangents;
   for(const FLxRoadAutoJunction* J : AutoSide)
   {
    Tangents.Add(Spline->GetComponentTransform().InverseTransformPosition(J->Before.Last()),Spline->GetComponentTransform().InverseTransformVectorNoScale(J->Outward));
    Tangents.Add(Spline->GetComponentTransform().InverseTransformPosition(J->After[0]),-Spline->GetComponentTransform().InverseTransformVectorNoScale(J->Outward));
    Tangents.Add(Spline->GetComponentTransform().InverseTransformPosition(J->Before[0]),Spline->GetDirectionAtDistanceAlongSpline(J->Interval.X,ESplineCoordinateSpace::Local).GetSafeNormal2D());
    Tangents.Add(Spline->GetComponentTransform().InverseTransformPosition(J->After.Last()),Spline->GetDirectionAtDistanceAlongSpline(J->Interval.Y,ESplineCoordinateSpace::Local).GetSafeNormal2D());
   }
   float Cursor=0;
   for(const FLxRoadAutoJunction* J : AutoSide)
   {
    for(const LxRoad::FFrame& F : Frames)
    { if(F.Distance>=Cursor-0.001 && F.Distance<=J->Interval.X+0.001) { Path.AddUnique(F.Position+F.Right*(Side==0 ? -W : W)); } }
    for(FVector P : J->Before) { Path.AddUnique(Spline->GetComponentTransform().InverseTransformPosition(P)); }
    LxRoad::SweepCurbPath(Path,Profile,Tile,Offset,Curb,Convex,bBuildCollision,0,Tangents);
    Path.Reset();
    for(FVector P : J->After) { Path.AddUnique(Spline->GetComponentTransform().InverseTransformPosition(P)); }
    Cursor=J->Interval.Y;
   }
   for(const LxRoad::FFrame& F : Frames)
   { if(F.Distance>=Cursor-0.001) { Path.AddUnique(F.Position+F.Right*(Side==0 ? -W : W)); } }
   LxRoad::SweepCurbPath(Path,Profile,Tile,Offset,Curb,Convex,bBuildCollision,0,Tangents);
  }
  else if(Junctions[Side].bEnabled)
  {
   TMap<FVector,FVector> CornerTangents;
   const LxRoad::FJunction& J=Junctions[Side];
   const float Sign=Side==0 ? -1.0f : 1.0f;
   if(J.Radius>0)
   {
    // 圆弧与主路、支路相切处使用精确切向，不受相邻直线长度影响。
    for(float End : {-1.0f,1.0f})
    {
     for(bool bInner : {false,true})
     {
      const float Distance=J.Center+End*(J.HalfWidth+(bInner ? 0 : J.Radius));
      const LxRoad::FFrame* Frame=Frames.FindByPredicate([&](const LxRoad::FFrame& F){ return FMath::IsNearlyEqual(F.Distance,Distance,0.0001f); });
      if(!Frame) { continue; }
      const FVector Point=Frame->Position+Frame->Right*(Sign*(W+(bInner ? J.Radius : 0)));
      const FVector Tangent=bInner ? Frame->Right*(-End*Sign) : FVector(Frame->Right.Y,-Frame->Right.X,0);
      CornerTangents.Add(Point,Tangent.GetSafeNormal2D());
     }
    }
   }
   for(FVector2D& P : Profile) { P.X-=Side==0 ? -W : W; }
   if(bClosed)
   {
    // 在主路闭环接缝处拼接路径，只有支路出口保留路缘石端盖。
    for(int32 I=1; I<Before.Num(); ++I) { After.Add(Before[I]); }
    LxRoad::SweepCurbPath(After,Profile,Tile,Offset,Curb,Convex,bBuildCollision,J.Radius,CornerTangents);
   }
   else
   {
    LxRoad::SweepCurbPath(Before,Profile,Tile,Offset,Curb,Convex,bBuildCollision,J.Radius,CornerTangents);
    LxRoad::SweepCurbPath(After,Profile,Tile,Offset,Curb,Convex,bBuildCollision,J.Radius,CornerTangents);
   }
  }
  else { LxRoad::Sweep(Frames,Profile,bClosed,Tile,Offset,Curb,Convex,bBuildCollision); }
 }
 for(const FLxRoadAutoJunction& J : ConnectedJunctions)
 {
  if(J.Side>=0) { continue; }
  for(int32 Side=0; Side<2; ++Side)
  {
   if(!(Side==0 ? bLeftCurb : bRightCurb)) { continue; }
   TArray<FVector> Path;
   for(FVector P : (Side==0 ? J.Before : J.After)) { Path.Add(Spline->GetComponentTransform().InverseTransformPosition(P)); }
   TMap<FVector,FVector> Tangents;
   if(!Path.IsEmpty())
   {
    Tangents.Add(Path[0],Spline->GetComponentTransform().InverseTransformVectorNoScale(Side==0 ? J.Outward : -J.EndDirection));
    Tangents.Add(Path.Last(),Spline->GetComponentTransform().InverseTransformVectorNoScale(Side==0 ? J.EndDirection : -J.Outward));
   }
   TArray<FVector2D> Profile;
   if(Bevel>0.01f) { Profile={{-CW,-D},{0,-D},{0,CH-Bevel},{-Bevel,CH},{-CW+Bevel,CH},{-CW,CH-Bevel}}; }
   else { Profile={{-CW,-D},{0,-D},{0,CH},{-CW,CH}}; }
   LxRoad::SweepCurbPath(Path,Profile,FMath::Max(1.f,CurbTextureSize),TextureOffset,Curbs[Side],Convex,bBuildCollision,0,Tangents,true);
  }
 }
 LxRoad::FMesh EndCurbs;
 for(int32 End=0; End<2; ++End)
 {
  if(bClosed || !(End ? bEndCurb : bStartCurb) || ConnectedEndpoints[End].bConnected) { continue; }
  const float Radius=FMath::Clamp(End ? EndCurbRadius : StartCurbRadius,0.f,W);
  const float EndDistance=End ? Length : 0, EndSign=End ? 1.f : -1.f;
  const FVector Origin=End ? Frames.Last().Position : Frames[0].Position;
  const FVector Direction=Spline->GetDirectionAtDistanceAlongSpline(EndDistance,ESplineCoordinateSpace::Local)*EndSign;
  const FVector Out=Direction.GetSafeNormal2D(), Across(-Out.Y,Out.X,0);
  const FVector SlopedOut=Direction/FMath::Max(0.0001,Direction.Size2D());
  TArray<LxRoad::FFrame> CapFrames;
  /** 添加沿真实端点坡度延伸的封口截面，保留解析切线与连续纹理距离。 */
  const auto AddFrame=[&](double X,double Y,FVector Tangent)
  {
   const FVector P=Origin+SlopedOut*X+Across*Y;
   if(!CapFrames.IsEmpty() && CapFrames.Last().Position.Equals(P,0.00001)) { return; }
   const float Distance=CapFrames.IsEmpty() ? 0.f : CapFrames.Last().Distance+FVector::Distance(CapFrames.Last().Position,P);
   CapFrames.Add({P,FVector(-Tangent.Y,Tangent.X,0),Distance});
  };
  if(Radius<0.01f)
  {
   // 直角端部横跨两侧路缘石的外边界，封住角部而不留下小三角缺口。
   AddFrame(0,-W-CW,Across); AddFrame(0,W+CW,Across);
  }
  else
  {
   const int32 Steps=FMath::Clamp(FMath::CeilToInt(HALF_PI*Radius/20),8,64);
   for(int32 I=0; I<=Steps; ++I)
   {
    // 显式固定四分之一圆端点，避免最大半径时浮点余弦留下极短边与退化碰撞。
    const double Angle=UE_DOUBLE_HALF_PI*I/Steps;
    const double S=I==0 ? 0. : (I==Steps ? 1. : FMath::Sin(Angle));
    const double C=I==0 ? 1. : (I==Steps ? 0. : FMath::Cos(Angle));
    AddFrame(Radius*S,-W+Radius*(1-C),Out*C+Across*S);
   }
   for(int32 I=0; I<=Steps; ++I)
   {
    const double Angle=UE_DOUBLE_HALF_PI*I/Steps;
    const double S=I==0 ? 0. : (I==Steps ? 1. : FMath::Sin(Angle));
    const double C=I==0 ? 1. : (I==Steps ? 0. : FMath::Cos(Angle));
    AddFrame(Radius*C,W-Radius+Radius*S,-Out*S+Across*C);
   }
   const FVector Down(0,0,-D);
   const float Tile=FMath::Max(1.f,RoadTextureSize);
   /** 封口铺面与主路端面使用同一纹理相位，并延续道路坡度的实际距离。 */
   const auto UV=[&](FVector P)
   {
    const FVector Delta=P-Origin;
    return FVector2D((3*W+D-FVector::DotProduct(Delta,Across)*EndSign)/Tile,
     (EndDistance+FVector::DotProduct(Delta,Out)*SlopedOut.Size()*EndSign)/Tile)+TextureOffset;
   };
   for(int32 I=0; I+1<CapFrames.Num(); ++I)
   {
    const FVector A=CapFrames[I].Position, B=CapFrames[I+1].Position;
    Road.Triangle(Origin,B,A,UV(Origin),UV(B),UV(A));
    Road.Triangle(Origin+Down,A+Down,B+Down,UV(Origin),UV(A),UV(B));
    Road.Quad(A,B,B+Down,A+Down,UV(A),UV(B),UV(B)+FVector2D(0,D/Tile),UV(A)+FVector2D(0,D/Tile));
    if(bBuildCollision) { Convex.Add({Origin,A,B,Origin+Down,A+Down,B+Down}); }
   }
  }
  TArray<FVector2D> Profile;
  if(Bevel>0.01f) { Profile={{-CW,-D},{0,-D},{0,CH-Bevel},{-Bevel,CH},{-CW+Bevel,CH},{-CW,CH-Bevel}}; }
  else { Profile={{-CW,-D},{0,-D},{0,CH},{-CW,CH}}; }
  LxRoad::Sweep(CapFrames,Profile,false,FMath::Max(1.f,CurbTextureSize),TextureOffset,EndCurbs,Convex,bBuildCollision);
 }
 Road.Submit(RoadMesh,0,RoadMaterial);
 for(int32 Side=0; Side<2; ++Side)
 { if(!Curbs[Side].Vertices.IsEmpty()) { Curbs[Side].Submit(RoadMesh,Side+1,CurbMaterial); } }
 if(!EndCurbs.Vertices.IsEmpty()) { EndCurbs.Submit(RoadMesh,3,CurbMaterial); }
 RoadMesh->SetCastShadow(bCastShadow);
 RoadMesh->bCastStaticShadow=false;
 RoadMesh->bCastDynamicShadow=bCastShadow;
 RoadMesh->SetCollisionEnabled(bBuildCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
 if(bBuildCollision) { RoadMesh->SetCollisionConvexMeshes(Convex); }
 CollisionDataVersion=1;
}
