#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxMagicStreetLampActor.generated.h"
class UProceduralMeshComponent;
class USpotLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ULxLampScheduleComponent;

/** 路灯照明方向，本地正 X 对应马路，负 X 对应人行道。 */
UENUM(BlueprintType, meta=(DisplayName="路灯类型"))
enum class ELxStreetLampLayout : uint8
{
 Single UMETA(DisplayName="单侧悬灯"),
 Double UMETA(DisplayName="双侧悬灯（马路与人行道）")
};

/** 石座、金属杆与卷草灯架构成的自动悬吊魔力路灯。 */
UCLASS(BlueprintType, Blueprintable, AutoExpandCategories=("路灯|马路照明", "路灯|人行道照明"), meta=(DisplayName="自动魔力路灯"))
class LXARPG_API ALxMagicStreetLampActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一灯体网格和两个独立的定向照明组件。 */
 ALxMagicStreetLampActor();
 /** 写入灯具光照单位的兼容版本。 */
 virtual void Serialize(FArchive& Ar) override;
 /** 分别迁移旧版马路侧与人行道侧流明设置。 */
 virtual void PostLoad() override;
 /** 编辑尺寸时重建，调光或整体移动时复用几何。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 加载关卡时恢复晶核动态材质与预设光照。 */
 virtual void BeginPlay() override;
#if WITH_EDITOR
 /** 编辑器参数变化后立即更新可见灯体和照明。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 撤销重做时恢复对应几何。 */
 virtual void PostEditUndo() override;
#endif
 /** 只在编辑世界中重新生成，不开放游戏内调节接口。 */
 UFUNCTION(CallInEditor, Category="路灯|生成", meta=(DisplayName="重新生成路灯"))
 void RebuildStreetLamp();
 /** 合并石座、灯杆、灯架、悬吊灯体的渲染组件。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="路灯|组件", meta=(DisplayName="路灯灯体"))
 TObjectPtr<UProceduralMeshComponent> StreetMesh;
 /** 沿本地正 X 方向向下照明的马路侧聚光灯。 */
 UPROPERTY(BlueprintReadOnly, Category="路灯|组件", meta=(DisplayName="马路侧光源（由灯具设置）"))
 TObjectPtr<USpotLightComponent> RoadLight;
 /** 沿本地负 X 方向向下照明，单侧预设中完全关闭。 */
 UPROPERTY(BlueprintReadOnly, Category="路灯|组件", meta=(DisplayName="人行道侧光源（由灯具设置）"))
 TObjectPtr<USpotLightComponent> SidewalkLight;
 /** 单臂或一长一短的双臂悬灯。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|结构", meta=(DisplayName="路灯类型"))
 ELxStreetLampLayout Layout = ELxStreetLampLayout::Single;
 /** 石座顶面到马路灯臂轴线的金属杆长度，底座固定高 60 厘米。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|尺寸", meta=(DisplayName="灯杆长度", ClampMin="180", ClampMax="800", Units="cm"))
 float PoleLength = 320;
 /** 灯杆中心到马路悬灯中心的水平距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|尺寸", meta=(DisplayName="马路侧灯臂长度", ClampMin="70", ClampMax="220", Units="cm"))
 float RoadArmLength = 120;
 /** 灯杆中心到人行道悬灯中心的水平距离，支臂比马路侧低 35 厘米。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|尺寸", meta=(DisplayName="人行道侧灯臂长度", ClampMin="65", ClampMax="180", Units="cm", EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 float SidewalkArmLength = 85;
 /** 横臂到灯笼冠顶的竖直金属吊杆长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|尺寸", meta=(DisplayName="吊杆长度", ClampMin="20", ClampMax="65", Units="cm"))
 float SuspensionLength = 30;
 /** 旧版马路侧流明字段，仅用于加载迁移。 */
 UPROPERTY(BlueprintReadOnly, meta=(DisplayName="旧版马路侧流明", Category="路灯|兼容"))
 float RoadBrightness = 2800;
 /** 独立开关马路侧光源与晶核自发光。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧启用照明"))
 bool bRoadLightEnabled = true;
 /** 马路侧 EV 光强，每增加一档光强翻倍。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧亮度（EV）", ClampMin="-20", ClampMax="20", UIMin="-5", UIMax="15", Delta="0.1", EditCondition="bRoadLightEnabled"))
 float RoadBrightnessEV;
 /** 当前实例马路侧光源与晶核的颜色，与另一侧独立。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧光照颜色", HideAlphaChannel))
 FLinearColor RoadLightColor = FLinearColor::White;
 /** 是否额外调整马路侧光源冷暖，关闭后直接使用指定颜色。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧启用色温"))
 bool bRoadUseTemperature = true;
 /** 马路侧光源和晶核共同使用的色温。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧色温（开尔文）", ClampMin="1700", ClampMax="12000", EditCondition="bRoadUseTemperature"))
 float RoadTemperature = 4200;
 /** 马路侧聚光灯的最大作用距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧照明范围", ClampMin="300", ClampMax="5000", Units="cm"))
 float RoadLightRange = 1400;
 /** 马路侧光锥中心到边缘的半角；角度越大照射越宽，边缘柔和过渡随角度自动调整。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|马路照明", meta=(DisplayName="马路侧照射角度（半角）", ClampMin="1", ClampMax="80", Units="deg"))
 float RoadOuterConeAngle = 40;
 /** 旧版人行道侧流明字段，仅用于加载迁移。 */
 UPROPERTY(BlueprintReadOnly, meta=(DisplayName="旧版人行道侧流明", Category="路灯|兼容"))
 float SidewalkBrightness = 1600;
 /** 独立开关人行道侧光源与晶核自发光。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧启用照明", EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 bool bSidewalkLightEnabled = true;
 /** 人行道侧 EV 光强，每增加一档光强翻倍。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧亮度（EV）", ClampMin="-20", ClampMax="20", UIMin="-5", UIMax="15", Delta="0.1", EditCondition="Layout == ELxStreetLampLayout::Double && bSidewalkLightEnabled"))
 float SidewalkBrightnessEV;
 /** 当前实例人行道侧光源与晶核的颜色，与另一侧独立。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧光照颜色", HideAlphaChannel, EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 FLinearColor SidewalkLightColor = FLinearColor::White;
 /** 是否额外调整人行道侧光源冷暖，关闭后直接使用指定颜色。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧启用色温", EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 bool bSidewalkUseTemperature = true;
 /** 人行道可使用更温暖的光色，保持马路侧原有设置。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧色温（开尔文）", ClampMin="1700", ClampMax="12000", EditCondition="Layout == ELxStreetLampLayout::Double && bSidewalkUseTemperature", EditConditionHides))
 float SidewalkTemperature = 3400;
 /** 人行道侧聚光灯的最大作用距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧照明范围", ClampMin="300", ClampMax="5000", Units="cm", EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 float SidewalkLightRange = 1000;
 /** 人行道侧光锥中心到边缘的半角；角度越大照射越宽，边缘柔和过渡随角度自动调整。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|人行道照明", meta=(DisplayName="人行道侧照射角度（半角）", ClampMin="1", ClampMax="80", Units="deg", EditCondition="Layout == ELxStreetLampLayout::Double", EditConditionHides))
 float SidewalkOuterConeAngle = 30;
 /** 按游戏世界时间自动控制照明；关闭后使用原有照明开关。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|自动照明", meta=(DisplayName="自动启用"))
 bool bAutoEnable = true;
 /** 每天开始渐亮的小时，支持小数小时；与关闭时间相同表示全天开启。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|自动照明", meta=(DisplayName="自动开启时间（小时）", ClampMin="0", ClampMax="24", EditCondition="bAutoEnable"))
 float AutoOnHour = 18;
 /** 每天开始渐灭的小时；开启时间大于关闭时间时自动跨午夜。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|自动照明", meta=(DisplayName="自动关闭时间（小时）", ClampMin="0", ClampMax="24", EditCondition="bAutoEnable"))
 float AutoOffHour = 6;
 /** 一次渐亮或渐灭的运行秒数，不随世界昼夜流速加速；光源和晶核同步。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|自动照明", meta=(DisplayName="亮度渐变时长", ClampMin="0.1", ClampMax="60", Units="s"))
 float LightFadeSeconds = 2;
 /** 需要时为两侧光源开启场景阴影。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|光照", meta=(DisplayName="投射场景阴影"))
 bool bLightShadows = false;
 /** 灰白石座使用的石材。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|材质", meta=(DisplayName="底座石材"))
 TObjectPtr<UMaterialInterface> StoneMaterial;
 /** 灯杆、灯架、吊杆与灯罩使用的深海蓝金属。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|材质", meta=(DisplayName="灯杆灯架金属"))
 TObjectPtr<UMaterialInterface> MetalMaterial;
 /** 卷草、菱形符文和环带使用的浅金色材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|材质", meta=(DisplayName="浮雕纹饰材质"))
 TObjectPtr<UMaterialInterface> OrnamentMaterial;
 /** 支持“魔力颜色”和“发光强度”参数的魔纹晶核。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="路灯|材质", meta=(DisplayName="悬灯晶核材质"))
 TObjectPtr<UMaterialInterface> CrystalMaterial;
 /** 记录尺寸更新次数，调光和移动不增加此值。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="路灯|统计", meta=(DisplayName="几何生成次数"))
 int32 GeometryBuildCount = 0;
private:
 /** 隐藏的共用时间调度组件，仅过渡期间开启逐帧更新。 */
 UPROPERTY()
 TObjectPtr<ULxLampScheduleComponent> LightSchedule;
 /** 只同步真实光强与晶核自发光，避免每帧重建灯体。 */
 void ApplyScheduledBrightness(float Alpha);
 /** 更新两侧灯体、简单碰撞、晶核材质和真实光源。 */
 void UpdateStreetLamp();
 /** 上次构建的结构参数。 */
 TArray<double> CachedGeometry;
 /** 马路侧独立晶核材质。 */
 UPROPERTY(Transient, meta=(DisplayName="马路晶核实例"))
 TObjectPtr<UMaterialInstanceDynamic> RoadCrystal;
 /** 人行道侧独立晶核材质。 */
 UPROPERTY(Transient, meta=(DisplayName="人行道晶核实例"))
 TObjectPtr<UMaterialInstanceDynamic> SidewalkCrystal;
};
