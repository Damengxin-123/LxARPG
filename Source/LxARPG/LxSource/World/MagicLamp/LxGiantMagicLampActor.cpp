#include "LxGiantMagicLampActor.h"
#include "LxMagicLampGeometry.h"

namespace LxGiantMagicLamp
{
 /** 写入独立三角切面及连续的面内坐标，尖端不产生退化面。 */
 static void CrystalFace(LxMagicLamp::FSurface& Surface,const FVector& Tip,const FVector& A,const FVector& B)
 {
  const int32 Start=Surface.Vertices.Num();
  const FVector Normal=FVector::CrossProduct(B-Tip,A-Tip).GetSafeNormal();
  Surface.Vertices.Append({Tip,A,B});
  Surface.Normals.Append({Normal,Normal,Normal});
  Surface.UVs.Append({FVector2D(.5,1),FVector2D(0,0),FVector2D(1,0)});
  Surface.Indices.Append({Start,Start+1,Start+2});
 }

 /** 用重心坐标把菱形纹嵌入三角切面，细线轻微浮起以避免深度闪烁。 */
 static void FaceRune(LxMagicLamp::FSurface& Surface,const FVector& Tip,const FVector& A,const FVector& B,double Radius,bool bPetal)
 {
  const FVector Mid=(A+B)*.5;
  const FVector Normal=FVector::CrossProduct(B-Tip,A-Tip).GetSafeNormal();
  const FVector Offset=Normal*Radius*.007;
  const FVector Across=(B-A)*(bPetal?.115:.075);
  const FVector P=FMath::Lerp(Mid,Tip,.2)+Offset;
  const FVector Q=FMath::Lerp(Mid,Tip,.4)+Across+Offset;
  const FVector R=FMath::Lerp(Mid,Tip,.74)+Offset;
  const FVector S=FMath::Lerp(Mid,Tip,.4)-Across+Offset;
  const double Width=Radius*.004;
  Surface.Line(P,Q,Width); Surface.Line(Q,R,Width);
  Surface.Line(R,S,Width); Surface.Line(S,P,Width);
 }
}

ALxGiantMagicLampActor::ALxGiantMagicLampActor()
{
 BrightnessEV=18;
 LightRange=35000;
 LightColor=FLinearColor(.72f,.88f,1.f);
 bUseTemperature=false;
}

FVector ALxGiantMagicLampActor::GetLampDimensions() const
{ return FVector(CrystalRadius*2,CrystalRadius*2,CrystalHeight); }

void ALxGiantMagicLampActor::SanitizeGeometrySettings()
{
 CrystalHeight=LxMagicLamp::Safe(CrystalHeight,2400,200,10000);
 CrystalRadius=LxMagicLamp::Safe(CrystalRadius,450,50,3000);
 FacetCount=FMath::Clamp(FacetCount,4,16);
 RingCount=FMath::Clamp(RingCount,0,5);
 RingRadiusScale=LxMagicLamp::Safe(RingRadiusScale,1.55,1.3,2.5);
 RingSpacingRatio=LxMagicLamp::Safe(RingSpacingRatio,.16,.08,.18);
 RingTiltDegrees=LxMagicLamp::Safe(RingTiltDegrees,12,0,25);
 RingThicknessRatio=LxMagicLamp::Safe(RingThicknessRatio,.022,.008,.045);
}

uint32 ALxGiantMagicLampActor::GetGeometrySettingsHash() const
{
 uint32 Hash=HashCombine(GetTypeHash(FacetCount),GetTypeHash(RingCount));
 Hash=HashCombine(Hash,GetTypeHash(RingRadiusScale));
 Hash=HashCombine(Hash,GetTypeHash(RingSpacingRatio));
 Hash=HashCombine(Hash,GetTypeHash(RingTiltDegrees));
 Hash=HashCombine(Hash,GetTypeHash(RingThicknessRatio));
 return HashCombine(Hash,GetTypeHash(bCrystalRunes));
}

void ALxGiantMagicLampActor::BuildLampGeometry(LxMagicLamp::FSurface* Sections,const FVector& Dimensions) const
{
 const double Radius=Dimensions.X*.5,HalfHeight=Dimensions.Z*.5;
 for(int32 Face=0;Face<FacetCount;++Face)
 {
  const double Angle=2*PI*Face/FacetCount,Next=2*PI*(Face+1)/FacetCount;
  const FVector A(Radius*FMath::Cos(Angle),Radius*FMath::Sin(Angle),0);
  const FVector B(Radius*FMath::Cos(Next),Radius*FMath::Sin(Next),0);
  for(double Sign:{-1.0,1.0})
  {
   const FVector Tip(0,0,Sign*HalfHeight);
   const FVector& First=Sign>0?B:A;
   const FVector& Second=Sign>0?A:B;
   LxGiantMagicLamp::CrystalFace(Sections[2],Tip,First,Second);
   if(bCrystalRunes)
    LxGiantMagicLamp::FaceRune(Sections[1],Tip,First,Second,Radius,Ornament==ELxLampOrnament::Petal);
  }
 }
 // 尖端以简洁白石双锥托帽收束，避免尖塔式繁复装饰。
 for(double Sign:{-1.0,1.0})
 {
  const double CapZ=Sign*HalfHeight*.88,CapRadius=Radius*.145;
  for(int32 Face=0;Face<FacetCount;++Face)
  {
   const double Angle=2*PI*Face/FacetCount,Next=2*PI*(Face+1)/FacetCount;
   const FVector A(CapRadius*FMath::Cos(Angle),CapRadius*FMath::Sin(Angle),CapZ);
   const FVector B(CapRadius*FMath::Cos(Next),CapRadius*FMath::Sin(Next),CapZ);
   LxGiantMagicLamp::CrystalFace(Sections[0],FVector(0,0,Sign*HalfHeight*1.012),Sign>0?B:A,Sign>0?A:B);
   LxGiantMagicLamp::CrystalFace(Sections[0],FVector(0,0,Sign*HalfHeight*.82),Sign>0?A:B,Sign>0?B:A);
  }
  Sections[1].Ring(FVector(0,0,CapZ),FVector::ForwardVector,FVector::RightVector,CapRadius,Radius*.012);
 }
 for(int32 Ring=0;Ring<RingCount;++Ring)
 {
  const double Level=Ring-(RingCount-1)*.5;
  const double Z=Level*Dimensions.Z*RingSpacingRatio;
  const double Phase=Ring*PI*.618;
  // 扁宽晶体自动减小倾角，防止环线穿入晶面；正常塔灯仍使用完整倾角。
  const double TiltLimit=FMath::RadiansToDegrees(FMath::Atan(HalfHeight*.22/(Radius*RingRadiusScale)));
  const double Tilt=FMath::DegreesToRadians(FMath::Min(double(RingTiltDegrees),TiltLimit))*(Ring%2?-1:1);
  const FVector U(FMath::Cos(Phase),FMath::Sin(Phase),0);
  const FVector V(-FMath::Sin(Phase)*FMath::Cos(Tilt),FMath::Cos(Phase)*FMath::Cos(Tilt),FMath::Sin(Tilt));
  const double RingRadius=Radius*RingRadiusScale*(1-.08*FMath::Abs(Level));
  Sections[1].Ring(FVector(0,0,Z),U,V,RingRadius,Radius*RingThicknessRatio);
 }
}
