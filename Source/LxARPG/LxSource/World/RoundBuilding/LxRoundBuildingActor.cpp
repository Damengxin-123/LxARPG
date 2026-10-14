#include "LxRoundBuildingActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "../GeneratedMesh/LxGeneratedMeshBuild.h"
#include "../GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "../WoodenDoor/LxWoodenDoorActor.h"
#include "ProceduralMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Operations/MeshBoolean.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace LxRoundBuilding
{
 using namespace UE::Geometry;
 /** 有效内部高度，确保楼板厚度始终朝向室内。 */
 double FloorTop(const ALxRoundBuildingActor& A) { return A.bFloor ? FMath::Clamp<double>(A.FloorThickness,1,A.WallHeight-2) : 0; }
 /** 有效天花板顶面，不超过墙高且保留室内空间。 */
 double CeilingTop(const ALxRoundBuildingActor& A) { return FMath::Clamp<double>(A.bCeilingFollowsWall ? A.WallHeight-FMath::Max(0.f,A.CeilingTopOffset) : A.CeilingHeight,FloorTop(A)+1,A.WallHeight); }
 /** 天花板下表面的有效高度。 */
 double CeilingBottom(const ALxRoundBuildingActor& A) { return CeilingTop(A)-(A.bCeiling ? FMath::Clamp<double>(A.CeilingThickness,1,CeilingTop(A)-FloorTop(A)) : 0); }
 /** 向实体添加共享顶点的四边形，保持布尔运算需要的外向绕序。 */
 void Quad(FDynamicMesh3& M,int A,int B,int C,int D) { M.AppendTriangle(A,C,B); M.AppendTriangle(A,D,C); }
 /** 创建闭合圆柱、圆环或环形扇区；所有边界共用顶点。 */
 void Ring(FDynamicMesh3& M,double Outer,double Inner,double Bottom,double Top,int Steps,double Begin=0,double End=UE_DOUBLE_TWO_PI,bool bFull=true)
 {
  const int Count=bFull ? Steps : Steps+1;
  TArray<int> OB,OT,IB,IT;
  for(int I=0; I<Count; ++I)
  {
   const double Angle=Begin+(End-Begin)*I/Steps;
   const FVector3d D(FMath::Cos(Angle),FMath::Sin(Angle),0);
   OB.Add(M.AppendVertex(D*Outer+FVector3d(0,0,Bottom))); OT.Add(M.AppendVertex(D*Outer+FVector3d(0,0,Top)));
   if(Inner>0) { IB.Add(M.AppendVertex(D*Inner+FVector3d(0,0,Bottom))); IT.Add(M.AppendVertex(D*Inner+FVector3d(0,0,Top))); }
  }
  const int CB=Inner>0 ? -1 : M.AppendVertex(FVector3d(0,0,Bottom));
  const int CT=Inner>0 ? -1 : M.AppendVertex(FVector3d(0,0,Top));
  for(int I=0; I<Steps; ++I)
  {
   const int J=(I+1)%Count;
   Quad(M,OB[I],OB[J],OT[J],OT[I]);
   if(Inner>0)
   {
    Quad(M,IB[J],IB[I],IT[I],IT[J]); Quad(M,OT[I],OT[J],IT[J],IT[I]); Quad(M,OB[J],OB[I],IB[I],IB[J]);
   }
   else { M.AppendTriangle(CT,OT[J],OT[I]); M.AppendTriangle(CB,OB[I],OB[J]); }
  }
  if(!bFull && Inner>0)
  { Quad(M,OB[0],OT[0],IT[0],IB[0]); Quad(M,OB.Last(),IB.Last(),IT.Last(),OT.Last()); }
 }
 /** 构造径向矩形切割体，边框是平面而非放射状楔形。 */
 FDynamicMesh3 OpeningBox(double Angle,double HalfWidth,double Near,double Far,double Bottom,double Top)
 {
  FDynamicMesh3 M;
  const FVector3d N(FMath::Cos(Angle),FMath::Sin(Angle),0), T(-N.Y,N.X,0);
  for(double Z : {Bottom,Top}) for(double Y : {-HalfWidth,HalfWidth}) for(double X : {Near,Far})
  { M.AppendVertex(N*X+T*Y+FVector3d(0,0,Z)); }
  Quad(M,0,2,3,1); Quad(M,4,5,7,6); Quad(M,0,1,5,4);
  Quad(M,2,6,7,3); Quad(M,0,4,6,2); Quad(M,1,3,7,5);
  return M;
 }
 /** 追加嵌入圆墙的平直石框块，前表面保持矩形轮廓。 */
 void TrimBlock(FDynamicMesh3& Mesh,double Angle,double Left,double Right,double Near,double Far,double Bottom,double Top)
 {
  if(Top-Bottom<0.01 || Right-Left<0.01) { return; }
  const FVector3d Tangent(-FMath::Sin(Angle),FMath::Cos(Angle),0);
  const FDynamicMesh3 Block=OpeningBox(Angle,(Right-Left)*0.5,Near,Far,Bottom,Top);
  TArray<int> Map;
  for(int V : Block.VertexIndicesItr()) { Map.Add(Mesh.AppendVertex(Block.GetVertex(V)+Tangent*((Left+Right)*0.5))); }
  for(int ID : Block.TriangleIndicesItr()) { const FIndex3i T=Block.GetTriangle(ID); Mesh.AppendTriangle(Map[T.A],Map[T.B],Map[T.C]); }
 }
 /** 通过闭合实体差集生成真实开洞；失败时保留诊断，不提交破损结果。 */
 bool Subtract(FDynamicMesh3& Mesh,const FDynamicMesh3& Cutter)
 {
  if(Mesh.TriangleCount()==0) { return true; }
  FDynamicMesh3 Result;
  FMeshBoolean Boolean(&Mesh,&Cutter,&Result,FMeshBoolean::EBooleanOp::Difference);
  Boolean.bSimplifyAlongNewEdges=true;
  if(!Boolean.Compute() || (Result.TriangleCount()>0 && !Result.IsClosed())) { return false; }
  Mesh=MoveTemp(Result); return true;
 }
 /** 输出程序化预览网格，切口法线保持平面，UV 按实际长度平铺。 */
 void Submit(const FDynamicMesh3& M,UProceduralMeshComponent* Component,int Slot,UMaterialInterface* Material,double Tile,bool bCollision,double Outer=0,double Inner=0)
 {
  if(M.TriangleCount()==0) { return; }
  TArray<FVector> V,N; TArray<int32> Indices; TArray<FVector2D> UV; TArray<FProcMeshTangent> Tangents;
  for(int ID : M.TriangleIndicesItr())
  {
   const FIndex3i Tri=M.GetTriangle(ID); const FVector Normal(M.GetTriNormal(ID));
   const FVector T=FMath::Abs(Normal.Z)>0.5 ? FVector::ForwardVector : FVector(-Normal.Y,Normal.X,0).GetSafeNormal();
   const FVector B=FVector::CrossProduct(Normal,T);
   const FVector Center(M.GetTriCentroid(ID));
   const double RadialDot=FVector::DotProduct(Normal,FVector(Center.X,Center.Y,0).GetSafeNormal());
   const bool bRound=Outer>0 && FMath::Abs(RadialDot)>0.995;
   const double Repeats=FMath::Max(1.,FMath::RoundToDouble(UE_DOUBLE_TWO_PI*(RadialDot>0 ? Outer : Inner)/Tile));
   const double CenterAngle=FMath::Atan2(Center.Y,Center.X);
   const int Base=V.Num();
   for(int Index : {Tri.A,Tri.B,Tri.C})
   {
    const FVector P(M.GetVertex(Index)); V.Add(P); N.Add(Normal);
    if(bRound)
    {
     // 跨圆周接缝的三角形展开角度，整圈整数重复保证砖缝闭合。
     double Angle=FMath::Atan2(P.Y,P.X);
     if(Angle-CenterAngle>UE_DOUBLE_PI) { Angle-=UE_DOUBLE_TWO_PI; }
     if(Angle-CenterAngle<-UE_DOUBLE_PI) { Angle+=UE_DOUBLE_TWO_PI; }
     UV.Add(FVector2D(Angle/UE_DOUBLE_TWO_PI*Repeats,P.Z/Tile));
    }
    else { UV.Add(FVector2D(FVector::DotProduct(P,T),FVector::DotProduct(P,B))/Tile); }
    // 内墙的法线朝里，切线与副切线仍须对应圆周 U 和向上 V。
    Tangents.Emplace(bRound && RadialDot<0 ? -T : T,bRound && RadialDot<0);
   }
   Indices.Append({Base,Base+1,Base+2});
  }
  Component->CreateMeshSection_LinearColor(Slot,V,Indices,N,UV,{},Tangents,bCollision);
  Component->SetMaterial(Slot,Material);
 }
}

ALxRoundBuildingActor::ALxRoundBuildingActor()
{
 PrimaryActorTick.bCanEverTick=false;
 // 原生类型也具有完整材质，并通过硬引用把这些默认资产纳入烹饪。
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultWall(TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/灰白错缝砖墙.灰白错缝砖墙"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultTrim(TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/深灰门窗石框.深灰门窗石框"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultFloor(TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/大块浅灰石板地面.大块浅灰石板地面"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultCeiling(TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/大块灰白石板天花.大块灰白石板天花"));
 WallMaterial=DefaultWall.Object; OpeningTrimMaterial=DefaultTrim.Object;
 FloorMaterial=DefaultFloor.Object; CeilingMaterial=DefaultCeiling.Object;
 BuildingMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("BuildingMesh")); SetRootComponent(BuildingMesh);
 BuildingMesh->bEditableWhenInherited=false; BuildingMesh->bUseAsyncCooking=false;
 BuildingMesh->bUseComplexAsSimpleCollision=true; BuildingMesh->SetCollisionProfileName(TEXT("BlockAll"));
 OptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("OptimizedMesh"));
 OptimizedMesh->SetupAttachment(BuildingMesh); OptimizedMesh->bEditableWhenInherited=false;
}
void ALxRoundBuildingActor::OnConstruction(const FTransform& Transform)
{
 Super::OnConstruction(Transform);
 if (!OptimizedMesh || !OptimizedMesh->IsSourceCurrent()) { RebuildBuilding(); }
}
void ALxRoundBuildingActor::PostRegisterAllComponents()
{
 Super::PostRegisterAllComponents();
 if(IsTemplate() || !OptimizedMesh) { return; }
 OptimizedMesh->SetRelativeTransform(FTransform::Identity);
 OptimizedMesh->TransformUpdated.RemoveAll(this);
 OptimizedMesh->TransformUpdated.AddWeakLambda(this,[this](USceneComponent*,EUpdateTransformFlags,ETeleportType)
 { if(!OptimizedMesh->GetRelativeTransform().Equals(FTransform::Identity)) { OptimizedMesh->SetRelativeTransform(FTransform::Identity); } });
}
void ALxRoundBuildingActor::CancelDeferredUpdate()
{ if(DeferredUpdate.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdate); DeferredUpdate.Reset(); } }
void ALxRoundBuildingActor::BeginDestroy() { CancelDeferredUpdate(); Super::BeginDestroy(); }

TArray<ALxRoundBuildingActor*> ALxRoundBuildingActor::FindNeighbors() const
{
 TArray<ALxRoundBuildingActor*> Result;
 if(!GetWorld() || !bNestedFloorHoles || IsActorBeingDestroyed() || IsTemplate()) { return Result; }
 if(!FMath::IsFinite(Radius) || !FMath::IsFinite(WallHeight) || Radius<100 || WallHeight<100
  || !GetActorScale3D().Equals(FVector::OneVector,0.001) || FVector::DotProduct(GetActorUpVector(),FVector::UpVector)<0.9999) { return Result; }
 for(TActorIterator<ALxRoundBuildingActor> It(GetWorld()); It; ++It)
 {
  auto* B=*It;
  if(B==this || B->IsActorBeingDestroyed() || !B->bNestedFloorHoles || B->IsTemplate()) { continue; }
  if(!FMath::IsFinite(B->Radius) || !FMath::IsFinite(B->WallHeight) || !FMath::IsFinite(B->CeilingHeight)
   || !FMath::IsFinite(B->CeilingTopOffset) || !FMath::IsFinite(B->FloorThickness) || B->Radius<100 || B->WallHeight<100) { continue; }
  if(!B->GetActorScale3D().Equals(FVector::OneVector,0.001) || FVector::DotProduct(B->GetActorUpVector(),FVector::UpVector)<0.9999) { continue; }
  const double Distance=FVector::Dist2D(GetActorLocation(),B->GetActorLocation());
  if(FMath::Abs(Radius-B->Radius)<0.01 || Distance+FMath::Min(Radius,B->Radius)>FMath::Max(Radius,B->Radius)+0.001) { continue; }
  if(GetActorLocation().Z+WallHeight<B->GetActorLocation().Z-0.01 || B->GetActorLocation().Z+B->WallHeight<GetActorLocation().Z-0.01) { continue; }
  Result.Add(B);
 }
 Result.Sort([](const ALxRoundBuildingActor& A,const ALxRoundBuildingActor& B){ return A.GetPathName()<B.GetPathName(); });
 return Result;
}
void ALxRoundBuildingActor::RebuildBuilding()
{
 if(IsTemplate() || !GetWorld() || IsActorBeingDestroyed()) { return; }
 CancelDeferredUpdate();
 TArray<ALxRoundBuildingActor*> Affected=FindNeighbors();
 for(auto Old : PreviousNeighbors) { if(Old.IsValid() && !Old->IsActorBeingDestroyed()) { Affected.AddUnique(Old.Get()); } }
 Affected.AddUnique(this);
 for(auto* A : Affected)
 {
  const auto Neighbors=A->FindNeighbors(); A->PreviousNeighbors.Reset();
  for(auto* N : Neighbors) { A->PreviousNeighbors.Add(N); }
#if WITH_EDITOR
  if(A!=this && !GetWorld()->IsGameWorld()) { A->Modify(); }
#endif
  A->BuildGeometry(Neighbors);
 }
}
void ALxRoundBuildingActor::Destroyed()
{
 CancelDeferredUpdate();
 const auto Neighbors=PreviousNeighbors;
 Super::Destroyed();
 if(GetWorld() && !GetWorld()->bIsTearingDown)
 { for(auto N : Neighbors) { if(N.IsValid() && !N->IsActorBeingDestroyed()) { N->RebuildBuilding(); } } }
}

void ALxRoundBuildingActor::BuildGeometry(const TArray<ALxRoundBuildingActor*>& Neighbors)
{
 using namespace LxRoundBuilding;
 // 只迁移初版无接缝石材和空材质；墙面与楼板分别使用砖墙和大石板，保留其他自定义材质。
 const auto* Defaults=GetDefault<ALxRoundBuildingActor>();
 const bool bLegacyWall=WallMaterial && WallMaterial->GetPathName()==TEXT("/Game/项目内容/美术资产/海港城市/砖墙/材质/灰白石砖.灰白石砖");
 if(!WallMaterial || bLegacyWall) { WallMaterial=Defaults->WallMaterial; }
 const bool bLegacyFloor=FloorMaterial && FloorMaterial->GetPathName()==TEXT("/Game/项目内容/美术资产/海港城市/砖墙/材质/灰白石砖.灰白石砖");
 const bool bLegacyCeiling=CeilingMaterial && CeilingMaterial->GetPathName()==TEXT("/Game/项目内容/美术资产/海港城市/砖墙/材质/灰白石砖.灰白石砖");
 if(!FloorMaterial || bLegacyFloor) { FloorMaterial=Defaults->FloorMaterial; }
 if(!CeilingMaterial || bLegacyCeiling) { CeilingMaterial=Defaults->CeilingMaterial; }
 if(!OpeningTrimMaterial) { OpeningTrimMaterial=Defaults->OpeningTrimMaterial; }
 ON_SCOPE_EXIT { OptimizedMesh->SetRelativeTransform(FTransform::Identity); OptimizedMesh->Synchronize(BuildingMesh); };
 BuildingMesh->ClearAllMeshSections(); BuildingMesh->ClearCollisionConvexMeshes(); GenerationDiagnostic.Reset();
 for(float V : {Radius,WallThickness,WallHeight,FloorThickness,CeilingThickness,CeilingHeight,CeilingTopOffset,OpeningTrimWidth,OpeningTrimDepth,TextureSize,NotchDepth,BattlementRatio})
 { if(!FMath::IsFinite(V)) { GenerationDiagnostic=TEXT("尺寸含有无效数值。"); return; } }
 if(Radius<100 || WallHeight<100 || WallThickness<5 || WallThickness>=Radius-10)
 { GenerationDiagnostic=TEXT("外半径至少100厘米、墙高至少100厘米，墙厚需要保留10厘米以上内部半径。"); return; }
 if(!GetActorScale3D().Equals(FVector::OneVector,0.001) || FVector::DotProduct(GetActorUpVector(),FVector::UpVector)<0.9999)
 { GenerationDiagnostic=TEXT("请使用半径、高度参数调整尺寸，建筑保持单位缩放和水平旋转。"); return; }
 const double Inner=Radius-WallThickness, FloorZ=FloorTop(*this), CeilingZ=CeilingTop(*this), BottomZ=CeilingBottom(*this);
 const double LowWall=bBattlements ? FMath::Max(CeilingZ,WallHeight-FMath::Max(0.f,NotchDepth)) : WallHeight;
 const int Segments=LxGeneratedMeshBuild::IsPreview(this)?24:FMath::Clamp(CircleSegments,24,256);
 FDynamicMesh3 Wall,Floor,Ceiling,Teeth,Trim;
 Ring(Wall,Radius,Inner,0,LowWall,Segments);
 if(bFloor) { Ring(Floor,Inner,0,0,FloorZ,Segments); }
 if(bCeiling) { Ring(Ceiling,Inner,0,BottomZ,CeilingZ,Segments); }
 for(FLxRoundBuildingOpening& O : Openings)
 {
  if(O.Position.ContainsNaN() || !FMath::IsFinite(O.Width) || !FMath::IsFinite(O.Height) || O.Width<10 || O.Width>=2*Inner || O.Height<10)
  { GenerationDiagnostic=TEXT("门窗净宽须小于内部直径，净宽和净高至少10厘米。"); return; }
  const double Angle=FMath::Atan2(O.Position.Y,O.Position.X);
  const double Base=O.bDoor ? FloorZ : FMath::Clamp<double>(O.Position.Z,FloorZ,BottomZ);
  if(Base+O.Height>BottomZ+0.001)
  { GenerationDiagnostic=FString::Printf(TEXT("%s 超出室内净高，请降低窗台或洞口高度、抬高天花板。"),*O.Name); return; }
  O.Position=FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,Base);
  const FDynamicMesh3 Cutter=OpeningBox(Angle,O.Width*0.5,0,Radius+10,Base-0.001,Base+O.Height);
  if(!Subtract(Wall,Cutter)) { GenerationDiagnostic=TEXT("门窗切割失败，请稍微移开完全重合的洞口边界。"); return; }
  if(bOpeningTrim)
  {
   const double Half=O.Width*0.5, Width=FMath::Clamp<double>(OpeningTrimWidth,1,Radius-Half-1);
   const double Front=Radius+FMath::Max(1.f,OpeningTrimDepth);
   const double Back=FMath::Sqrt(FMath::Max(0.,Radius*Radius-FMath::Square(Half+Width)))-2;
   const double Top=Base+O.Height, Cap=FMath::Min(LowWall,Top+Width);
   // 门框没有下横梁；窗框增加略外挑的石窗台。
   TrimBlock(Trim,Angle,-Half-Width,-Half,Back,Front,Base,Top);
   TrimBlock(Trim,Angle,Half,Half+Width,Back,Front,Base,Top);
   TrimBlock(Trim,Angle,-Half-Width,Half+Width,Back,Front,Top,Cap);
   if(!O.bDoor) { TrimBlock(Trim,Angle,-Half-Width,Half+Width,Back,Front+FMath::Max(1.f,OpeningTrimDepth)*0.4,FMath::Max(0.,Base-Width),Base); }
  }
 }
 // 邻近洞口可能与装饰相交，清理装饰以保证矩形通行净尺寸。
 if(bOpeningTrim)
 {
  for(const auto& O : Openings)
  {
   const double Angle=FMath::Atan2(O.Position.Y,O.Position.X);
   const auto Cutter=OpeningBox(Angle,O.Width*0.5,0,Radius+FMath::Max(10.f,OpeningTrimDepth*2+1),O.Position.Z,O.Position.Z+O.Height);
   if(!Subtract(Trim,Cutter)) { GenerationDiagnostic=TEXT("门窗装饰切割失败，请检查重合洞口。"); return; }
  }
 }
 for(auto* B : Neighbors)
 {
  const double RelativeZ=B->GetActorLocation().Z-GetActorLocation().Z;
  for(int Slot=0; Slot<2; ++Slot)
  {
   FDynamicMesh3& Slab=Slot==0 ? Floor : Ceiling;
   if(Slab.TriangleCount()==0) { continue; }
   const double Low=Slot==0 ? 0 : BottomZ, High=Slot==0 ? FloorZ : CeilingZ;
   if(Radius<B->Radius)
   {
    // 内嵌高塔在大厅内部或连接层的楼板整体让出通道，高于大厅的塔顶保持完整。
    if(High>=RelativeZ-0.01 && Low<=RelativeZ+CeilingTop(*B)+0.01) { Slab.Clear(); }
   }
   else if(High>=RelativeZ-0.01 && Low<=RelativeZ+CeilingTop(*B)+0.01)
   {
    FDynamicMesh3 Cutter;
    Ring(Cutter,B->Radius,0,Low-1,High+1,LxGeneratedMeshBuild::IsPreview(this)?24:FMath::Clamp(B->CircleSegments,24,256));
    // 切割圆周与高塔外墙使用相同分段和朝向，孔边与墙体精确相接。
    const FVector Local=GetActorTransform().InverseTransformPosition(B->GetActorLocation());
    const double Yaw=FMath::DegreesToRadians(B->GetActorRotation().Yaw-GetActorRotation().Yaw);
    for(int V : Cutter.VertexIndicesItr())
    {
     const FVector3d P=Cutter.GetVertex(V);
     Cutter.SetVertex(V,FVector3d(P.X*FMath::Cos(Yaw)-P.Y*FMath::Sin(Yaw)+Local.X,P.X*FMath::Sin(Yaw)+P.Y*FMath::Cos(Yaw)+Local.Y,P.Z));
    }
    if(!Subtract(Slab,Cutter)) { GenerationDiagnostic=TEXT("内嵌建筑楼板开孔失败，请检查圆周是否完全重合。"); return; }
   }
  }
 }
 if(bBattlements && WallHeight-LowWall>0.001)
 {
  const int Count=FMath::Clamp(BattlementCount,3,64);
  const double Period=UE_DOUBLE_TWO_PI/Count, Ratio=FMath::Clamp(BattlementRatio,0.1f,0.9f);
  for(int I=0; I<Count; ++I)
  { Ring(Teeth,Radius,Inner,LowWall,WallHeight,FMath::Max(2,FMath::CeilToInt(Segments*Ratio/Count)),I*Period,(I+Ratio)*Period,false); }
 }
 Submit(Wall,BuildingMesh,0,WallMaterial,FMath::Max(1.f,TextureSize),bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this),Radius,Inner);
 Submit(Floor,BuildingMesh,1,FloorMaterial,FMath::Max(1.f,TextureSize),bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this));
 Submit(Ceiling,BuildingMesh,2,CeilingMaterial,FMath::Max(1.f,TextureSize),bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this));
 Submit(Teeth,BuildingMesh,3,WallMaterial,FMath::Max(1.f,TextureSize),bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this),Radius,Inner);
 Submit(Trim,BuildingMesh,4,OpeningTrimMaterial ? OpeningTrimMaterial.Get() : WallMaterial.Get(),FMath::Max(1.f,TextureSize),bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this));
 BuildingMesh->SetCollisionEnabled(bEnableCollision && !LxGeneratedMeshBuild::IsPreview(this) ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
}

void ALxRoundBuildingActor::AddDoorOpening()
{
 Modify(); FLxRoundBuildingOpening O; const double A=Openings.Num()*2.399963229728653;
 O.Position=FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,LxRoundBuilding::FloorTop(*this));
 O.Name=FString::Printf(TEXT("门洞%d"),Openings.Num()+1); Openings.Add(O); RebuildBuilding();
}
void ALxRoundBuildingActor::AddWindowOpening()
{
 Modify(); FLxRoundBuildingOpening O; const double A=Openings.Num()*2.399963229728653;
 O.bDoor=false; O.Name=FString::Printf(TEXT("窗洞%d"),Openings.Num()+1);
 O.Position=FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,120); O.Width=100; O.Height=100;
 Openings.Add(O); RebuildBuilding();
}
bool ALxRoundBuildingActor::FitOpeningToDoor(int32 Index,ALxWoodenDoorActor* Door,float Clearance)
{
 if(!Openings.IsValidIndex(Index) || !IsValid(Door) || !FMath::IsFinite(Clearance)) { return false; }
 Modify(); auto& O=Openings[Index]; O.bDoor=true; O.Width=Door->Width+2*FMath::Max(0.f,Clearance); O.Height=Door->Height+FMath::Max(0.f,Clearance);
 RebuildBuilding(); return GenerationDiagnostic.IsEmpty();
}
FTransform ALxRoundBuildingActor::GetOpeningTransform(int32 Index) const
{
 if(!Openings.IsValidIndex(Index)) { return GetActorTransform(); }
 const auto& O=Openings[Index]; const double A=FMath::Atan2(O.Position.Y,O.Position.X);
 const double Half=O.Width*0.5;
 const double Depth=FMath::Sqrt(FMath::Max(0.,Radius*Radius-Half*Half));
 const FVector P(FMath::Cos(A)*Depth,FMath::Sin(A)*Depth,O.bDoor ? LxRoundBuilding::FloorTop(*this) : O.Position.Z);
 return FTransform(FRotator(0,FMath::RadiansToDegrees(A)+90,0),P)*GetActorTransform();
}
#if WITH_EDITOR
void ALxRoundBuildingActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 if(!IsTemplate() && (Event.ChangeType&EPropertyChangeType::Interactive)!=0)
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  UObject::PostEditChangeProperty(Event); return;
 }
 CancelDeferredUpdate(); Super::PostEditChangeProperty(Event);
}
void ALxRoundBuildingActor::PostEditMove(bool bFinished)
{ LxGeneratedMeshBuild::RequestPreview(this); if(bFinished) { CancelDeferredUpdate(); Super::PostEditMove(true); RebuildBuilding(); } }
void ALxRoundBuildingActor::PostEditUndo() { CancelDeferredUpdate(); Super::PostEditUndo(); RebuildBuilding(); }
#endif
