#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "UObject/Object.h"
#include "LxCharacterResourceRecoveryModule.generated.h"

class ULxCharacterAttributeComponent;

/** 根据资源自身的每秒恢复值持续结算恢复，不参与词条重算或即时效果包分发。 */
UCLASS(BlueprintType, DisplayName="角色资源恢复模块")
class LXARPG_API ULxCharacterResourceRecoveryModule : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定所属属性组件与生命周期事件；实际恢复由开始接口显式启用。 */
	void InitializeModule(ULxCharacterAttributeComponent* InOwnerComponent);

	/** 停止定时器并解除所属组件的事件绑定。 */
	void ShutdownModule();

	/** 启用自然恢复；仅权威端的游戏世界能够运行，死亡时等待复活。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|资源恢复", DisplayName="开始资源自然恢复")
	void StartRecovery();

	/** 结算到当前游戏时间后停用自然恢复，并清除不足零点零一的累积余量。 */
	UFUNCTION(BlueprintCallable, Category="角色|属性|资源恢复", DisplayName="停止资源自然恢复")
	void StopRecovery();

	/** 判断恢复定时器当前是否正在运行。 */
	UFUNCTION(BlueprintPure, Category="角色|属性|资源恢复", DisplayName="资源自然恢复是否运行")
	bool IsRecoveryRunning() const { return bRecoveryRunning; }

	/** 在外部属性修改前按旧恢复速率结算，避免将新速率追溯应用到已经过去的时间。 */
	void SettleRecoveryToCurrentTime();

	/** 属性修改后同步速率与有效余量；仍启用的恢复在外部治疗救回角色后自动续接。 */
	void RefreshRecoveryRates();

	/** 判断当前是否正在提交恢复，供属性组件避免回调重入结算。 */
	bool IsApplyingRecovery() const { return bApplyingRecovery; }

	/** 使用所属属性组件的游戏世界。 */
	virtual UWorld* GetWorld() const override;

private:
	/** 判断所属组件是否位于权威端游戏世界，且角色存活、标准生命未归零。 */
	bool CanRunRecovery() const;

	/** 从当前游戏时间重新开始计时，并安装固定检查间隔的恢复定时器。 */
	void ResumeRecoveryTimer();

	/** 清除定时器和小数余量，不改变玩家或游戏逻辑指定的启用状态。 */
	void SuspendRecoveryTimer();

	/** 定时器到达后按实际经过的游戏时间结算。 */
	void HandleRecoveryTimer();

	/** 死亡时暂停并清空余量，复活后按原启用状态从当前时间重新开始。 */
	void HandleLifecycleStateChanged(bool bInAlive, FGameplayTag InLifecycleStateTag);

	/** 提供资源数据并统一提交恢复量的所属属性组件。 */
	UPROPERTY(Transient, VisibleAnywhere, Category="角色|属性|资源恢复", DisplayName="所属属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> OwnerComponent;

	/** 当前资源快照中有效的每秒恢复速率，按资源标签自动索引。 */
	TMap<FGameplayTag, double> RecoveryRates;

	/** 尚未达到零点零一资源单位的双精度余量，不会因定时检查而丢失。 */
	TMap<FGameplayTag, double> RecoveryRemainders;

	/** 以游戏世界时间运行的恢复定时器。 */
	FTimerHandle RecoveryTimerHandle;

	/** 最近一次结算所处的游戏时间，单位为秒。 */
	double LastRecoveryGameTimeSeconds = 0.0;

	/** 是否已经显式启用恢复；死亡暂停不会改变该意图。 */
	bool bRecoveryRequested = false;

	/** 恢复定时器是否处于运行状态。 */
	bool bRecoveryRunning = false;

	/** 正在结算与提交恢复量，防止属性变更广播重新进入本次结算。 */
	bool bApplyingRecovery = false;
};
