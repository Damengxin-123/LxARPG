#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxSunActor.generated.h"

class UDirectionalLightComponent;
class USceneComponent;
class UPostProcessComponent;
class UCurveFloat;
class UCurveLinearColor;
class ULxWorldInfoManager;
class ASkyLight;

/** 订阅世界时间的太阳，自动更新方向、照度和颜色。 */
UCLASS(BlueprintType, Blueprintable, AutoExpandCategories=("太阳|光照"), meta=(DisplayName="太阳"))
class LXARPG_API ALxSunActor : public AActor
{
	GENERATED_BODY()
public:
	/** 创建可移动的大气太阳光，不重复创建场景天空组件。 */
	ALxSunActor();
	/** 兼容已有太阳实例，恢复全局光源的常驻加载规则。 */
	virtual void PostLoad() override;
#if WITH_EDITOR
	/** 在详情面板拖动光照参数时立即更新当前实例。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	/** 撤销光照调整后恢复对应的预览效果。 */
	virtual void PostEditUndo() override;
	/** 全局太阳不能由距离决定加载，避免角色进入区域后失去日照。 */
	virtual bool CanChangeIsSpatiallyLoadedFlag() const override { return false; }
	/** 太阳没有局部影响范围，不参与世界分区空间边界计算。 */
	virtual void GetStreamingBounds(FBox& OutRuntimeBounds, FBox& OutEditorBounds) const override;
#endif
	/** 编辑器使用预览时间，游戏世界优先读取当前世界时间。 */
	virtual void OnConstruction(const FTransform& Transform) override;
	/** 开始运行时绑定全局时间并立即应用存档中的时刻。 */
	virtual void BeginPlay() override;
	/** 关卡卸载或销毁时解除时间订阅。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** 从世界管理器重新读取时间；编辑器中使用预览小时。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="太阳|时间", meta=(DisplayName="刷新太阳"))
	void RefreshSun();
	/** 修改测试倍率并立即应用到世界时钟；蓝图赋值也通过此接口生效。 */
	UFUNCTION(BlueprintSetter, Category="太阳|测试", meta=(DisplayName="设置测试时间流速倍率"))
	void SetTestTimeSpeedMultiplier(double InMultiplier);
	/** 根据指定小时更新太阳，仅改变光照，不改变全局时间。 */
	UFUNCTION(BlueprintCallable, Category="太阳|时间", meta=(DisplayName="应用太阳时刻"))
	void ApplyTimeOfDay(double InHours);
	/** 光照刷新后通知蓝图，可用于联动云层、雾和场景表现。 */
	UFUNCTION(BlueprintImplementableEvent, Category="太阳|时间", meta=(DisplayName="太阳更新后"))
	void OnSunUpdated(double Hours, float DaylightFactor);
	/** 太阳定位根节点，光照方向使用世界坐标独立计算。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="太阳|组件", meta=(DisplayName="太阳根节点"))
	TObjectPtr<USceneComponent> SceneRoot;
	/** 作为天空大气主太阳的可移动方向光。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="太阳|组件", meta=(DisplayName="太阳光"))
	TObjectPtr<UDirectionalLightComponent> SunLight;
	/** 自动联动捕获场景的天空光，使环境补光随昼夜更新；不修改指定立方体贴图的天空光。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|环境光", meta=(DisplayName="联动实时天空光"))
	bool bSynchronizeSkyLight = true;
	/** 可选场景天空光；未指定时，仅在场景存在唯一天空光时自动联动。 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="太阳|环境光", meta=(DisplayName="联动天空光", EditCondition="bSynchronizeSkyLight"))
	TObjectPtr<ASkyLight> SkyLightActor;
	/** 为全场景提供低优先级曝光基线，局部后处理仍可覆盖。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="太阳|组件", meta=(DisplayName="稳定曝光"))
	TObjectPtr<UPostProcessComponent> Exposure;
	/** 限制视线变化引起的自动曝光，防止暗处被持续提亮成灰色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曝光", meta=(DisplayName="启用稳定曝光"))
	bool bStabilizeExposure = true;
	/** 使用物理日光照度时的曝光基准；数值越大，画面越暗，不随太阳落山自动提亮。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曝光", meta=(DisplayName="基准曝光EV100", ClampMin="-10", ClampMax="20", EditCondition="bStabilizeExposure"))
	float ExposureEV100 = 11.0f;
	/** 基准上下允许适应的曝光档数；零表示固定曝光，彻底消除进出阴影时的亮度抽动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曝光", meta=(DisplayName="曝光适应幅度", ClampMin="0", ClampMax="3", EditCondition="bStabilizeExposure"))
	float ExposureAdaptationRange = 0.0f;
	/** 局部曝光暗部对比度；一表示不额外提亮阴影，较小值可柔化明暗差。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曝光", meta=(DisplayName="暗部对比度", ClampMin="0", ClampMax="1", EditCondition="bStabilizeExposure"))
	float ShadowContrast = 1.0f;
	/** 未运行游戏时使用的小时，修改后立即预览。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|时间", meta=(DisplayName="预览小时", ClampMin="0", ClampMax="24"))
	double PreviewHour = 8.0;
	/** 运行游戏时乘在基础时间流速上：一不变、大于一加速、小于一减速、零暂停；不改存档基础流速。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintSetter=SetTestTimeSpeedMultiplier, Category="太阳|测试", meta=(DisplayName="时间流速倍率（测试）", ClampMin="0", UIMin="0", UIMax="100", ToolTip="默认1保持原流速，10为十倍速度，0.5为半速，0暂停昼夜。运行游戏时生效，同步影响太阳和自动灯具；不修改存档基础流速。多个太阳以最后刷新者为准，不叠乘。"))
	double TestTimeSpeedMultiplier = 1.0;
	/** 日出方位角；零度表示太阳位于世界正 X 方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|轨迹", meta=(DisplayName="日出方向", Units="deg"))
	float SunriseAzimuth = 0.0f;
	/** 正午太阳高于地平线的最大角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|轨迹", meta=(DisplayName="正午太阳高度", ClampMin="1", ClampMax="90", Units="deg"))
	float NoonElevation = 65.0f;
	/** 每个太阳实例的正午照度上限；默认昼夜曲线在十二点达到此值，夜间仍归零。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="正午最大亮度", ClampMin="0", Units="lx", DisplayPriority="1"))
	float NoonIntensity = 50000.0f;
	/** 叠加在昼夜颜色或颜色曲线上的染色；白色保持原有光色，不影响太阳时刻。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="光照颜色", HideAlphaChannel, DisplayPriority="2"))
	FLinearColor LightTint = FLinearColor::White;
	/** 可选冷暖调节，与光照颜色相乘；默认关闭以保留已有场景的光色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="启用色温", DisplayPriority="3"))
	bool bUseTemperature = false;
	/** 色温仅调整冷暖，不改变昼夜亮度；低值偏暖，高值偏冷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="色温（开尔文）", ClampMin="1700", ClampMax="12000", EditCondition="bUseTemperature", DisplayPriority="4"))
	float Temperature = 6500.0f;
	/** 低空太阳的暖色，夜间太阳照度固定归零。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="晨昏颜色"))
	FLinearColor HorizonColor = FLinearColor(1.0f, 0.55f, 0.3f);
	/** 高空太阳的清透暖白色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|光照", meta=(DisplayName="日间颜色"))
	FLinearColor DayColor = FLinearColor(1.0f, 0.96f, 0.9f);
	/** 可选曲线：横轴为零到二十四小时，纵轴为零到一的正午照度倍率；超过一按一处理，地平线以下仍关闭太阳。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曲线", meta=(DisplayName="照度倍率曲线"))
	TObjectPtr<UCurveFloat> IntensityCurve;
	/** 可选颜色曲线：横轴为零到二十四小时，覆盖默认晨昏颜色插值。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="太阳|曲线", meta=(DisplayName="太阳颜色曲线"))
	TObjectPtr<UCurveLinearColor> ColorCurve;
private:
	/** 只更新曝光相关覆盖项，保留场景的其它后处理效果。 */
	void RefreshExposure();
	/** 在刷新或开始运行时配置已有天空光，避免每次推进时间都重新捕获。 */
	void RefreshSkyLight();
	/** 统一接收自然推进、读档和手动调时事件。 */
	UFUNCTION(Category="太阳|时间", meta=(DisplayName="响应世界时间变化"))
	void HandleTimeChanged(double TotalGameSeconds);
	/** 当前绑定的世界信息管理器，不阻止游戏实例清理。 */
	TWeakObjectPtr<ULxWorldInfoManager> WorldInfoManager;
};
