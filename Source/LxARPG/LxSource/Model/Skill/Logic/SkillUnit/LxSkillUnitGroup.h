#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMeleeSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAttachEffectSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillAuraEffectSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxSkillUnitGroup.generated.h"

class ALxSkillUnitActor;
class ULxSkillUnitGroup;
class ALxAttachEffectSkillUnitActor;
class ALxContinuousAuraEffectSkillUnitActor;
struct FLxAttachEffectEndResult;
struct FLxAuraTargetEffectRemoveResult;

/** 技能单元组命中事件，仅转发组内技能单元本次产生的命中结果，避免重复结算历史目标。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillUnitGroupHit, ULxSkillUnitGroup*, SkillUnitGroup, const FLxSkillUnitResult&, SkillUnitResult);

/** 技能单元组生命周期事件，用于通知技能对象释放运行时缓存。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillUnitGroupLifecycle, ULxSkillUnitGroup*, SkillUnitGroup,
	const TArray<FVector>&, DestroyedLocations);

/** 技能单元组持续效果解除事件，将依附或光环单元需要解除效果的目标统一为目标列表。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLxSkillUnitGroupEffectsRemoved, ULxSkillUnitGroup*, SkillUnitGroup,
	ALxSkillUnitActor*, SourceSkillUnit, const TArray<AActor*>&, EffectTargets);

/** 技能单元组运行时对象，集中管理一次创建出的多个技能单元事件与生命周期。 */
UCLASS(BlueprintType, DisplayName="技能单元组")
class LXARPG_API ULxSkillUnitGroup : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化技能单元组，并绑定传入技能单元的命中与销毁事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组", DisplayName="初始化技能单元组")
	void InitializeSkillUnitGroup(const TArray<ALxSkillUnitActor*>& InSkillUnits);

	/** 添加一个技能单元到组内并绑定统一事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组", DisplayName="添加技能单元")
	bool AddSkillUnit(ALxSkillUnitActor* InSkillUnit);

	/** 添加多个技能单元到组内并绑定统一事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组", DisplayName="添加技能单元数组")
	int32 AddSkillUnits(const TArray<ALxSkillUnitActor*>& InSkillUnits);

	/** 设置技能单元结束后是否自动销毁，用于区分临时组与可复用持久组。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|生命周期", DisplayName="设置结束后销毁技能单元")
	void SetDestroyUnitsWhenFinished(bool bInDestroyUnitsWhenFinished) { bDestroyUnitsWhenFinished = bInDestroyUnitsWhenFinished; }

	/** 激活组内所有仍然有效的技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|运行", DisplayName="激活技能单元组")
	void ActivateSkillUnits();

	/** 停止组内所有仍然有效的技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|运行", DisplayName="停止技能单元组")
	void StopSkillUnits();

	/** 取消组内所有仍然有效的技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|运行", DisplayName="取消技能单元组")
	void CancelSkillUnits();

	/** 使用新的世界变换更新组内所有仍然有效的技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|运行", DisplayName="更新技能单元组变换")
	void UpdateSkillUnitsTransform(const FTransform& InTransform);

	/** 判断组内是否至少存在一个正在运行的技能单元。 */
	UFUNCTION(BlueprintPure, Category="技能单元组|运行", DisplayName="技能单元组是否正在运行")
	bool HasActiveSkillUnits() const;

	/** 将近身战斗组件确认的有效武器命中转交给组内近战技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|近战", DisplayName="接收近战武器命中")
	bool ReceiveMeleeWeaponHit(const FLxMeleeHitContext& InHitContext);

	/** 获取组内第一个近战技能单元的命中限制参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|近战", DisplayName="获取近战效果参数")
	bool GetMeleeSpec(FLxSkillMeleeSpec& OutMeleeSpec) const;

	/** 销毁组内所有仍然有效的技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|运行", DisplayName="销毁技能单元组")
	void DestroySkillUnits();

	/** 清空技能单元组事件绑定与引用。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组", DisplayName="清空技能单元组")
	void ClearSkillUnits();

	/** 获取组内当前仍然有效的技能单元。 */
	UFUNCTION(BlueprintPure, Category="技能单元组", DisplayName="获取技能单元数组")
	TArray<ALxSkillUnitActor*> GetSkillUnits() const;

	/** 获取组内当前仍然有效的技能单元数量。 */
	UFUNCTION(BlueprintPure, Category="技能单元组", DisplayName="获取有效技能单元数量")
	int32 GetValidSkillUnitCount() const;

	/** 获取技能单元组累计输出的命中结果列表。 */
	UFUNCTION(BlueprintPure, Category="技能单元组|命中结果", DisplayName="获取累计命中结果")
	FLxSkillUnitResult GetAccumulatedHitResult() const { return AccumulatedHitResult; }

	/** 清空技能单元组累计输出的命中结果列表。 */
	UFUNCTION(BlueprintCallable, Category="技能单元组|命中结果", DisplayName="清空累计命中结果")
	void ClearAccumulatedHitResult();

	/** 判断技能单元组是否已经没有有效技能单元。 */
	UFUNCTION(BlueprintPure, Category="技能单元组", DisplayName="技能单元组是否为空")
	bool IsSkillUnitGroupEmpty() const;

	/** 组内任意技能单元产生命中结果时触发，事件参数只包含本次命中结果。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元组|事件", DisplayName="技能单元组命中事件")
	FOnLxSkillUnitGroupHit OnSkillUnitGroupHit;

	/** 组内所有技能单元都销毁后触发，并携带每个技能单元销毁时的世界位置。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元组|事件", DisplayName="技能单元组完成事件")
	FOnLxSkillUnitGroupLifecycle OnSkillUnitGroupFinished;

	/** 组内持续依附或光环效果需要从目标身上解除时触发。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元组|事件", DisplayName="技能单元组效果解除事件")
	FOnLxSkillUnitGroupEffectsRemoved OnSkillUnitGroupEffectsRemoved;

	virtual void BeginDestroy() override;

private:
	/** 将组内技能单元的单次命中结果合并到累计命中结果列表。 */
	void MergeSkillUnitHitResult(const FLxSkillUnitResult& InSkillUnitResult);

	/** 绑定单个技能单元的事件。 */
	void BindSkillUnitEvents(ALxSkillUnitActor* InSkillUnit);

	/** 解绑单个技能单元的事件。 */
	void UnbindSkillUnitEvents(ALxSkillUnitActor* InSkillUnit);

	/** 移除一个技能单元引用并在组为空时广播完成事件。 */
	void RemoveManagedSkillUnit(ALxSkillUnitActor* InSkillUnit);

	/** 在组内没有有效技能单元时广播完成事件。 */
	void BroadcastFinishedIfEmpty();

	/** 判断技能单元是否已经被组管理。 */
	bool ContainsSkillUnit(ALxSkillUnitActor* InSkillUnit) const;

	UFUNCTION()
	void HandleManagedSkillUnitHit(ALxSkillUnitActor* InSkillUnit, const FLxSkillUnitResult& InSkillUnitResult);

	UFUNCTION()
	void HandleManagedSkillUnitFinished(ALxSkillUnitActor* InSkillUnit, const FLxSkillUnitResult& InSkillUnitResult);

	UFUNCTION()
	void HandleManagedSkillUnitDestroyed(AActor* DestroyedActor);

	/** 将依附效果结束事件转换为统一目标列表。 */
	UFUNCTION()
	void HandleAttachEffectEnded(ALxAttachEffectSkillUnitActor* SkillUnit, const FLxAttachEffectEndResult& EndResult);

	/** 将光环目标离开事件转换为统一目标列表。 */
	UFUNCTION()
	void HandleAuraTargetEffectRemoved(ALxContinuousAuraEffectSkillUnitActor* SkillUnit,
		const FLxAuraTargetEffectRemoveResult& RemoveResult);

	/** 技能单元组累计输出的命中结果列表。 */
	UPROPERTY(Transient)
	FLxSkillUnitResult AccumulatedHitResult;

	/** 组内当前管理的技能单元引用。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ALxSkillUnitActor>> ManagedSkillUnits;

	/** 记录本组所有技能单元失效销毁时的世界位置，供完成事件创建后续技能单元。 */
	UPROPERTY(Transient)
	TArray<FVector> DestroyedSkillUnitLocations;

	/** 技能单元结束时是否自动销毁；持久技能单元组会关闭此选项以便重复启停。 */
	bool bDestroyUnitsWhenFinished = true;

	/** 是否已经广播过完成事件，避免重复释放技能对象缓存。 */
	bool bFinishedBroadcasted = false;
};
