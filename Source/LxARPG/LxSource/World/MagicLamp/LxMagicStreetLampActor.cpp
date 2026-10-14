#include "LxMagicStreetLampActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxLampLightSettings.h"
#include "LxLampScheduleComponent.h"
#include "LxMagicLampGeometry.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UnrealType.h"

namespace LxStreetLamp
{
 using LxMagicLamp::FSurface;
 /** 写入闭合端面的单个三角形，法线遵循 UE 绕序。 */
 static void Triangle(FSurface& S,const FVector& A,const FVector& B,const FVector& C)
 {
  const int32 I=S.Vertices.Num();
  const FVector N=FVector::CrossProduct(C-A,B-A).GetSafeNormal();
  S.Vertices.Append({A,B,C}); S.Normals.Append({N,N,N});
  S.UVs.Append({FVector2D(.5,.5),FVector2D(0,0),FVector2D(1,0)});
  S.Indices.Append({I,I+1,I+2});
 }
 /** 生成闭合多棱台，用于倒角石座、金属杆和灯笼晶体。 */
 static void Frustum(FSurface& S,const FVector& Center,double Bottom,double Top,double Height,int32 Sides=8)
 {
  for(int32 I=0;I<Sides;++I)
  {
   const double A=2*PI*(I+.5)/Sides,B=2*PI*(I+1.5)/Sides;
   const FVector U(FMath::Cos(A),FMath::Sin(A),0),V(FMath::Cos(B),FMath::Sin(B),0);
   const FVector Low=Center-FVector(0,0,Height*.5),High=Center+FVector(0,0,Height*.5);
   S.Quad(Low+U*Bottom,High+U*Top,High+V*Top,Low+V*Bottom);
   Triangle(S,Low,Low+U*Bottom,Low+V*Bottom);
   Triangle(S,High,High+V*Top,High+U*Top);
  }
 }
 /** 可从远处辨认的双菱形符文，位于本地 XZ 平面。 */
 static void Rune(FSurface& S,const FVector& Center,double Size,double Width)
 {
  const FVector X(Size*.6,0,0),Z(0,0,Size);
  S.Line(Center+Z,Center+X,Width); S.Line(Center+X,Center-Z,Width);
  S.Line(Center-Z,Center-X,Width); S.Line(Center-X,Center+Z,Width);
  S.Line(Center+Z*.55,Center+X*.55,Width*.65); S.Line(Center+X*.55,Center-Z*.55,Width*.65);
  S.Line(Center-Z*.55,Center-X*.55,Width*.65); S.Line(Center-X*.55,Center+Z*.55,Width*.65);
  S.Line(Center-Z*1.25,Center-Z,Width); S.Line(Center+Z,Center+Z*1.25,Width);
 }
 /** 灯架三次贝塞尔卷草；连续曲线而非贴图形成明显轮廓。 */
 static void Curve(FSurface& S,const FVector& A,const FVector& B,const FVector& C,const FVector& D,double Width)
 {
  FVector Previous=A;
  for(int32 I=1;I<=24;++I)
  {
   const double T=double(I)/24,Q=1-T;
   const FVector P=A*(Q*Q*Q)+B*(3*Q*Q*T)+C*(3*Q*T*T)+D*(T*T*T);
   S.Line(Previous,P,Width); Previous=P;
  }
 }
 /** 灯杆两侧的大型卷草撑架，金色细边与深蓝金属主体叠合。 */
 static void Arm(FSurface* S,double Sign,double Reach,double Height)
 {
  S[1].Line(FVector(0,0,Height),FVector(Sign*Reach,0,Height),4.2);
  S[2].Line(FVector(0,-3.5,Height+1.5),FVector(Sign*Reach,-3.5,Height+1.5),1.05);
  S[1].Line(FVector(Sign*Reach,0,Height-3),FVector(Sign*Reach,0,Height+8),5.5);
  const FVector A(Sign*6,0,Height-68),B(Sign*Reach*.56,0,Height-68);
  const FVector C(Sign*Reach*.25,0,Height-3),D(Sign*Reach*.86,0,Height-3);
  Curve(S[1],A,B,C,D,3);
  Curve(S[2],A+FVector(0,-3,0),B+FVector(0,-3,0),C+FVector(0,-3,0),D+FVector(0,-3,0),1.3);
  // 向内收卷的金属花纹在侧面形成清楚的镂空图案。
  const FVector Center(Sign*Reach*.4,-.8,Height-29);
  FVector Prev=Center+FVector(Sign*17,0,0);
  for(int32 I=1;I<=40;++I)
  {
   const double T=double(I)/40,Angle=T*PI*2.1,R=17*(1-T*.83);
   const FVector P=Center+FVector(Sign*R*FMath::Cos(Angle),0,R*FMath::Sin(Angle));
   S[2].Line(Prev,P,1.7); Prev=P;
  }
  Rune(S[2],FVector(Sign*Reach*.72,-1,Height+14),9,1.25);
  S[1].Line(FVector(Sign*Reach*.72,0,Height+3),FVector(Sign*Reach*.72,0,Height+7),2);
 }
 /** 八面悬灯上端与金属吊杆相连，下方聚光灯沿各自一侧照明。 */
 static void Lantern(FSurface* S,int32 CrystalSlot,double X,double ArmHeight,double Drop)
 {
  const double Top=ArmHeight-Drop,Bottom=Top-64;
  S[1].Line(FVector(X,0,ArmHeight),FVector(X,0,Top+1),2.3);
  S[2].Ring(FVector(X,0,ArmHeight-7),FVector::ForwardVector,FVector::RightVector,3.1,1.0);
  Frustum(S[1],FVector(X,0,Top-6),27,9,12);
  Frustum(S[2],FVector(X,0,Top-12),28,28,3);
  Frustum(S[CrystalSlot],FVector(X,0,Top-29),22,23,30);
  Frustum(S[CrystalSlot],FVector(X,0,Top-51),15,22,14);
  Frustum(S[1],FVector(X,0,Bottom+3),13,18,8);
  Frustum(S[2],FVector(X,0,Bottom+8),18,18,3);
  Frustum(S[1],FVector(X,0,Bottom-4),4,11,6);
  for(int32 I=0;I<8;++I)
  {
   const double A=2*PI*(I+.5)/8;
   const FVector U(FMath::Cos(A),FMath::Sin(A),0);
   const FVector P=FVector(X,0,Top-13)+U*24,Q=FVector(X,0,Top-44)+U*23,R=FVector(X,0,Bottom+8)+U*17;
   S[1].Line(P,Q,1.5); S[1].Line(Q,R,1.5);
   S[2].Line(P+U*.7,Q+U*.7,.7); S[2].Line(Q+U*.7,R+U*.7,.7);
  }
  for(double Sign:{-1.,1.}) { Rune(S[2],FVector(X,Sign*22.5,Top-30),10,1.15); }
 }
 /** 创建石座与金属柱的两个简单凸包，避免逐三角形碰撞。 */
 static TArray<FVector> CollisionBox(const FVector& Center,const FVector& Half)
 {
  TArray<FVector> Points;
  for(double X:{-1.,1.}) for(double Y:{-1.,1.}) for(double Z:{-1.,1.})
  { Points.Add(Center+FVector(X*Half.X,Y*Half.Y,Z*Half.Z)); }
  return Points;
 }
 /** 独立同步一侧的真实光照与晶核色温，不触发网格重建。 */
 static void Lighting(ALxMagicStreetLampActor* Owner,USpotLightComponent* Light,
  TObjectPtr<UMaterialInstanceDynamic>& Instance,UMaterialInterface* Material,int32 Slot,
  const FVector& Position,bool bRoad,bool bEnabled,float BrightnessEV,float Temperature,float Range,const FLinearColor& Color,bool bUseTemperature,float OuterConeAngle)
 {
  Light->SetRelativeLocation(Position);
  // 保留原有照射方向；外锥角控制宽度，内锥角按原有比例保留柔和边缘。
  Light->SetRelativeRotation(FRotator(bRoad?-50:-60,bRoad?0:180,0));
  Light->SetInnerConeAngle(OuterConeAngle*(bRoad?27.f/40.f:20.f/30.f)); Light->SetOuterConeAngle(OuterConeAngle);
  Light->SetIntensityUnits(ELightUnits::EV);
  Light->bEditableWhenInherited=false;
  // 晶核外观仍按原有参考角度换算，调整照射宽度不会改变灯体自发光。
  const float Lumens=LxLampLightSettings::EVToLumens(BrightnessEV,FMath::Cos(FMath::DegreesToRadians(bRoad?40.f:30.f)));
  Light->SetIntensity(BrightnessEV); Light->SetVisibility(bEnabled);
  Light->SetLightColor(Color.GetClamped());
  Light->SetUseTemperature(bUseTemperature); Light->SetTemperature(Temperature);
  Light->SetAttenuationRadius(Range); Light->SetSourceRadius(5);
  Light->SetCastShadows(Owner->bLightShadows);
  if(!Instance || Instance->Parent!=Material)
  { Instance=Material?UMaterialInstanceDynamic::Create(Material,Owner):nullptr; }
  if(Instance)
  {
   Instance->SetVectorParameterValue(TEXT("魔力颜色"),Color.GetClamped()*(bUseTemperature?FLinearColor::MakeFromColorTemperature(Temperature):FLinearColor::White));
   Instance->SetScalarParameterValue(TEXT("发光强度"),bEnabled?FMath::Clamp(FMath::Sqrt(Lumens/1600.f)*1.2f,0.f,5.f):0);
  }
  Owner->StreetMesh->SetMaterial(Slot,Instance?static_cast<UMaterialInterface*>(Instance):Material);
 }
}

ALxMagicStreetLampActor::ALxMagicStreetLampActor()
{
 PrimaryActorTick.bCanEverTick=false;
 LightSchedule=CreateDefaultSubobject<ULxLampScheduleComponent>(TEXT("LightSchedule"));
 RoadBrightnessEV=LxLampLightSettings::LumensToEV(2800,FMath::Cos(FMath::DegreesToRadians(40.f)));
 SidewalkBrightnessEV=LxLampLightSettings::LumensToEV(1600,FMath::Cos(FMath::DegreesToRadians(30.f)));
 StreetMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("StreetMesh"));
 SetRootComponent(StreetMesh);
 StreetMesh->bUseComplexAsSimpleCollision=false;
 StreetMesh->SetCollisionProfileName(TEXT("BlockAll"));
 StreetMesh->SetGenerateOverlapEvents(false);
 RoadLight=CreateDefaultSubobject<ULxLampSpotLightComponent>(TEXT("RoadLight"));
 SidewalkLight=CreateDefaultSubobject<ULxLampSpotLightComponent>(TEXT("SidewalkLight"));
 for(USpotLightComponent* Light:{RoadLight.Get(),SidewalkLight.Get()})
 { Light->SetupAttachment(StreetMesh); Light->SetMobility(EComponentMobility::Movable); Light->bEditableWhenInherited=false; }
}
void ALxMagicStreetLampActor::Serialize(FArchive& Ar)
{
 Super::Serialize(Ar);
 Ar.UsingCustomVersion(LxLampLightSettings::VersionGuid);
}
void ALxMagicStreetLampActor::PostLoad()
{
 Super::PostLoad();
 if(GetLinkerCustomVersion(LxLampLightSettings::VersionGuid)<LxLampLightSettings::EVVersion)
 {
  const float RoadLumens=LxMagicLamp::Safe(RoadBrightness,2800,0,50000);
  const float SidewalkLumens=LxMagicLamp::Safe(SidewalkBrightness,1600,0,50000);
  RoadBrightnessEV=LxLampLightSettings::LumensToEV(RoadLumens,FMath::Cos(FMath::DegreesToRadians(40.f)));
  SidewalkBrightnessEV=LxLampLightSettings::LumensToEV(SidewalkLumens,FMath::Cos(FMath::DegreesToRadians(30.f)));
  bRoadLightEnabled=RoadLumens>0;
  bSidewalkLightEnabled=SidewalkLumens>0;
 }
}
void ALxMagicStreetLampActor::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); UpdateStreetLamp(); }
void ALxMagicStreetLampActor::BeginPlay()
{ Super::BeginPlay(); UpdateStreetLamp(); }
void ALxMagicStreetLampActor::RebuildStreetLamp()
{
 if(GetWorld() && GetWorld()->IsGameWorld()) { return; }
 CachedGeometry.Reset(); UpdateStreetLamp();
}
void ALxMagicStreetLampActor::UpdateStreetLamp()
{
 if(IsTemplate() || !StreetMesh || !RoadLight || !SidewalkLight) { return; }
 if(!LightSchedule->OnBrightnessChanged.IsBoundToObject(this))
  LightSchedule->OnBrightnessChanged.AddUObject(this,&ThisClass::ApplyScheduledBrightness);
 LightSchedule->Configure(bAutoEnable,AutoOnHour,AutoOffHour,LightFadeSeconds);
 PoleLength=LxMagicLamp::Safe(PoleLength,320,180,800);
 RoadArmLength=LxMagicLamp::Safe(RoadArmLength,120,70,220);
 SidewalkArmLength=LxMagicLamp::Safe(SidewalkArmLength,85,65,180);
 SuspensionLength=LxMagicLamp::Safe(SuspensionLength,30,20,65);
 RoadOuterConeAngle=LxMagicLamp::Safe(RoadOuterConeAngle,40,1,80);
 SidewalkOuterConeAngle=LxMagicLamp::Safe(SidewalkOuterConeAngle,30,1,80);
 RoadBrightnessEV=LxMagicLamp::Safe(RoadBrightnessEV,LxLampLightSettings::LumensToEV(2800,FMath::Cos(FMath::DegreesToRadians(40.f))),-20,20);
 SidewalkBrightnessEV=LxMagicLamp::Safe(SidewalkBrightnessEV,LxLampLightSettings::LumensToEV(1600,FMath::Cos(FMath::DegreesToRadians(30.f))),-20,20);
 RoadTemperature=LxMagicLamp::Safe(RoadTemperature,4200,1700,12000);
 SidewalkTemperature=LxMagicLamp::Safe(SidewalkTemperature,3400,1700,12000);
 RoadLightRange=LxMagicLamp::Safe(RoadLightRange,1400,300,5000);
 SidewalkLightRange=LxMagicLamp::Safe(SidewalkLightRange,1000,300,5000);
 SetActorScale3D(FVector::OneVector);
 const bool bDouble=Layout==ELxStreetLampLayout::Double;
 const double H=60+PoleLength;
 const TArray<double> Geometry={PoleLength,RoadArmLength,bDouble?SidewalkArmLength:0, SuspensionLength,bDouble?1.:0.};
 if(CachedGeometry!=Geometry || StreetMesh->GetNumSections()!=5)
 {
  LxMagicLamp::FSurface S[5];
  // 分层整石底座：宽脚、收窄座身和小挑檐，不采用现代混凝土外观。
  LxStreetLamp::Frustum(S[0],FVector(0,0,5),43,41,10,4);
  LxStreetLamp::Frustum(S[0],FVector(0,0,30),34,32,40,4);
  LxStreetLamp::Frustum(S[0],FVector(0,0,55),35,37,10,4);
  LxStreetLamp::Frustum(S[1],FVector(0,0,63),12,10,6,16);
  LxStreetLamp::Frustum(S[1],FVector(0,0,(60+H)*.5),7,5.5,PoleLength,24);
  for(double Z:{68.,90.,60+PoleLength*.48,H-75,H-5})
  { LxStreetLamp::Frustum(S[2],FVector(0,0,Z),8.5,8.5,3,16); }
  for(double Sign:{-1.,1.})
  {
   LxStreetLamp::Rune(S[2],FVector(0,Sign*24.1,30),13,1.35);
   LxStreetLamp::Rune(S[2],FVector(0,Sign*8.3,60+PoleLength*.48),17,1.2);
   S[2].Line(FVector(Sign*5.2,-4.5,95),FVector(Sign*4.3,-4.1,H-78),.85);
  }
  LxStreetLamp::Arm(S,1,RoadArmLength,H);
  LxStreetLamp::Lantern(S,3,RoadArmLength,H,SuspensionLength);
  if(bDouble)
  {
   LxStreetLamp::Arm(S,-1,SidewalkArmLength,H-35);
   LxStreetLamp::Lantern(S,4,-SidewalkArmLength,H-35,SuspensionLength);
  }
  S[1].Line(FVector(0,0,H-4),FVector(0,0,H+45),3.5);
  LxStreetLamp::Rune(S[2],FVector(0,-3.8,H+23),18,2);
  for(int32 I=0;I<5;++I) { S[I].Upload(StreetMesh,I); }
  TArray<TArray<FVector>> Hulls;
  Hulls.Add(LxStreetLamp::CollisionBox(FVector(0,0,30),FVector(30.5,30.5,30)));
  Hulls.Add(LxStreetLamp::CollisionBox(FVector(0,0,60+PoleLength*.5),FVector(7,7,PoleLength*.5)));
  StreetMesh->SetCollisionConvexMeshes(Hulls);
  CachedGeometry=Geometry; ++GeometryBuildCount;
 }
 StreetMesh->SetMaterial(0,StoneMaterial); StreetMesh->SetMaterial(1,MetalMaterial); StreetMesh->SetMaterial(2,OrnamentMaterial);
 const double LightZ=H-SuspensionLength-73;
 LxStreetLamp::Lighting(this,RoadLight,RoadCrystal,CrystalMaterial,3,FVector(RoadArmLength,0,LightZ),true,bRoadLightEnabled,RoadBrightnessEV,RoadTemperature,RoadLightRange,RoadLightColor,bRoadUseTemperature,RoadOuterConeAngle);
 LxStreetLamp::Lighting(this,SidewalkLight,SidewalkCrystal,CrystalMaterial,4,FVector(-SidewalkArmLength,0,LightZ-35),false,bDouble && bSidewalkLightEnabled,SidewalkBrightnessEV,SidewalkTemperature,SidewalkLightRange,SidewalkLightColor,bSidewalkUseTemperature,SidewalkOuterConeAngle);
 ApplyScheduledBrightness(LightSchedule->GetBrightnessAlpha());
 if(GetWorld() && !StreetMesh->IsRegistered()) { RegisterAllComponents(); }
}
void ALxMagicStreetLampActor::ApplyScheduledBrightness(float Alpha)
{
 const float RoadLumens=LxLampLightSettings::EVToLumens(RoadBrightnessEV,FMath::Cos(FMath::DegreesToRadians(40.f)));
 const float SidewalkLumens=LxLampLightSettings::EVToLumens(SidewalkBrightnessEV,FMath::Cos(FMath::DegreesToRadians(30.f)));
 LxLampLightSettings::ApplyFade(RoadLight,RoadCrystal,bRoadLightEnabled,RoadBrightnessEV,FMath::Clamp(FMath::Sqrt(RoadLumens/1600.f)*1.2f,0.f,5.f),Alpha);
 LxLampLightSettings::ApplyFade(SidewalkLight,SidewalkCrystal,bSidewalkLightEnabled && Layout==ELxStreetLampLayout::Double,SidewalkBrightnessEV,FMath::Clamp(FMath::Sqrt(SidewalkLumens/1600.f)*1.2f,0.f,5.f),Alpha);
}
#if WITH_EDITOR
void ALxMagicStreetLampActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{ Super::PostEditChangeProperty(Event); UpdateStreetLamp(); }
void ALxMagicStreetLampActor::PostEditUndo()
{ Super::PostEditUndo(); CachedGeometry.Reset(); UpdateStreetLamp(); }
#endif
