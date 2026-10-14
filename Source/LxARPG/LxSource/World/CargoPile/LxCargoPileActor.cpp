#include "LxCargoPileActor.h"
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

namespace LxCargoPile
{
 /** 将所有倒角货物构件直接写入一个网格描述，不为货物构件创建组件。 */
 struct FBuilder
 {
  /** 货堆的唯一几何描述。 */
  FMeshDescription Description;
  /** 木、铁、布、绳使用四个固定材质分区。 */
  FPolygonGroupID Groups[4];
  /** 当前写入的材质分区。 */
  int32 Slot = 0;
  /** 当前货物构件的稳定纹理偏移，同块构件六面共享。 */
  float Phase = 0;
  /** 当前货物底面中心与水平旋转。 */
  FTransform CargoTransform;
  /** 构件旋转使木纹沿板材长轴延伸。 */
  FQuat PartRotation = FQuat::Identity;
  /** 回转构件的木纹沿竖直方向。 */
  bool bVerticalGrain = false;
  /** 注册属性并建立固定槽位。 */
  FBuilder()
  {
   FStaticMeshAttributes Attributes(Description);
   Attributes.Register();
   Attributes.GetVertexInstanceUVs().SetNumChannels(3);
   const FName Names[]={TEXT("暖木"),TEXT("铁箍"),TEXT("麻布"),TEXT("绳索")};
   for (int32 I=0; I<4; ++I)
   {
    Groups[I]=Description.CreatePolygonGroup();
    Attributes.GetPolygonGroupMaterialSlotNames()[Groups[I]]=Names[I];
   }
  }
  /** 保存各顶点相同的构件局部三维坐标，令正面、侧面和倒角处纹理连续。 */
  void Face(TArray<FVector> Points, const FVector& Outward, const FVector& Origin,
   const FVector& Forward, const FLinearColor& Color)
  {
   if (FVector::DotProduct(FVector::CrossProduct(Points[2]-Points[0],Points[1]-Points[0]),Outward)<0)
   { Algo::Reverse(Points); }
   /** 将构件局部坐标变换到货物堆空间。 */
   const auto Map=[&](const FVector& P) { return CargoTransform.TransformPosition(Origin+PartRotation.RotateVector(P)); };
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
    Attributes.GetVertexInstanceUVs().Set(Instance,0,FVector2f((bVerticalGrain?P.Z:P.X)/100+Phase,P.Y/100));
    Attributes.GetVertexInstanceUVs().Set(Instance,1,FVector2f((bVerticalGrain?P.X:P.Z)/100,Phase));
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
  /** 构造闭合的六面、十二条倒角边和八个倒角角面，同一块货物不按墙面拆分。 */
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


namespace LxCargoPile
{
 /** 标准海运货物尺寸，以底面中心为原点。 */
 static FVector SizeFor(ELxCargoKind Kind)
 {
  const FVector Sizes[]={FVector(120,90,85),FVector(62,50,48),FVector(175,58,62),FVector(72,72,98),
   FVector(68,58,64),FVector(52,50,85),FVector(78,55,38),FVector(178,80,48),FVector(90,68,58)};
  return Sizes[static_cast<uint8>(Kind)];
 }
 /** 按形态返回保守顶面支撑尺寸；扎口麻袋不继续承重。 */
 static FVector2D SupportFor(ELxCargoKind Kind,const FVector& Size)
 {
  if (Kind<=ELxCargoKind::LongCrate) { return FVector2D(Size.X,Size.Y); }
  if (Kind==ELxCargoKind::Barrel) { return FVector2D(Size.X*.62,Size.Y*.62); }
  if (Kind==ELxCargoKind::Timber) { return FVector2D(Size.X*.94,Size.Y*.85); }
  if (Kind==ELxCargoKind::ClothBale) { return FVector2D(Size.X*.72,Size.Y*.72); }
  return FVector2D::ZeroVector;
 }
 /** 圆形、软袋底部小于最大轮廓，使用实际底部范围估计支撑。 */
 static FVector2D ContactFor(ELxCargoKind Kind,const FVector& Size)
 {
  const double Factor=Kind==ELxCargoKind::Barrel?.6:(Kind>=ELxCargoKind::GrainSack && Kind<=ELxCargoKind::FlatSack?.65:1.0);
  return FVector2D(Size.X*Factor,Size.Y*Factor);
 }
 /** 水平旋转后的矩形外接尺寸。 */
 static FVector2D Envelope(const FVector2D& Size,double Yaw)
 {
  const double C=FMath::Abs(FMath::Cos(FMath::DegreesToRadians(Yaw))),S=FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Yaw)));
  return FVector2D(C*Size.X+S*Size.Y,S*Size.X+C*Size.Y);
 }
 /** 写入一个闭合倒角板材，可独立设置木纹朝向。 */
 static void Box(FBuilder& B,int32 Slot,FVector Center,FVector Size,float Shade=1.f,FQuat Rotation=FQuat::Identity)
 {
  B.Slot=Slot; B.PartRotation=Rotation; B.bVerticalGrain=false;
  B.Block(Center,Size,FVector::ForwardVector,FMath::Min(.65f,static_cast<float>(Size.GetMin()*.13)),FLinearColor(Shade,Shade,Shade,1));
 }
 /** 由椭圆截面扫掠封闭体，用于木桶、铁箍和不同扎口麻袋。 */
 static void Lathe(FBuilder& B,int32 Slot,FVector2D Radius,const TArray<FVector2D>& Profile,float Shade,int32 Segments=20)
 {
  B.Slot=Slot; B.PartRotation=FQuat::Identity; B.bVerticalGrain=true;
  TArray<TArray<FVector>> Rings;
  for (const FVector2D& Level : Profile)
  {
   TArray<FVector> Ring;
   for (int32 I=0; I<Segments; ++I)
   {
    const double Angle=I*2*PI/Segments;
    const double HeightFraction=Level.X/FMath::Max(.01,Profile.Last().X);
    const double Gather=FMath::Pow(HeightFraction,6);
    const double Fold=Slot==2?1+(.035+.20*Gather)*FMath::Sin(Angle*7+HeightFraction*2):1;
    // 袋口留出不规则折叠布边，避免呈现硬质瓶塞的形态。
    const double Z=Slot==2?Level.X-Gather*Profile.Last().X*.028*(.5+.5*FMath::Sin(Angle*7)):Level.X;
    Ring.Add(FVector(Radius.X*Level.Y*Fold*FMath::Cos(Angle),Radius.Y*Level.Y*Fold*FMath::Sin(Angle),Z));
   }
   Rings.Add(MoveTemp(Ring));
  }
  const int32 FirstSide=B.Description.VertexInstances().Num();
  for (int32 R=0; R+1<Rings.Num(); ++R) for (int32 I=0; I<Segments; ++I)
  {
   const int32 J=(I+1)%Segments;
   const FVector A=Rings[R][I],C=Rings[R+1][J];
   const float StaveShade=Slot==0?Shade*(I%3==0?.88f:1.f):Shade;
   B.Face({A,Rings[R+1][I],C},FVector(A.X,A.Y,0),FVector::ZeroVector,FVector::ForwardVector,FLinearColor(StaveShade,StaveShade,StaveShade,1));
   B.Face({A,C,Rings[R][J]},FVector(A.X,A.Y,0),FVector::ZeroVector,FVector::ForwardVector,FLinearColor(StaveShade,StaveShade,StaveShade,1));
  }
  // 麻袋和桶身跨截面平滑着色，端盖仍保持硬边。
  FStaticMeshAttributes Attributes(B.Description); TMap<FVector3f,FVector3f> NormalSums;
  const int32 LastSide=B.Description.VertexInstances().Num();
  for (int32 I=FirstSide; I<LastSide; ++I)
  {
   const FVertexInstanceID VI(I); const FVector3f Pos=Attributes.GetVertexPositions()[B.Description.GetVertexInstanceVertex(VI)];
   NormalSums.FindOrAdd(Pos)+=Attributes.GetVertexInstanceNormals()[VI];
  }
  for (int32 I=FirstSide; I<LastSide; ++I)
  {
   const FVertexInstanceID VI(I); const FVector3f Pos=Attributes.GetVertexPositions()[B.Description.GetVertexInstanceVertex(VI)];
   const FVector3f N=NormalSums[Pos].GetSafeNormal(); Attributes.GetVertexInstanceNormals()[VI]=N;
   const FVector3f T=Attributes.GetVertexInstanceTangents()[VI];
   Attributes.GetVertexInstanceTangents()[VI]=(T-N*FVector3f::DotProduct(T,N)).GetSafeNormal();
  }
  for (int32 Side=0; Side<2; ++Side)
  {
   const auto& Ring=Side?Rings.Last():Rings[0]; const FVector Center(0,0,Ring[0].Z);
   for (int32 I=0; I<Segments; ++I)
   { B.Face({Center,Ring[I],Ring[(I+1)%Segments]},FVector(0,0,Side?1:-1),FVector::ZeroVector,FVector::ForwardVector,FLinearColor(Shade,Shade,Shade,1)); }
  }
 }
 /** 绳索或窄带沿盒状货物闭合捆扎，不伸出预留货物范围。 */
 static void Straps(FBuilder& B,const FVector& S,int32 Slot=3)
 {
  const double T=FMath::Min(S.X,S.Y)*.032;
  for (double X : {-S.X*.28,S.X*.28})
  {
   Box(B,Slot,FVector(X,0,S.Z-T*.5),FVector(T,S.Y*.99,T));
   for (double Y : {-S.Y/2+T/2,S.Y/2-T/2}) { Box(B,Slot,FVector(X,Y,S.Z/2),FVector(T,T,S.Z)); }
  }
 }
 /** 生成带独立板缝、加固边框和钉头的海运木箱。 */
 static void Crate(FBuilder& B,const FVector& S)
 {
  const double T=FMath::Min(S.X,S.Y)*.075;
  Box(B,0,FVector(0,0,S.Z/2),FVector(S.X-T,S.Y-T,S.Z-T),.55f);
  const int32 Boards=FMath::Max(3,FMath::RoundToInt(S.Y/14)); const double Step=S.Y/Boards;
  // 箱盖和底板顺长轴排布。
  for (int32 I=0; I<Boards; ++I) for (double Z : {T*.5,S.Z-T*.5})
  { Box(B,0,FVector(0,-S.Y/2+Step*(I+.5),Z),FVector(S.X,Step-.5,T),.96f-(I%3)*.04f); }
  const int32 Rows=FMath::Max(3,FMath::RoundToInt(S.Z/17)); const double RH=S.Z/Rows;
  for (int32 I=0; I<Rows; ++I)
  {
   const double Z=RH*(I+.5);
   for (double Y : {-S.Y/2+T/2,S.Y/2-T/2}) { Box(B,0,FVector(0,Y,Z),FVector(S.X-T,RH-.5,T),.88f,FQuat(FVector::ForwardVector,PI/2)); }
   for (double X : {-S.X/2+T/2,S.X/2-T/2})
   { Box(B,0,FVector(X,0,Z),FVector(S.Y-T,RH-.5,T),.92f,FRotator(0,90,90).Quaternion()); }
  }
  // 角柱与横向压条形成厚实但克制的木质框架。
  for (double X : {-S.X/2+T/2,S.X/2-T/2}) for (double Y : {-S.Y/2+T/2,S.Y/2-T/2})
  { Box(B,0,FVector(X,Y,S.Z/2),FVector(S.Z,T,T),.73f,FQuat(FVector::RightVector,PI/2)); }
  for (double X : {-S.X*.32,S.X*.32})
  {
   Box(B,0,FVector(X,0,S.Z-T/2),FVector(S.Y,T*1.2,T),.7f,FQuat(FVector::UpVector,PI/2));
   for (double Y : {-S.Y/2+T/2,S.Y/2-T/2})
   {
    Box(B,0,FVector(X,Y,S.Z/2),FVector(S.Z,T,T),.7f,FQuat(FVector::RightVector,PI/2));
    for (double Z : {T*1.4,S.Z-T*1.4}) { Box(B,1,FVector(X,Y+(Y>0?1:-1)*T*.44,Z),FVector(T*.25,T*.1,T*.25)); }
   }
  }
 }
 /** 生成可辨识的九类货物，并共用四个材质槽。 */
 static void BuildCargo(FBuilder& B,const FLxCargoPlacement& P,int32 Index)
 {
  B.CargoTransform=FTransform(FRotator(0,P.Yaw,0),P.Position); B.Phase=Index*.731f;
  const FVector S=P.Size;
  if (P.Kind<=ELxCargoKind::LongCrate) { Crate(B,S); return; }
  if (P.Kind==ELxCargoKind::Barrel)
  {
   const FVector2D R(S.X*.48,S.Y*.48);
   Lathe(B,0,R,{{0,.87},{S.Z*.08,.92},{S.Z*.3,1},{S.Z*.7,1},{S.Z*.92,.92},{S.Z,.87}},.9f);
   for (double H : {.09,.25,.75,.91})
   {
    const double Radius=H<.15 || H>.85?.965:1.04;
    Lathe(B,1,R,{{S.Z*(H-.022),Radius},{S.Z*(H+.022),Radius}},.95f);
   }
   // 桶盖接缝直接刻画为窄深色条，保留完整承重顶面。
   for (double Y : {-S.Y*.2,0.0,S.Y*.2})
   {
    const double Half=FMath::Sqrt(FMath::Square(S.X*.41)-Y*Y);
    Box(B,0,FVector(0,Y,S.Z-.12),FVector(Half*2,.55,.24),.35f);
   }
   Box(B,0,FVector(S.X*.14,0,S.Z-.15),FVector(6,6,.3),.55f);
   return;
  }
  if (P.Kind>=ELxCargoKind::GrainSack && P.Kind<=ELxCargoKind::FlatSack)
  {
   const FVector2D R(S.X*.48,S.Y*.48);
   Lathe(B,2,R,{{0,.65},{S.Z*.08,.86},{S.Z*.3,1},{S.Z*.58,.99},{S.Z*.76,.79},{S.Z*.88,.22},{S.Z*.94,.19},{S.Z, .32}},.95f,24);
   Lathe(B,3,R,{{S.Z*.875,.25},{S.Z*.92,.25}},.85f,16);
   // 折叠封口与一条可读的缝线。
   // 扎口结与两条下垂绳头，不使用类似瓶盖的顶部方块。
   Box(B,3,FVector(0,-S.Y*.12,S.Z*.90),FVector(S.X*.10,S.Y*.045,S.Z*.035),.92f);
   Box(B,3,FVector(-S.X*.045,-S.Y*.15,S.Z*.83),FVector(S.X*.025,S.Y*.025,S.Z*.14),.85f,FRotator(0,0,12).Quaternion());
   Box(B,3,FVector(S.X*.035,-S.Y*.16,S.Z*.85),FVector(S.X*.024,S.Y*.024,S.Z*.10),.85f,FRotator(0,0,-15).Quaternion());
   return;
  }
  if (P.Kind==ELxCargoKind::Timber)
  {
   for (int32 Z=0; Z<3; ++Z) for (int32 Y=0; Y<4; ++Y)
   { Box(B,0,FVector(0,(Y-1.5)*S.Y*.235,Z*S.Z/3+S.Z*.15),FVector(S.X*.98,S.Y*.228,S.Z*.30),.82f+.04f*((Y+Z)%3)); }
   Straps(B,S); return;
  }
  Box(B,2,FVector(0,0,S.Z/2),FVector(S.X*.98,S.Y*.98,S.Z),.88f); Straps(B,S);
 }
}

ALxCargoPileActor::ALxCargoPileActor()
{
 PrimaryActorTick.bCanEverTick=false;
#if WITH_EDITORONLY_DATA
 bRunConstructionScriptOnDrag=false;
#endif
 PileMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CargoPileMesh"));
 SetRootComponent(PileMesh); PileMesh->SetMobility(EComponentMobility::Movable);
 PileMesh->SetCollisionProfileName(TEXT("BlockAll")); PileMesh->SetGenerateOverlapEvents(false);
}
void ALxCargoPileActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); UpdatePileIfNeeded(); }
void ALxCargoPileActor::PostRegisterAllComponents() { Super::PostRegisterAllComponents(); if (PileMesh) { UpdatePileIfNeeded(); } }
void ALxCargoPileActor::BeginPlay() { Super::BeginPlay(); UpdatePileIfNeeded(); }
void ALxCargoPileActor::RebuildPile() { CancelDeferredUpdate(); bGeometryCached=false; UpdatePileIfNeeded(); RestorePileRegistration(); }
void ALxCargoPileActor::RerollPile() { RandomSeed=RandomSeed==MAX_int32?0:RandomSeed+1; RebuildPile(); }
TArray<double> ALxCargoPileActor::GetGeometrySettings() const
{
 return {LxGeneratedMeshBuild::CacheVersion,LxGeneratedMeshBuild::IsPreview(this)?1.0:0.0,bEnableNanite?1.0:0.0,SizeHandle.X,SizeHandle.Y,SizeHandle.Z,StackHeight,Density,Disorder,CargoScale,static_cast<double>(RandomSeed),static_cast<double>(MaxCargoCount),
  bCrates?1.0:0.0,bBarrels?1.0:0.0,bSacks?1.0:0.0,bTimber?1.0:0.0,bCloth?1.0:0.0};
}
void ALxCargoPileActor::UpdatePileIfNeeded()
{
 if (IsTemplate()) { return; }
 if (bGeometryCached && PileMesh->GetStaticMesh() && CachedGeometrySettings==GetGeometrySettings()) { RefreshMaterialsAndCollision(); return; }
 bGeometryCached=false; ++GeometryBuildCount;
 GeneratedMeshCache=nullptr; PileMesh->SetStaticMesh(nullptr); Placements.Reset();
 GeneratedCargoCount=0; GeneratedTriangleCount=0; GeneratedHeight=0; GenerationDiagnostic.Reset(); RefreshMaterialsAndCollision();
 for (double V : GetGeometrySettings()) { if (!FMath::IsFinite(V)) { GenerationDiagnostic=TEXT("尺寸或分布参数包含无效数值。"); return; } }
 SizeHandle=FVector(FMath::Clamp(SizeHandle.X,60.0,10000.0),FMath::Clamp(SizeHandle.Y,60.0,10000.0),0);
 StackHeight=FMath::Clamp(StackHeight,30.f,1500.f); Density=FMath::Clamp(Density,0.f,1.f); Disorder=FMath::Clamp(Disorder,0.f,1.f);
 CargoScale=FMath::Clamp(CargoScale,.5f,2.f); MaxCargoCount=FMath::Clamp(MaxCargoCount,1,256);
 TArray<ELxCargoKind> Kinds;
 for (int32 I=0; I<9; ++I)
 {
  if ((I<=2 && bCrates) || (I==3 && bBarrels) || (I>=4 && I<=6 && bSacks) || (I==7 && bTimber) || (I==8 && bCloth))
  { Kinds.Add(static_cast<ELxCargoKind>(I)); }
 }
 if (Kinds.IsEmpty() || Density<=0) { GenerationDiagnostic=TEXT("没有启用的货物种类或密度为零。"); return; }
 FRandomStream Random(RandomSeed);
 /** 每个底层货物独占的安全堆位，宽度随货物而变。 */
 struct FSlot
 {
  /** 水平占地区间。 */
  FVector2D Min,Max;
  /** 底层货物类型。 */
  ELxCargoKind Kind;
  /** 底层朝向。 */
  float Yaw;
 };
 TArray<FSlot> Slots;
 const int32 Rows=FMath::Max(1,FMath::FloorToInt(SizeHandle.Y/(150*CargoScale)));
 const double CD=SizeHandle.Y/Rows,Gap=10*CargoScale;
 // 按行紧凑装箱，每个底层货物按实际转向占地分配宽度，避免大网格造成稀疏。
 for (int32 Row=0; Row<Rows && Slots.Num()<4096; ++Row)
 {
  double Cursor=0;
  while (Cursor<SizeHandle.X-20*CargoScale && Slots.Num()<4096)
  {
   bool bFound=false;
   for (int32 Attempt=0; Attempt<48; ++Attempt)
   {
    const bool PreferCrate=bCrates && Random.FRand()<.55;
    const ELxCargoKind Kind=PreferCrate?static_cast<ELxCargoKind>(Random.RandRange(0,2)):Kinds[Random.RandRange(0,Kinds.Num()-1)];
    const FVector S=LxCargoPile::SizeFor(Kind)*CargoScale;
    const float Yaw=Random.FRandRange(-32,32)*Disorder;
    const FVector2D Foot=LxCargoPile::Envelope(FVector2D(S.X,S.Y),Yaw);
    if (S.Z>StackHeight || Foot.X+Gap>SizeHandle.X-Cursor || Foot.Y>CD-2) { continue; }
    FSlot Slot; Slot.Min=FVector2D(Cursor,Row*CD); Slot.Max=FVector2D(Cursor+Foot.X+Gap,(Row+1)*CD); Slot.Kind=Kind; Slot.Yaw=Yaw;
    Slots.Add(Slot); Cursor=Slot.Max.X; bFound=true; break;
   }
   if (!bFound) { break; }
  }
 }
 TArray<int32> Selected; for (int32 I=0; I<Slots.Num(); ++I) { Selected.Add(I); }
 for (int32 I=Selected.Num()-1; I>0; --I) { Selected.Swap(I,Random.RandRange(0,I)); }
 if (!Selected.IsEmpty()) { Selected.SetNum(FMath::Clamp(FMath::RoundToInt(Selected.Num()*Density),1,Selected.Num())); }
 Selected.Sort();
 for (int32 SlotIndex : Selected)
 {
  if (Placements.Num()>=MaxCargoCount) { break; }
  const FSlot& Slot=Slots[SlotIndex]; const double CW=Slot.Max.X-Slot.Min.X;
  int32 Parent=INDEX_NONE;
  // 限制一列尝试次数及层数，低剩余高度或柔软袋顶会自然结束。
  for (int32 Layer=0; Layer<32 && Placements.Num()<MaxCargoCount; ++Layer)
  {
   bool bPlaced=false;
   for (int32 Attempt=0; Attempt<48; ++Attempt)
   {
    FLxCargoPlacement P;
    // 底层适当偏向结实木箱，麻袋和木桶仍可直接落地。
    P.Kind=Layer==0?Slot.Kind:Kinds[Random.RandRange(0,Kinds.Num()-1)];
    P.Size=LxCargoPile::SizeFor(P.Kind)*CargoScale;
    const FLxCargoPlacement* Base=Parent!=INDEX_NONE?&Placements[Parent]:nullptr;
    P.Yaw=Base?Base->Yaw+Random.FRandRange(-32,32)*Disorder:Slot.Yaw;
    P.Footprint=LxCargoPile::Envelope(FVector2D(P.Size.X,P.Size.Y),P.Yaw);
    P.Position=Base?Base->Position:FVector((Slot.Min.X+Slot.Max.X)/2,(Slot.Min.Y+Slot.Max.Y)/2,0);
    P.Position.Z=Base?Base->Position.Z+Base->Size.Z:0;
    if (P.Position.Z+P.Size.Z>StackHeight+.001 || P.Footprint.X>CW-2 || P.Footprint.Y>CD-2) { continue; }
    if (Base)
    {
     const FVector2D Contact=LxCargoPile::Envelope(LxCargoPile::ContactFor(P.Kind,P.Size),P.Yaw-Base->Yaw);
     const FVector2D Margin=(Base->SupportSize-Contact)*.5;
     if (Margin.X<-.001 || Margin.Y<-.001) { continue; }
     const FVector LocalOffset(Random.FRandRange(-1,1)*FMath::Max(0.0,Margin.X)*Disorder*.65,Random.FRandRange(-1,1)*FMath::Max(0.0,Margin.Y)*Disorder*.65,0);
     P.Position+=FRotator(0,Base->Yaw,0).RotateVector(LocalOffset);
    }
    else
    {
     P.Position.X+=Random.FRandRange(-1,1)*(CW-P.Footprint.X-2)*.35*Disorder;
     P.Position.Y+=Random.FRandRange(-1,1)*(CD-P.Footprint.Y-2)*.35*Disorder;
    }
    if (P.Position.X-P.Footprint.X/2<Slot.Min.X || P.Position.X+P.Footprint.X/2>Slot.Max.X
     || P.Position.Y-P.Footprint.Y/2<Slot.Min.Y || P.Position.Y+P.Footprint.Y/2>Slot.Max.Y) { continue; }
    P.SupportSize=LxCargoPile::SupportFor(P.Kind,P.Size); P.SupportIndex=Parent;
    Parent=Placements.Add(P); bPlaced=true;
    GeneratedHeight=FMath::Max(GeneratedHeight,static_cast<float>(P.Position.Z+P.Size.Z));
    break;
   }
   if (!bPlaced || Placements[Parent].SupportSize.IsNearlyZero()) { break; }
  }
 }
 if (Placements.IsEmpty()) { GenerationDiagnostic=TEXT("当前范围或高度放不下已启用的货物，请扩大范围、提高高度或减小货物尺寸倍率。"); return; }
 LxCargoPile::FBuilder Builder; FKAggregateGeom Collision;
 for (int32 I=0; I<Placements.Num(); ++I)
 {
  const auto& P=Placements[I];
  if(LxGeneratedMeshBuild::IsPreview(this))
  {
   Builder.CargoTransform=FTransform(FRotator(0,P.Yaw,0),P.Position);
   Builder.Block(FVector(0,0,P.Size.Z*.5),P.Size,FVector::ForwardVector,0,FLinearColor::White);
  }
  else { LxCargoPile::BuildCargo(Builder,P,I); }
  FKBoxElem Box; Box.Center=P.Position+FVector(0,0,P.Size.Z/2); Box.Rotation=FRotator(0,P.Yaw,0);
  Box.X=P.Size.X; Box.Y=P.Size.Y; Box.Z=P.Size.Z; Collision.BoxElems.Add(Box);
 }
 UStaticMesh* Mesh=NewObject<ULxGeneratedStaticMesh>(this,NAME_None,RF_Transactional);
 UMaterialInterface* Materials[]={WoodMaterial.Get(),MetalMaterial.Get(),ClothMaterial.Get(),RopeMaterial.Get()};
 const FName Names[]={TEXT("暖木"),TEXT("铁箍"),TEXT("麻布"),TEXT("绳索")};
 int32 Section=0;
 for (int32 I=0; I<4; ++I)
 {
  Mesh->GetStaticMaterials().Emplace(Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface),Names[I]);
#if WITH_EDITOR
  if (Builder.Description.GetNumPolygonGroupPolygons(Builder.Groups[I])>0)
  { FMeshSectionInfo Info; Info.MaterialIndex=I; Info.bEnableCollision=true; Mesh->GetSectionInfoMap().Set(0,Section++,Info); }
#endif
 }
 Mesh->CreateBodySetup(); Mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseSimpleAsComplex;
 if (!LxGeneratedMeshBuild::Build(Mesh,Builder.Description,bEnableNanite)) { GenerationDiagnostic=TEXT("货物合并网格生成失败。"); return; }
 UBodySetup* Body=Mesh->GetBodySetup(); Body->AggGeom=MoveTemp(Collision); Body->DefaultInstance.SetCollisionProfileName(TEXT("BlockAll"));
 Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes(); GeneratedMeshCache=Mesh;
 PileMesh->SetStaticMesh(Mesh);
 GeneratedCargoCount=Placements.Num(); GeneratedTriangleCount=Builder.Description.Triangles().Num();
 CachedGeometrySettings=GetGeometrySettings(); bGeometryCached=true; RefreshMaterialsAndCollision();
}
void ALxCargoPileActor::RefreshMaterialsAndCollision()
{
 UMaterialInterface* Materials[]={WoodMaterial.Get(),MetalMaterial.Get(),ClothMaterial.Get(),RopeMaterial.Get()};
 for (int32 I=0; I<4; ++I)
 {
  UMaterialInterface* M=Materials[I]?Materials[I]:UMaterial::GetDefaultMaterial(MD_Surface);
  if (PileMesh->GetMaterial(I)!=M) { PileMesh->SetMaterial(I,M); }
 }
 const ECollisionEnabled::Type Mode=bEnableCollision && PileMesh->GetStaticMesh()?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision;
 if (PileMesh->GetCollisionEnabled()!=Mode) { PileMesh->SetCollisionEnabled(Mode); }
 LxGeneratedMeshBuild::RefreshMaterialCompatibility(PileMesh);
}
void ALxCargoPileActor::RestorePileRegistration()
{
 if (!IsTemplate() && GetWorld() && !IsActorBeingDestroyed() && !PileMesh->IsRegistered()) { RegisterAllComponents(); }
}
void ALxCargoPileActor::CancelDeferredUpdate()
{
 if (DeferredUpdateHandle.IsValid()) { FTSTicker::RemoveTicker(DeferredUpdateHandle); DeferredUpdateHandle.Reset(); }
}

void ALxCargoPileActor::BeginDestroy()
{
 CancelDeferredUpdate();
 Super::BeginDestroy();
}

#if WITH_EDITOR
void ALxCargoPileActor::PreEditChange(FProperty* PropertyAboutToChange)
{
 // 宽深控件传入 nullptr；自身数值参数不会替换组件，无需提前注销旧预览。
 const bool bPileValue=!PropertyAboutToChange
  || (PropertyAboutToChange->GetOwnerStruct()==StaticClass() && !CastField<FObjectPropertyBase>(PropertyAboutToChange));
 if (!IsTemplate() && bPileValue)
 {
  UObject::PreEditChange(PropertyAboutToChange);
  return;
 }
 // 材质引用、继承属性与模板仍保留演员原有的完整编辑流程。
 Super::PreEditChange(PropertyAboutToChange);
}

void ALxCargoPileActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
 LxGeneratedMeshBuild::RequestPreview(this);
 const bool bHandle=Event.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxCargoPileActor,SizeHandle)
  || Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(ALxCargoPileActor,SizeHandle);
 if (!IsTemplate() && (bHandle || (Event.ChangeType&EPropertyChangeType::Interactive)!=0))
 {
  LxGeneratedMeshBuild::RequestPreview(this);
  // AActor 的默认实现会逐次注销所有组件并重跑蓝图；连续操作发送属性通知，并定时更新简化预览。
  UObject::PostEditChangeProperty(Event);
  RestorePileRegistration();
  return;
 }
 CancelDeferredUpdate();
 Super::PostEditChangeProperty(Event);
 UpdatePileIfNeeded();
 RestorePileRegistration();
}

void ALxCargoPileActor::PostEditMove(bool bFinished)
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
 UpdatePileIfNeeded();
}

void ALxCargoPileActor::PostEditUndo()
{
 CancelDeferredUpdate();
 bGeometryCached=false;
 Super::PostEditUndo();
 UpdatePileIfNeeded();
}
#endif

void ALxCargoPileActor::SaveStaticMeshAsset()
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
 UpdatePileIfNeeded();
 RestorePileRegistration();
 UStaticMesh* Preview=PileMesh->GetStaticMesh();
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
 for (int32 Slot=0; Slot<4; ++Slot)
 { Asset->GetStaticMaterials()[Slot].MaterialInterface=PileMesh->GetMaterial(Slot); }
 FStaticMeshSourceModel& Source=Asset->AddSourceModel();
 Source.BuildSettings.bRecomputeNormals=false;
 Source.BuildSettings.bRecomputeTangents=false;
 Source.BuildSettings.bRemoveDegenerates=false;
 Source.BuildSettings.bGenerateLightmapUVs=true;
 Source.BuildSettings.SrcLightmapIndex=2;
 Source.BuildSettings.DstLightmapIndex=3;
 Asset->SetLightMapCoordinateIndex(3);
 // 完整资产构建会保留空多边形组；导出前移除关闭货物留下的空组，避免多余渲染分区。
 FMeshDescription ExportDescription(*Preview->GetMeshDescription(0));
 TArray<FPolygonGroupID> EmptyGroups;
 for (FPolygonGroupID Group : ExportDescription.PolygonGroups().GetElementIDs())
 { if (ExportDescription.GetNumPolygonGroupPolygons(Group)==0) { EmptyGroups.Add(Group); } }
 for (FPolygonGroupID Group : EmptyGroups) { ExportDescription.DeletePolygonGroup(Group); }
 FElementIDRemappings Remappings;
 ExportDescription.Compact(Remappings);
 Asset->CreateMeshDescription(0,ExportDescription);
 Asset->CommitMeshDescription(0);
 // 所有货物的简单形状共用唯一物理体，不复制三角面碰撞。
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
  GenerationDiagnostic=TEXT("保存货物堆静态网格失败，请检查目标目录是否可写。");
  return;
 }
 FAssetRegistryModule::AssetCreated(Asset);
 Modify();
 SavedStaticMesh=Asset;
 GenerationDiagnostic=TEXT("已保存货物堆静态网格：")+PackageName;
#else
 GenerationDiagnostic=TEXT("保存静态网格资产仅在编辑器中可用；运行时仍可生成货物堆静态网格。");
#endif
}
