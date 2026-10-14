#pragma once
#include "CoreMinimal.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "LxLampLightSettings.generated.h"

class UMaterialInstanceDynamic;

/** 灯具内部点光源；编辑入口统一放在灯具 Actor 上。 */
UCLASS(NotBlueprintable, HideCategories=(Light, LightFunction, LightProfiles, LightShafts, DistanceFieldShadows, RayTracing, Rendering, Transform, TransformCommon, Performance, Mobility, LOD, Activation, Tags, AssetUserData, ComponentTick, ComponentReplication, Replication, Variable, Navigation, Cooking, Physics, "Components|Activation"), meta=(DisplayName="灯具内部点光源"))
class LXARPG_API ULxLampPointLightComponent : public UPointLightComponent
{
 GENERATED_BODY()
};

/** 路灯内部聚光源；隐藏与灯具参数重复的组件设置。 */
UCLASS(NotBlueprintable, HideCategories=(Light, LightFunction, LightProfiles, LightShafts, DistanceFieldShadows, RayTracing, Rendering, Transform, TransformCommon, Performance, Mobility, LOD, Activation, Tags, AssetUserData, ComponentTick, ComponentReplication, Replication, Variable, Navigation, Cooking, Physics, "Components|Activation"), meta=(DisplayName="灯具内部聚光源"))
class LXARPG_API ULxLampSpotLightComponent : public USpotLightComponent
{
 GENERATED_BODY()
};

/** 灯具 EV 存档版本与等效光通量换算，遵循引擎的原生 EV 定义。 */
namespace LxLampLightSettings
{
 /** 按线性倍率衰减 EV 光源与晶核；零倍率隐藏光源，避免零 EV 仍然发光。 */
 void ApplyFade(ULocalLightComponent* Light, UMaterialInstanceDynamic* Crystal, bool bEnabled, float EV, float FullGlow, float Alpha);
 /** 标识新增 EV 参数的资产版本，旧流明字段仅用于加载迁移。 */
 extern const FGuid VersionGuid;
 /** 当前版本首次使用 EV。 */
 constexpr int32 EVVersion = 1;
 /** 将流明转为 EV；聚光灯需传入外锥角余弦，点光源默认覆盖整球。 */
 float LumensToEV(float Lumens, float CosHalfConeAngle = -1.0f);
 /** 将 EV 还原为等效流明，保持晶核原有自发光表现。 */
 float EVToLumens(float EV, float CosHalfConeAngle = -1.0f);
}
