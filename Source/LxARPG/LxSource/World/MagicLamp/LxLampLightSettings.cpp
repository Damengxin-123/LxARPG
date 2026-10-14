#include "LxLampLightSettings.h"
#include "Serialization/CustomVersion.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace LxLampLightSettings
{
 void ApplyFade(ULocalLightComponent* Light, UMaterialInstanceDynamic* Crystal, bool bEnabled, float EV, float FullGlow, float Alpha)
 {
  const float Amount=bEnabled?FMath::Clamp(Alpha,0.f,1.f):0.f;
  if(Light)
  {
   Light->SetIntensity(Amount>0?EV+FMath::Log2(Amount):EV);
   Light->SetVisibility(Amount>0);
  }
  if(Crystal) Crystal->SetScalarParameterValue(TEXT("发光强度"),FullGlow*Amount);
 }
 const FGuid VersionGuid(0xAF21EE53, 0x617E4D69, 0x967FDDA0, 0xCAE44F17);
 /** 保存灯具时写入 EV 版本，避免重复换算已保存的新参数。 */
 static FCustomVersionRegistration Registration(VersionGuid, EVVersion, TEXT("LxLampLightEV"));
 float LumensToEV(float Lumens, float CosHalfConeAngle)
 {
  const float Factor = ULocalLightComponent::GetUnitsConversionFactor(ELightUnits::Lumens, ELightUnits::EV, CosHalfConeAngle);
  return FMath::Log2(FMath::Max(Lumens * Factor, FMath::Pow(2.0f, -20.0f)));
 }
 float EVToLumens(float EV, float CosHalfConeAngle)
 {
  return FMath::Pow(2.0f, EV) * ULocalLightComponent::GetUnitsConversionFactor(ELightUnits::EV, ELightUnits::Lumens, CosHalfConeAngle);
 }
}
