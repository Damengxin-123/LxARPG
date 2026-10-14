#pragma once
#include "CoreMinimal.h"
#include "Algo/Reverse.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
namespace LxWoodenDoor
{
 /** 将所有倒角木构件直接写入一个网格描述，不为木板创建组件。 */
 struct FBuilder
 {
  /** 将顺纹坐标转换为竖直门体坐标，保持右手系。 */
  static FVector ToDoor(const FVector& P) { return FVector(P.Y,P.Z,P.X); }
  /** 门体几何描述。 */
  FMeshDescription Description;
  /** 三个固定材质分区：暖木板、深色边框与金属。 */
  FPolygonGroupID Groups[3];
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
   const FName Names[]={TEXT("木板"),TEXT("深色边框"),TEXT("金属")};
   for (int32 I=0; I<3; ++I)
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
    Attributes.GetVertexPositions()[Vertex]=FVector3f(ToDoor(Origin+Forward*P.X+Right*P.Y+FVector::UpVector*P.Z));
    const FVertexInstanceID Instance=Description.CreateVertexInstance(Vertex);
    Instances.Add(Instance);
    Attributes.GetVertexInstanceNormals()[Instance]=FVector3f(ToDoor(Normal));
    Attributes.GetVertexInstanceTangents()[Instance]=FVector3f(ToDoor(Tangent));
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
  /** 构造闭合的六面、十二条倒角边和八个倒角角面，每根门构件保持完整。 */
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
