#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxMagicLampActor.generated.h"
class UProceduralMeshComponent;
class UPointLightComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ULxLampScheduleComponent;
namespace LxMagicLamp { struct FSurface; }

/** 魔力灯的实体装饰语言。 */
UENUM(BlueprintType, meta=(DisplayName="魔灯纹饰"))
enum class ELxLampOrnament : uint8
{
 Star UMETA(DisplayName="星环符文"),
 Petal UMETA(DisplayName="花冠纹饰")
};

/** 自动生成灯体与真实光源；开发时调节，不使用运行时 Tick。 */
UCLASS(Abstract, BlueprintType, Blueprintable, AutoExpandCategories=("魔灯|光照"), meta=(DisplayName="自动魔力灯"))
class LXARPG_API ALxMagicLampActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建合并灯体和唯一点光源。 */
 ALxMagicLampActor();
 /** 写入灯具光照单位的兼容版本。 */
 virtual void Serialize(FArchive& Ar) override;
 /** 将旧场景的流明设置等效迁移到 EV。 */
 virtual void PostLoad() override;
 /** 放置和编辑时同步形状、光源和晶核。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 加载后恢复动态材质与保存的光照设置。 */
 virtual void BeginPlay() override;
#if WITH_EDITOR
 /** 支持详情面板与三维尺寸手柄预览。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 撤销或重做后恢复灯体。 */
 virtual void PostEditUndo() override;
#endif
 /** 仅编辑世界可执行的刷新，不提供游戏内蓝图调光接口。 */
 UFUNCTION(CallInEditor, Category="魔灯|生成", meta=(DisplayName="重新生成魔灯"))
 void RebuildLamp();
 /** 灯壳、纹饰与魔力晶核共用的网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="魔灯|组件", meta=(DisplayName="魔灯灯体"))
 TObjectPtr<UProceduralMeshComponent> LampMesh;
 /** 真正照亮场景的点光源，以父级魔灯参数为准。 */
 UPROPERTY(BlueprintReadOnly, Category="魔灯|组件", meta=(DisplayName="内部光源（由灯具设置）"))
 TObjectPtr<UPointLightComponent> LampLight;
 /** 克制的星环或花瓣装饰。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|外观", meta=(DisplayName="装饰纹样"))
 ELxLampOrnament Ornament = ELxLampOrnament::Star;
 /** 旧版流明字段，仅用于读取已有场景；详情面板不再提供编辑入口。 */
 UPROPERTY(BlueprintReadOnly, meta=(DisplayName="旧版流明", Category="魔灯|兼容"))
 float Brightness = 650;
 /** 独立开关照明与晶核发光；EV 为零仍有光，熄灯使用此开关。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="启用照明", DisplayPriority="0"))
 bool bLightEnabled = true;
 /** 当前实例的 EV 光强；每增加一档光强翻倍，负值表示更微弱的光。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="亮度（EV）", ClampMin="-20", ClampMax="20", UIMin="-5", UIMax="15", Delta="0.1", EditCondition="bLightEnabled", DisplayPriority="1"))
 float BrightnessEV;
 /** 当前实例的光源与晶核颜色；白色保持原有色温效果，其他颜色用于彩色光源。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="光照颜色", HideAlphaChannel, DisplayPriority="2"))
 FLinearColor LightColor = FLinearColor::White;
 /** 是否额外使用色温调节冷暖；关闭后直接采用光照颜色。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="启用色温", DisplayPriority="3"))
 bool bUseTemperature = true;
 /** 光源与晶核共同使用的色温；低值偏暖，高值偏冷。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="色温（开尔文）", ClampMin="1700", ClampMax="12000", UIMin="2700", UIMax="8000", EditCondition="bUseTemperature", DisplayPriority="4"))
 float Temperature = 4200;
 /** 独立于灯体大小的照明影响范围。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="照明范围", ClampMin="50", ClampMax="100000", UIMax="50000", Units="cm"))
 float LightRange = 500;
 /** 按游戏世界时间自动控制照明；关闭后使用原有照明开关。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|自动照明", meta=(DisplayName="自动启用"))
 bool bAutoEnable = true;
 /** 每天开始渐亮的小时，支持小数小时；与关闭时间相同表示全天开启。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|自动照明", meta=(DisplayName="自动开启时间（小时）", ClampMin="0", ClampMax="24", EditCondition="bAutoEnable"))
 float AutoOnHour = 18;
 /** 每天开始渐灭的小时；开启时间大于关闭时间时自动跨午夜。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|自动照明", meta=(DisplayName="自动关闭时间（小时）", ClampMin="0", ClampMax="24", EditCondition="bAutoEnable"))
 float AutoOffHour = 6;
 /** 一次渐亮或渐灭的运行秒数，不随世界昼夜流速加速；光源和晶核同步。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|自动照明", meta=(DisplayName="亮度渐变时长", ClampMin="0.1", ClampMax="60", Units="s"))
 float LightFadeSeconds = 2;
 /** 默认关闭动态阴影以便城镇成组摆放。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|光照", meta=(DisplayName="投射场景阴影"))
 bool bLightShadows = false;
 /** 浅灰白哑光灯壳材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|材质", meta=(DisplayName="灯壳材质"))
 TObjectPtr<UMaterialInterface> HousingMaterial;
 /** 金色镶边与实体符文材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|材质", meta=(DisplayName="纹饰材质"))
 TObjectPtr<UMaterialInterface> OrnamentMaterial;
 /** 支持“魔力颜色”和“发光强度”参数的晶核材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|材质", meta=(DisplayName="晶核材质"))
 TObjectPtr<UMaterialInterface> CrystalMaterial;
 /** 实际几何更新次数，调光和移动不应增加。 */
 UPROPERTY(VisibleAnywhere, Transient, Category="魔灯|统计", meta=(DisplayName="几何生成次数"))
 int32 GeometryBuildCount = 0;
protected:
 /** 返回宽、厚、高；球灯返回直径。 */
 virtual FVector GetLampDimensions() const;
 /** 派生灯型选择球形生成路径。 */
 virtual bool IsSpherical() const;
 /** 清理派生灯型的额外几何参数。 */
 virtual void SanitizeGeometrySettings();
 /** 返回额外几何参数的缓存签名，调光不参与几何更新。 */
 virtual uint32 GetGeometrySettingsHash() const;
 /** 生成灯壳、纹饰与晶核三个材质分区。 */
 virtual void BuildLampGeometry(LxMagicLamp::FSurface* Sections, const FVector& Dimensions) const;
 /** 同步几何缓存与光照参数。 */
 void UpdateLamp();
private:
 /** 隐藏的共用时间调度组件，仅过渡期间开启逐帧更新。 */
 UPROPERTY()
 TObjectPtr<ULxLampScheduleComponent> LightSchedule;
 /** 只同步真实光强与晶核自发光，避免每帧重建灯体。 */
 void ApplyScheduledBrightness(float Alpha);
 /** 上次生成的有效尺寸。 */
 FVector CachedDimensions = FVector::ZeroVector;
 /** 上次生成的纹饰。 */
 ELxLampOrnament CachedOrnament = ELxLampOrnament::Star;
 /** 上次生成时派生灯型的参数签名。 */
 uint32 CachedGeometrySettingsHash = 0;
 /** 当前晶核动态材质，加载后重建。 */
 UPROPERTY(Transient)
 TObjectPtr<UMaterialInstanceDynamic> CrystalInstance;
};

/** 方形魔力灯：宽高使用三维手柄，厚度单独调节。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动方形魔灯"))
class LXARPG_API ALxBoxMagicLampActor : public ALxMagicLampActor
{
 GENERATED_BODY()
public:
 /** 从中心拖动 X/Z 改变半宽和半高；Y 自动归零。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|尺寸", meta=(DisplayName="宽高手柄（半宽半高）", MakeEditWidget=true, Units="cm"))
 FVector SizeHandle = FVector(30,0,45);
 /** 本地 Y 轴的总厚度；旋转演员可用于墙面或天花板。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|尺寸", meta=(DisplayName="灯体厚度", ClampMin="4", ClampMax="100", Units="cm"))
 float Thickness = 12;
protected:
 /** 清理非法输入并返回完整尺寸。 */
 virtual FVector GetLampDimensions() const override;
};

/** 球形魔力灯：唯一形状尺寸是半径，纹饰跟随等比变化。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动球形魔灯"))
class LXARPG_API ALxSphereMagicLampActor : public ALxMagicLampActor
{
 GENERATED_BODY()
public:
 /** 包含纹饰外轮廓的半径，不提供宽高或厚度参数。 */
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="魔灯|尺寸", meta=(DisplayName="灯体半径", ClampMin="8", ClampMax="150", Units="cm"))
 float Radius = 32;
protected:
 /** 返回包含纹饰的直径。 */
 virtual FVector GetLampDimensions() const override;
 /** 选择球形生成路径。 */
 virtual bool IsSpherical() const override;
};
