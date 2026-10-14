#include "LxMagicLampActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxLampLightSettings.h"
#include "LxLampScheduleComponent.h"
#include "LxMagicLampGeometry.h"
#include "ProceduralMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UnrealType.h"

namespace LxMagicLamp
{
 /** 方灯采用白石边框、双面晶板和金色符文。 */
 static void BoxLamp(FSurface* Sections,const FVector& Size,bool bPetal)
 {
  const double X=Size.X/2,Y=Size.Y/2,Z=Size.Z/2,W=FMath::Min(X,Z)*.105;
  Sections[2].Box(FVector::ZeroVector,FVector(Size.X-W*2,Size.Y*.65,Size.Z-W*2));
  for(double Sign:{-1.0,1.0})
  {
   Sections[0].Box(FVector(Sign*(X-W/2),0,0),FVector(W,Size.Y,Size.Z));
   Sections[0].Box(FVector(0,0,Sign*(Z-W/2)),FVector(Size.X-W*2,Size.Y,W));
   const double Front=Sign*(Y+.025);
   const FVector P[]={FVector(-X+W/2,Front,-Z+W/2),FVector(X-W/2,Front,-Z+W/2),
    FVector(X-W/2,Front,Z-W/2),FVector(-X+W/2,Front,Z-W/2)};
   for(int32 I=0;I<4;++I) { Sections[1].Line(P[I],P[(I+1)%4],W*.13); }
   const double RuneY=Sign*(Y*.65+W*.13),R=FMath::Min(X,Z)*.44;
   const FVector Center(0,RuneY,0);
   Sections[1].Ring(Center,FVector::ForwardVector,FVector::UpVector,R,W*.12);
   const int32 Count=bPetal?6:4;
   for(int32 I=0;I<Count;++I)
   {
    const double Angle=2*PI*I/Count;
    const FVector U(FMath::Cos(Angle),0,FMath::Sin(Angle)),V(-U.Z,0,U.X);
    const FVector A=Center+U*R*.3,B=Center+U*R*.72+V*R*.23,C=Center+U*R*1.3,D=Center+U*R*.72-V*R*.23;
    Sections[1].Line(A,B,W*.11); Sections[1].Line(B,C,W*.11);
    Sections[1].Line(C,D,W*.11); Sections[1].Line(D,A,W*.11);
   }
   for(double SX:{-1.0,1.0}) for(double SZ:{-1.0,1.0})
   {
    const FVector C(SX*(X-W*1.65),RuneY,SZ*(Z-W*1.65));
    const FVector A=C+FVector(0,0,W*.65),B=C+FVector(W*.35,0,0),D=C-FVector(W*.35,0,0),E=C-FVector(0,0,W*.65);
    Sections[1].Line(A,B,W*.08); Sections[1].Line(B,E,W*.08);
    Sections[1].Line(E,D,W*.08); Sections[1].Line(D,A,W*.08);
   }
  }
 }
 /** 球灯采用晶核和环形骨架，花冠变体沿经线展开花瓣。 */
 static void SphereLamp(FSurface* Sections,double Radius,bool bPetal)
 {
  Sections[2].Sphere(Radius*.84);
  Sections[1].Ring(FVector::ZeroVector,FVector::ForwardVector,FVector::RightVector,Radius*.975,Radius*.025);
  for(double Sign:{-1.0,1.0})
  {
   Sections[0].Ring(FVector(0,0,Sign*Radius*.9),FVector::ForwardVector,FVector::RightVector,Radius*.25,Radius*.065);
   Sections[1].Ring(FVector(0,0,Sign*Radius*.9),FVector::ForwardVector,FVector::RightVector,Radius*.3,Radius*.025);
  }
  for(int32 I=0;I<6;++I)
  {
   const double Angle=2*PI*I/6;
   const FVector U(FMath::Cos(Angle),FMath::Sin(Angle),0),V(-U.Y,U.X,0);
   for(int32 J=0;J<32;++J)
   {
    const double A=PI*(.09+.82*J/32),B=PI*(.09+.82*(J+1)/32);
    const double TwistA=bPetal?.22*FMath::Sin(A*2):0,TwistB=bPetal?.22*FMath::Sin(B*2):0;
    const FVector P=(U*FMath::Cos(TwistA)+V*FMath::Sin(TwistA))*FMath::Sin(A)+FVector::UpVector*FMath::Cos(A);
    const FVector Q=(U*FMath::Cos(TwistB)+V*FMath::Sin(TwistB))*FMath::Sin(B)+FVector::UpVector*FMath::Cos(B);
    Sections[1].Line(P*Radius*.975,Q*Radius*.975,Radius*.02);
   }
   const FVector C=U*Radius*.96;
   const FVector A=C+FVector::UpVector*Radius*.19,B=C+V*Radius*.1,D=C-V*Radius*.1,E=C-FVector::UpVector*Radius*.19;
   Sections[1].Line(A,B,Radius*.013); Sections[1].Line(B,E,Radius*.013);
   Sections[1].Line(E,D,Radius*.013); Sections[1].Line(D,A,Radius*.013);
  }
 }
}
ALxMagicLampActor::ALxMagicLampActor()
{
 PrimaryActorTick.bCanEverTick=false;
 LightSchedule=CreateDefaultSubobject<ULxLampScheduleComponent>(TEXT("LightSchedule"));
 BrightnessEV=LxLampLightSettings::LumensToEV(650);
 LampMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("LampMesh"));
 SetRootComponent(LampMesh);
 LampMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 LampMesh->SetCastShadow(false);
 LampMesh->SetCanEverAffectNavigation(false);
 LampLight=CreateDefaultSubobject<ULxLampPointLightComponent>(TEXT("LampLight"));
 LampLight->SetupAttachment(LampMesh);
 LampLight->SetMobility(EComponentMobility::Movable);
 LampLight->SetIntensityUnits(ELightUnits::EV);
 LampLight->bEditableWhenInherited=false;
}
void ALxMagicLampActor::Serialize(FArchive& Ar)
{
 Super::Serialize(Ar);
 Ar.UsingCustomVersion(LxLampLightSettings::VersionGuid);
}
void ALxMagicLampActor::PostLoad()
{
 Super::PostLoad();
 if(GetLinkerCustomVersion(LxLampLightSettings::VersionGuid)<LxLampLightSettings::EVVersion)
 {
  const float LegacyLumens=LxMagicLamp::Safe(Brightness,650,0,50000);
  BrightnessEV=LxLampLightSettings::LumensToEV(LegacyLumens);
  bLightEnabled=LegacyLumens>0;
 }
}
void ALxMagicLampActor::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); UpdateLamp(); }
void ALxMagicLampActor::BeginPlay()
{ Super::BeginPlay(); UpdateLamp(); }
FVector ALxMagicLampActor::GetLampDimensions() const { return FVector(60,12,90); }
bool ALxMagicLampActor::IsSpherical() const { return false; }
void ALxMagicLampActor::SanitizeGeometrySettings() {}
uint32 ALxMagicLampActor::GetGeometrySettingsHash() const { return 0; }
void ALxMagicLampActor::BuildLampGeometry(LxMagicLamp::FSurface* Sections,const FVector& Dimensions) const
{
 if(IsSpherical()) { LxMagicLamp::SphereLamp(Sections,Dimensions.X*.5,Ornament==ELxLampOrnament::Petal); }
 else { LxMagicLamp::BoxLamp(Sections,Dimensions,Ornament==ELxLampOrnament::Petal); }
}
FVector ALxBoxMagicLampActor::GetLampDimensions() const
{
 return FVector(LxMagicLamp::Safe(FMath::Abs(SizeHandle.X),30,8,250)*2,
  LxMagicLamp::Safe(Thickness,12,4,100),LxMagicLamp::Safe(FMath::Abs(SizeHandle.Z),45,8,250)*2);
}
FVector ALxSphereMagicLampActor::GetLampDimensions() const
{ return FVector(LxMagicLamp::Safe(Radius,32,8,150)*2); }
bool ALxSphereMagicLampActor::IsSpherical() const { return true; }
void ALxMagicLampActor::RebuildLamp()
{
 if(GetWorld() && GetWorld()->IsGameWorld()) { return; }
 CachedDimensions=FVector::ZeroVector; UpdateLamp();
}
void ALxMagicLampActor::UpdateLamp()
{
 if(IsTemplate() || !LampMesh || !LampLight) { return; }
 if(!LightSchedule->OnBrightnessChanged.IsBoundToObject(this))
  LightSchedule->OnBrightnessChanged.AddUObject(this,&ThisClass::ApplyScheduledBrightness);
 LightSchedule->Configure(bAutoEnable,AutoOnHour,AutoOffHour,LightFadeSeconds);
 SanitizeGeometrySettings();
 const FVector Dimensions=GetLampDimensions();
 const uint32 GeometrySettingsHash=GetGeometrySettingsHash();
 // 尺寸只从专用参数读取，防止球灯被非均匀演员缩放变成椭球。
 SetActorScale3D(FVector::OneVector);
 if(ALxBoxMagicLampActor* Box=Cast<ALxBoxMagicLampActor>(this))
 { Box->SizeHandle=FVector(Dimensions.X*.5,0,Dimensions.Z*.5); Box->Thickness=Dimensions.Y; }
 if(ALxSphereMagicLampActor* Sphere=Cast<ALxSphereMagicLampActor>(this)) { Sphere->Radius=Dimensions.X*.5; }
 if(CachedDimensions!=Dimensions || CachedOrnament!=Ornament || CachedGeometrySettingsHash!=GeometrySettingsHash || LampMesh->GetNumSections()!=3)
 {
  LxMagicLamp::FSurface Sections[3];
  BuildLampGeometry(Sections,Dimensions);
  for(int32 I=0;I<3;++I) { Sections[I].Upload(LampMesh,I); }
  CachedDimensions=Dimensions; CachedOrnament=Ornament; CachedGeometrySettingsHash=GeometrySettingsHash; ++GeometryBuildCount;
 }
 BrightnessEV=LxMagicLamp::Safe(BrightnessEV,LxLampLightSettings::LumensToEV(650),-20,20);
 const float Lumens=bLightEnabled?LxLampLightSettings::EVToLumens(BrightnessEV):0;
 Temperature=LxMagicLamp::Safe(Temperature,4200,1700,12000);
 LightRange=LxMagicLamp::Safe(LightRange,500,50,100000);
 LampLight->SetIntensityUnits(ELightUnits::EV);
 LampLight->SetIntensity(BrightnessEV); LampLight->SetVisibility(bLightEnabled);
 LampLight->bEditableWhenInherited=false;
 LampLight->SetLightColor(LightColor.GetClamped());
 LampLight->SetUseTemperature(bUseTemperature); LampLight->SetTemperature(Temperature);
 LampLight->SetAttenuationRadius(LightRange); LampLight->SetCastShadows(bLightShadows);
 LampLight->SetSourceRadius(FMath::Min3(Dimensions.X,Dimensions.Y,Dimensions.Z)*.2);
 LampMesh->SetMaterial(0,HousingMaterial); LampMesh->SetMaterial(1,OrnamentMaterial);
 if(!CrystalInstance || CrystalInstance->Parent!=CrystalMaterial)
 { CrystalInstance=CrystalMaterial?UMaterialInstanceDynamic::Create(CrystalMaterial,this):nullptr; }
 if(CrystalInstance)
 {
  CrystalInstance->SetVectorParameterValue(TEXT("魔力颜色"),LightColor.GetClamped()*(bUseTemperature?FLinearColor::MakeFromColorTemperature(Temperature):FLinearColor::White));
  CrystalInstance->SetScalarParameterValue(TEXT("发光强度"),Lumens<=0?0:FMath::Clamp(FMath::Sqrt(Lumens/650.f)*1.5f,0.f,6.f));
 }
 LampMesh->SetMaterial(2,CrystalInstance?static_cast<UMaterialInterface*>(CrystalInstance):CrystalMaterial.Get());
 ApplyScheduledBrightness(LightSchedule->GetBrightnessAlpha());
 if(GetWorld() && !LampMesh->IsRegistered()) { RegisterAllComponents(); }
}
void ALxMagicLampActor::ApplyScheduledBrightness(float Alpha)
{
 const float Lumens=LxLampLightSettings::EVToLumens(BrightnessEV);
 const float Glow=FMath::Clamp(FMath::Sqrt(Lumens/650.f)*1.5f,0.f,6.f);
 LxLampLightSettings::ApplyFade(LampLight,CrystalInstance,bLightEnabled,BrightnessEV,Glow,Alpha);
}
#if WITH_EDITOR
void ALxMagicLampActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{ Super::PostEditChangeProperty(Event); UpdateLamp(); }
void ALxMagicLampActor::PostEditUndo()
{ Super::PostEditUndo(); CachedDimensions=FVector::ZeroVector; UpdateLamp(); }
#endif
