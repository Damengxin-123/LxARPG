#pragma once
#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "LxWorldTimeSaveData.h"
#include "LxWorldInfoManager.generated.h"

class ULxSaveManager;
/** 读档、调时或自然推进后通知场景和蓝图更新。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLxWorldTimeChanged, double, TotalGameSeconds);

/** 游戏实例持有的世界信息管理器，目前负责跨地图时间及存档同步。 */
UCLASS(BlueprintType, meta=(DisplayName="游戏世界信息管理器"))
class LXARPG_API ULxWorldInfoManager : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
public:
	/** 构造期间禁用计时，初始化完成后才注册更新。 */
	ULxWorldInfoManager();
	/** 切换存档来源并恢复时间；后续成功读档自动同步。 */
	void Initialize(ULxSaveManager* InSaveManager);
	/** 解除存档事件并停止更新。 */
	void Deinitialize();
	/** 垃圾回收前解除计时和事件注册。 */
	virtual void BeginDestroy() override;
	/** 返回所属游戏实例当前世界，避免持有旧关卡。 */
	virtual UWorld* GetWorld() const override;
	/** 推进历法，遵循世界暂停和时间膨胀。 */
	virtual void Tick(float DeltaTime) override;
	/** 只在已开始的可写游戏会话中计时。 */
	virtual bool IsTickable() const override;
	/** 将更新限定到所属世界，隔离多个编辑器运行实例。 */
	virtual UWorld* GetTickableGameObjectWorld() const override;
	/** 提供性能统计标识。 */
	virtual TStatId GetStatId() const override;
	/** 获取当前世界时间快照。 */
	UFUNCTION(BlueprintPure, Category="世界信息|时间", meta=(DisplayName="获取世界时间"))
	FLxWorldTimeSaveData GetWorldTime() const { return WorldTime; }
	/** 获取当天小数小时，范围为零到二十四且不包含二十四。 */
	UFUNCTION(BlueprintPure, Category="世界信息|时间", meta=(DisplayName="获取当前小时"))
	double GetTimeOfDayHours() const;
	/** 获取从一开始的游戏天数。 */
	UFUNCTION(BlueprintPure, Category="世界信息|时间", meta=(DisplayName="获取当前天数"))
	int64 GetDayNumber() const;
	/** 设置累计时间；只读会话或无效数值返回失败。 */
	UFUNCTION(BlueprintCallable, Category="世界信息|时间", meta=(DisplayName="设置累计游戏秒数"))
	bool SetTotalGameSeconds(double InSeconds);
	/** 设置当天小时并保留天数，用于调试或剧情跳时。 */
	UFUNCTION(BlueprintCallable, Category="世界信息|时间", meta=(DisplayName="设置当天时间"))
	bool SetTimeOfDayHours(double InHours);
	/** 设置时间流速，零表示停止自然推进。 */
	UFUNCTION(BlueprintCallable, Category="世界信息|时间", meta=(DisplayName="设置时间流速"))
	bool SetTimeScale(double InScale);
	/** 单独暂停历法；该临时状态不写入存档。 */
	UFUNCTION(BlueprintCallable, Category="世界信息|时间", meta=(DisplayName="设置世界时间暂停"))
	void SetTimePaused(bool bPaused) { bTimePaused = bPaused; }
	/** 查询历法是否处于手动暂停状态。 */
	UFUNCTION(BlueprintPure, Category="世界信息|时间", meta=(DisplayName="世界时间是否暂停"))
	bool IsTimePaused() const { return bTimePaused; }
	/** 时间变化事件，太阳和界面可订阅同一来源。 */
	UPROPERTY(BlueprintAssignable, Category="世界信息|时间", meta=(DisplayName="世界时间变化"))
	FLxWorldTimeChanged OnTimeChanged;
	/** 设置场景提供的临时测试倍率；最后配置的来源生效，不叠乘且不写入存档。 */
	void SetTestTimeSpeedMultiplier(UObject* Source, double InMultiplier);
	/** 仅撤销指定来源自己的倍率，防止旧太阳卸载时清除新太阳设置。 */
	void ClearTestTimeSpeedMultiplier(const UObject* Source);
	/** 来源失效或离开当前世界后自动恢复正常流速。 */
	double GetTestTimeSpeedMultiplier() const;
private:
	/** 从成功载入的存档恢复时间，不反向修改只读菜单存档。 */
	void RestoreTime();
	/** 保存前提交最新时间，覆盖直接保存和退出保存入口。 */
	void CaptureTime();
	/** 判断是否允许修改当前会话时间。 */
	bool CanModifyTime() const;
	/** 当前世界的唯一运行时间状态。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="世界信息|时间", meta=(DisplayName="运行时间"))
	FLxWorldTimeSaveData WorldTime;
	/** 存档管理器由游戏实例持有，不延长旧会话生命周期。 */
	TWeakObjectPtr<ULxSaveManager> SaveManager;
	/** 临时暂停标志，切档时清除。 */
	bool bTimePaused = false;
	/** 临时倍率的场景来源，弱引用避免跨地图延长太阳生命周期。 */
	TWeakObjectPtr<UObject> TestTimeSpeedSource;
	/** 仅乘在运行计时上，存档始终保留原始基础流速。 */
	double TestTimeSpeedMultiplier = 1.0;
};
