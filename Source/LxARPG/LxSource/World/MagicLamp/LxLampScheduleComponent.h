#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxLampScheduleComponent.generated.h"
class ULxWorldInfoManager;

/** 实际线性光强的渐变倍率，同时驱动光源与晶核。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FLxLampBrightnessChanged, float);

/** 灯具共用的昼夜调度；只在渐亮或渐灭期间逐帧更新。 */
UCLASS(NotBlueprintable, meta=(DisplayName="灯具内部自动照明"))
class LXARPG_API ULxLampScheduleComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 /** 默认关闭逐帧更新，边界变化时才启用。 */
 ULxLampScheduleComponent();
 /** 订阅游戏世界时间，并从熄灭状态渐亮。 */
 virtual void BeginPlay() override;
 /** 移除世界时间订阅，避免卸载关卡后仍接收回调。 */
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 /** 使用运行秒推进平滑过渡，不受昼夜时间流速影响。 */
 virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
 /** 同步灯具设置；编辑器保持正常亮灯便于布置，运行时根据时段控制。 */
 void Configure(bool bAutomatic, float StartHour, float EndHour, float DurationSeconds);
 /** 绑定同一世界的时间管理器；无管理器时退回手动常亮，仍保留渐亮。 */
 void BindWorldTime(ULxWorldInfoManager* Manager);
 /** 返回当前真实光强倍率，零为熄灭，一为设定的 EV 光强。 */
 float GetBrightnessAlpha() const { return CurrentAlpha; }
 /** 判断小时是否在区间内；支持跨午夜，起止相同表示全天。 */
 static bool IsWithinHours(double Hour, float StartHour, float EndHour);
 /** 光强变化事件，仅更新渲染参数，不重建灯体。 */
 FLxLampBrightnessChanged OnBrightnessChanged;
private:
 /** 接收自然推进、剧情跳时和读档通知。 */
 UFUNCTION(Category="灯具|自动照明", meta=(DisplayName="响应灯具世界时间"))
 void HandleTimeChanged(double TotalGameSeconds);
 /** 仅在目标开关变化时启动渐变，避免每次时间通知重置进度。 */
 void RefreshTarget(double Hour);
 /** 已绑定的时间来源，不延长游戏实例生命周期。 */
 TWeakObjectPtr<ULxWorldInfoManager> WorldTime;
 /** 是否已进入运行调度，编辑器构造期间不执行时段判断。 */
 bool bRuntimeActive = false;
 /** 当前灯具是否使用时段控制。 */
 bool bAutoEnable = true;
 /** 开始与结束小时，均规范为零到二十四以内。 */
 float OnHour = 18, OffHour = 6;
 /** 完成一次过渡需要的运行秒数。 */
 float FadeSeconds = 2;
 /** 当前倍率与本次过渡起点；中途反转时保持亮度连续。 */
 float CurrentAlpha = 1, StartAlpha = 1;
 /** 当前目标，初始负值用于强制第一次刷新。 */
 float TargetAlpha = -1;
 /** 当前过渡累计的运行秒数。 */
 float ElapsedSeconds = 0;
};
