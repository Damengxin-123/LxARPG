#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterEntryPackage.h"
#include "LxCharacterBuffComponent.generated.h"

class ALxBaseCharacter;
struct FLxEffectSourceContext;

/** Buff 添加、移除等单个 Buff 变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffLogicChanged, ULxBuff*, BuffLogic);

/** Buff 周期生效事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffPeriodActivated, ULxBuff*, BuffLogic);

/** 运行时 Buff 缓存信息。 */
USTRUCT()
struct FLxBuffRuntimeInfo
{
	GENERATED_BODY()

	/** 运行时 Buff 对象。 */
	UPROPERTY()
	TObjectPtr<ULxBuff> BuffLogic = nullptr;

	/** Buff item tag ID. */
	UPROPERTY()
	FGameplayTag BuffIDTag;

	/** Buff 效果比例，由创建 Buff 词条传入，预留给后续词条结算使用。 */
	float EffectProportion = 1.f;

	/** 剩余持续时间，小于 0 表示永久 Buff。 */
	float RemainingDuration = -1.f;

	/** 按词条来源记录 Buff 引用次数，避免卸下装备时误删其他来源的同 ID Buff。 */
	UPROPERTY()
	TMap<ELxCharacterEntrySource, int32> SourceReferenceCounts;

	/** 按具体效果来源记录 Buff 引用次数，用于光环、依附等持续技能按单元实例撤回。 */
	UPROPERTY()
	TMap<FName, int32> SourceKeyReferenceCounts;
};

/** Buff 网络显示快照，用于把服务端 Buff 图标和剩余时间同步到客户端 UI。 */
USTRUCT(BlueprintType, DisplayName="Buff网络显示快照")
struct FLxReplicatedBuffRuntimeInfo
{
	GENERATED_BODY()

	/** 需要在客户端显示的 Buff 标签 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff|网络", DisplayName="Buff标签ID", meta=(Categories="物品"))
	FGameplayTag BuffIDTag;

	/** Buff 剩余持续时间，小于 0 表示永久 Buff。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Buff|网络", DisplayName="剩余持续时间")
	float RemainingDuration = -1.f;
};

/**
 * 角色 Buff 组件。
 *
 * 当前组件使用新的物品对象体系创建 Buff：Buff 本身是 ULxBuff，
 * 创建 Buff 的来源由通用效果包传入，并在组件内按来源记录引用次数。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色Buff模块")
class LXARPG_API ULxCharacterBuffModule : public ULxCharacterContentModuleBase
{
	GENERATED_BODY()

public:
	/** 创建 Buff 组件，并关闭 Tick。 */
	ULxCharacterBuffModule();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * 添加指定 ID 的 Buff。
	 *
	 * @param InBuffIDTag Buff 物品标签 ID。
	 * @param InEffectProportion Buff 效果比例。
	 * @param InDurationOverride 持续时间覆盖，小于 0 表示永久。
	 * @return 创建或刷新后的 Buff 对象。
	 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="添加Buff")
	ULxBuff* AddBuff(FGameplayTag InBuffIDTag, float InEffectProportion = 1.f, float InDurationOverride = -1.f, ELxCharacterEntrySource InEntrySource = ELxCharacterEntrySource::Other);

	/** 使用完整效果来源上下文添加 Buff，持续技能可通过相同来源上下文精确撤回。 */
	ULxBuff* AddBuffFromSourceContext(FGameplayTag InBuffIDTag, float InEffectProportion, float InDurationOverride, const FLxEffectSourceContext& InSourceContext);

	/** 移除指定 Buff 对象。 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="移除Buff")
	bool RemoveBuff(ULxBuff* InBuffLogic);

	/** 移除指定来源对 Buff 的引用；该 Buff 没有任何来源引用后才会真正移除。 */
	int32 RemoveBuffSourceReferenceByTagID(FGameplayTag InBuffIDTag, ELxCharacterEntrySource InEntrySource, int32 InReferenceCount = 1);

	/** 移除指定效果来源上下文持有的所有 Buff 引用，常用于持续光环离开范围时撤回目标效果。 */
	int32 RemoveBuffSourceReferencesBySourceContext(const FLxEffectSourceContext& InSourceContext);

	/** 清空所有 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="清空所有Buff")
	void ClearBuffs();

	/** 获取当前所有有效 Buff。 */
	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取全部Buff")
	void GetActiveBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取指定运行时 Buff 的效果比例；Buff 不存在时返回默认比例 1。 */
	float GetBuffEffectProportion(ULxBuff* InBuffLogic) const;

	/** Buff 添加事件。 */
	UPROPERTY(BlueprintAssignable, DisplayName="Buff添加事件")
	FOnBuffLogicChanged OnBuffAdded;

	/** Buff 移除事件。 */
	UPROPERTY(BlueprintAssignable, DisplayName="Buff移除事件")
	FOnBuffLogicChanged OnBuffRemoved;

	/** Buff 周期生效事件。 */
	UPROPERTY(BlueprintAssignable, DisplayName="Buff周期生效事件")
	FOnBuffPeriodActivated OnBuffPeriodActivated;

private:
	/** 初始化 Buff 模块。 */
	virtual void OnModuleInitialize() override;

	/** 模块关闭时清理全部 Buff 和计时器。 */
	virtual void OnModuleShutdown() override;

	/** 根据 Buff 对象查找运行时缓存。 */
	FLxBuffRuntimeInfo* FindRuntimeInfo(ULxBuff* InBuffLogic);

	/** 根据 Buff 对象查找运行时缓存。 */
	const FLxBuffRuntimeInfo* FindRuntimeInfo(ULxBuff* InBuffLogic) const;

	/** 根据 Buff 标签 ID 查找第一个运行时缓存。 */
	FLxBuffRuntimeInfo* FindFirstRuntimeInfoByTagID(FGameplayTag InBuffIDTag);

	int32 GetTotalSourceReferenceCount(const FLxBuffRuntimeInfo& InRuntimeInfo) const;

	/** 启动 Buff 计时器。 */
	void StartBuffTimer();

	/** 如果没有 Buff，则停止 Buff 计时器。 */
	void StopBuffTimerIfNeeded();

	/** Buff 计时器每次触发时处理周期生效和过期移除。 */
	UFUNCTION()
	void HandleBuffTimerTick();

	/** 激活指定 Buff 的周期效果。 */
	void ActivateBuffEntries(ULxBuff* InBuffLogic);

	/** 将服务端 Buff 运行时列表写入网络显示快照。 */
	void SyncReplicatedBuffList();

	/** 客户端收到 Buff 显示快照后重建本地 UI 用 Buff 对象。 */
	void ApplyReplicatedBuffList();

	/** Buff 网络显示快照同步到客户端时调用。 */
	UFUNCTION()
	void OnRep_ReplicatedBuffList();

	/** 当前组件所属角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 当前所有运行时 Buff 缓存。 */
	UPROPERTY()
	TArray<FLxBuffRuntimeInfo> m_vBuffRuntimeInfos;

	/** 用于客户端显示 Buff 图标和剩余时间的网络同步列表。 */
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedBuffList, VisibleAnywhere, Category="Buff|网络", DisplayName="网络同步Buff显示列表")
	TArray<FLxReplicatedBuffRuntimeInfo> ReplicatedBuffList;

	/** Buff 计时器句柄。 */
	FTimerHandle m_BuffTimerHandle;

	/** Buff 组件是否已经初始化。 */
	bool m_bBuffInitialized = false;
};
