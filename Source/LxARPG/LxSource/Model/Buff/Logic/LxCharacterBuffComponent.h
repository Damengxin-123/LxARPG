#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterEntryPackage.h"
#include "LxCharacterBuffComponent.generated.h"

class ALxBaseCharacter;
class ULxEntryObjectBase;

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
};

/**
 * 角色 Buff 组件。
 *
 * 当前组件使用新的物品对象体系创建 Buff：Buff 本身是 ULxBuff，
 * 创建 Buff 的词条来源使用 ULxEntryObjectBase，并从其中读取 FLxEntryCreateBuff 数据。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色Buff组件")
class LXARPG_API ULxCharacterBuffComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建 Buff 组件，并关闭 Tick。 */
	ULxCharacterBuffComponent();

	/** 初始化 Buff 组件。 */
	virtual void BaseComponentInitialize() override;

	/** 组件结束时清空运行时 Buff。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/**
	 * 根据创建 Buff 词条添加 Buff。
	 *
	 * @param InEntryObject 创建 Buff 的运行时词条对象。
	 * @param InCreatorEntryRatio 创建者词条额外比例。
	 * @return 创建或刷新后的 Buff 对象。
	 */
	ULxBuff* AddBuffByCreatorEntry(const ULxEntryObjectBase* InEntryObject, float InCreatorEntryRatio = 1.f, ELxCharacterEntrySource InEntrySource = ELxCharacterEntrySource::Other);

	/** 移除指定 Buff 对象。 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="移除Buff")
	bool RemoveBuff(ULxBuff* InBuffLogic);

	/** 按 Buff 标签 ID 移除 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="移除指定标签ID的Buff", meta=(Categories="物品"))
	int32 RemoveBuffByTagID(FGameplayTag InBuffIDTag);

	/** 移除指定来源对 Buff 的引用；该 Buff 没有任何来源引用后才会真正移除。 */
	int32 RemoveBuffSourceReferenceByTagID(FGameplayTag InBuffIDTag, ELxCharacterEntrySource InEntrySource, int32 InReferenceCount = 1);

	/** 清空所有 Buff。 */
	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="清空所有Buff")
	void ClearBuffs();

	/** 获取当前所有有效 Buff。 */
	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取全部Buff")
	void GetActiveBuffs(TArray<ULxBuff*>& OutBuffList) const;

	/** 获取需要显示的 Buff，目前与有效 Buff 列表一致。 */
	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取显示Buff")
	void GetDisplayBuffs(TArray<ULxBuff*>& OutBuffList) const;

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

	/** 当前组件所属角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 当前所有运行时 Buff 缓存。 */
	UPROPERTY()
	TArray<FLxBuffRuntimeInfo> m_vBuffRuntimeInfos;

	/** Buff 计时器句柄。 */
	FTimerHandle m_BuffTimerHandle;

	/** Buff 组件是否已经初始化。 */
	bool m_bBuffInitialized = false;
};
