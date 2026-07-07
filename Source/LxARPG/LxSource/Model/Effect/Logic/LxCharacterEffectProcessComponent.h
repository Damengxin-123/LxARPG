#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEntryPackage.h"
#include "LxCharacterEffectProcessComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxCharacterLifecycleComponent;
class ULxDamageCalculationFlow;
class ULxSkill;
class ALxSkillUnitActor;

/** 角色受到伤害后的实际承受结果事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterEffectProcessDamageReceived, const FLxDamageReceiveResult&, DamageReceiveResult, AActor*, AttackerActor);

/** 角色效果处理组件，负责解析技能词条、计算伤害并生成最终可传递的效果数据包。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色效果处理组件")
class LXARPG_API ULxCharacterEffectProcessComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色效果处理组件。 */
	ULxCharacterEffectProcessComponent();

	virtual void BaseComponentInitialize() override;

	/** 处理技能命中结果，将技能词条和有效目标转换为最终效果包并交给数据中转组件发送。 */
	UFUNCTION(BlueprintCallable, Category="角色效果处理", DisplayName="处理技能命中效果")
	void ProcessSkillHitEffects(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& InSkillEntryPackages,
		const TArray<AActor*>& HitTargets, bool bPersistentEffect = false,
		ALxSkillUnitActor* PersistentSourceSkillUnit = nullptr);

	/** 向目标发送同技能来源的空替换效果包，解除此前由持续依附或光环施加的缓存效果。 */
	UFUNCTION(BlueprintCallable, Category="角色效果处理", DisplayName="解除持续技能效果")
	void RemovePersistentSkillEffects(ALxSkillUnitActor* SourceSkillUnit, const TArray<AActor*>& EffectTargets);

	/** 基于已有效果包计算当前角色对目标的最终输出效果包。 */
	UFUNCTION(BlueprintCallable, Category="角色效果处理", DisplayName="构建输出效果包")
	bool BuildOutgoingEffectPackage(const FLxEffectPackage& InSourceEffectPackage, AActor* TargetActor, FLxEffectPackage& OutEffectPackage);

	/** 接收包含伤害的效果包，计算最终承受结果并按需应用到当前角色。 */
	UFUNCTION(BlueprintCallable, Category="角色效果处理", DisplayName="接收输入效果包")
	bool ReceiveIncomingEffectPackage(const FLxEffectPackage& InEffectPackage, FLxDamageReceiveResult& OutDamageReceiveResult, bool bApplyResult = true);

	/** 获取效果计算流程。 */
	UFUNCTION(BlueprintPure, Category="角色效果处理", DisplayName="获取效果计算流程")
	ULxDamageCalculationFlow* GetDamageCalculationFlow() const { return DamageCalculationFlow; }

	/** 角色受到伤害后的实际承受结果事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色效果处理", DisplayName="受到伤害事件")
	FOnLxCharacterEffectProcessDamageReceived OnCharacterDamageReceived;

protected:
	/** 当前角色运行时使用的效果计算流程实例，由游戏设置中的全局流程类型创建。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="角色效果处理", DisplayName="运行时效果计算流程")
	TObjectPtr<ULxDamageCalculationFlow> DamageCalculationFlow;

private:
	/** 缓存当前角色身上的依赖组件。 */
	void CacheOwnerComponents();

	/** 确保效果计算流程已经创建。 */
	void EnsureDamageCalculationFlow();

	/** 从技能词条构建待处理的基础效果包。 */
	void BuildEffectPackagesFromSkillEntries(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& InSkillEntryPackages,
		TArray<FLxEffectPackage>& OutEffectPackages, bool bPersistentEffect,
		ALxSkillUnitActor* PersistentSourceSkillUnit) const;

	/** 将最终承伤结果应用到当前角色属性。 */
	void ApplyDamageReceiveResultToTarget(const FLxDamageReceiveResult& InDamageReceiveResult);

	/** 根据生命值刷新角色生命周期。 */
	void RefreshLifecycleAfterDamage();

	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> DataTransferComponent = nullptr;

	UPROPERTY()
	TObjectPtr<ULxCharacterLifecycleComponent> LifecycleComponent = nullptr;
};
