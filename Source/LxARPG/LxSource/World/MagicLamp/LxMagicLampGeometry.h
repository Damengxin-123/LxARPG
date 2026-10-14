#pragma once
#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

namespace LxMagicLamp
{
 /** 一个材质分区的顶点和三角形缓冲。 */
 struct FSurface
 {
  /** 顶点位置。 */
  TArray<FVector> Vertices;
  /** 三角形索引。 */
  TArray<int32> Indices;
  /** 面或平滑法线。 */
  TArray<FVector> Normals;
  /** 材质纹理坐标。 */
  TArray<FVector2D> UVs;
  /** 按 UE 正面绕序写入四边面。 */
  void Quad(const FVector& A,const FVector& B,const FVector& C,const FVector& D)
  {
   const int32 I=Vertices.Num();
   const FVector N=FVector::CrossProduct(C-A,B-A).GetSafeNormal();
   Vertices.Append({A,B,C,D}); Normals.Append({N,N,N,N});
   UVs.Append({FVector2D(0,0),FVector2D(0,1),FVector2D(1,1),FVector2D(1,0)});
   Indices.Append({I,I+1,I+2,I,I+2,I+3});
  }
  /** 绘制闭合长方体。 */
  void Box(const FVector& Center,const FVector& Size)
  {
   const FVector E=Size*.5;
   const FVector P[]={Center+FVector(-E.X,-E.Y,-E.Z),Center+FVector(E.X,-E.Y,-E.Z),
    Center+FVector(E.X,E.Y,-E.Z),Center+FVector(-E.X,E.Y,-E.Z),
    Center+FVector(-E.X,-E.Y,E.Z),Center+FVector(E.X,-E.Y,E.Z),
    Center+FVector(E.X,E.Y,E.Z),Center+FVector(-E.X,E.Y,E.Z)};
   Quad(P[0],P[1],P[2],P[3]); Quad(P[4],P[7],P[6],P[5]);
   Quad(P[0],P[4],P[5],P[1]); Quad(P[1],P[5],P[6],P[2]);
   Quad(P[2],P[6],P[7],P[3]); Quad(P[3],P[7],P[4],P[0]);
  }
  /** 沿线段生成细八棱金属线，供符文和花瓣轮廓使用。 */
  void Line(const FVector& A,const FVector& B,double Width)
  {
   const FVector Axis=(B-A).GetSafeNormal();
   FVector U,V; Axis.FindBestAxisVectors(U,V); V=-V;
   for (int32 I=0;I<8;++I)
   {
    const double T=2*PI*I/8,Next=2*PI*(I+1)/8;
    const FVector P=(U*FMath::Cos(T)+V*FMath::Sin(T))*Width;
    const FVector Q=(U*FMath::Cos(Next)+V*FMath::Sin(Next))*Width;
    Quad(A+P,B+P,B+Q,A+Q);
   }
  }
  /** 在指定平面生成闭合圆环。 */
  void Ring(const FVector& Center,const FVector& U,const FVector& V,double Radius,double Width)
  {
   for (int32 I=0;I<64;++I)
   {
    const double A=2*PI*I/64,B=2*PI*(I+1)/64;
    Line(Center+(U*FMath::Cos(A)+V*FMath::Sin(A))*Radius,
     Center+(U*FMath::Cos(B)+V*FMath::Sin(B))*Radius,Width);
   }
  }
  /** 生成平滑球面晶核并避开极点退化三角形。 */
  void Sphere(double Radius)
  {
   constexpr int32 Longitude=48,Latitude=24;
   const int32 Start=Vertices.Num();
   for(int32 J=0;J<=Latitude;++J)
   {
    const double P=PI*J/Latitude;
    for(int32 I=0;I<=Longitude;++I)
    {
     const double T=2*PI*I/Longitude;
     const FVector N(FMath::Sin(P)*FMath::Cos(T),FMath::Sin(P)*FMath::Sin(T),FMath::Cos(P));
     Vertices.Add(N*Radius); Normals.Add(N); UVs.Add(FVector2D(double(I)/Longitude,double(J)/Latitude));
    }
   }
   for(int32 J=0;J<Latitude;++J) for(int32 I=0;I<Longitude;++I)
   {
    const int32 A=Start+J*(Longitude+1)+I,B=A+Longitude+1;
    if(J>0) { Indices.Append({A,A+1,B}); }
    if(J<Latitude-1) { Indices.Append({A+1,B+1,B}); }
   }
  }
  /** 上传非碰撞分区，灯具不阻挡角色或遮住自身光源。 */
  void Upload(UProceduralMeshComponent* Mesh,int32 Section) const
  {
   Mesh->CreateMeshSection_LinearColor(Section,Vertices,Indices,Normals,UVs,
    TArray<FLinearColor>(),TArray<FProcMeshTangent>(),false);
  }
 };
 /** 把非法输入转换为默认值并限制到可生成范围。 */
 static double Safe(double Value,double Default,double Min,double Max)
 { return FMath::IsFinite(Value)?FMath::Clamp(Value,Min,Max):Default; }

}
