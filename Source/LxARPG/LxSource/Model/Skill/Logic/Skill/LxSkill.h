#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillCastContext.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEnum.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEntryPackage.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillMeleeSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitCreateParams.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxSkill.generated.h"

class ALxSkillUnitActor;
class ULxSkill;
class ULxSkillUnitGroup;
class ALxStraightProjectileSkillUnitActor;
class ALxGroundBounceProjectileSkillUnitActor;
class ALxLobProjectileSkillUnitActor;
class ALxDirectHitAreaSkillUnitActor;
class ALxDurationAreaSkillUnitActor;
class ALxScalingAreaSkillUnitActor;
class ALxMeleeSkillUnitActor;
class ALxSingleRaySkillUnitActor;
class ALxContinuousRaySkillUnitActor;
class ALxContinuousAttachEffectSkillUnitActor;
class ALxPeriodicAttachEffectSkillUnitActor;
class ALxContinuousAuraEffectSkillUnitActor;
class ALxPeriodicAuraEffectSkillUnitActor;

/** 技能命中词条事件，通知技能释放组件把命中词条和有效目标交给效果处理组件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLxSkillHitEntriesReady, ULxSkill*, SourceSkill, const TArray<FLxSkillEntryPackage>&, SkillEntryPackages, const TArray<AActor*>&, HitTargets);

/** 技能持续效果解除事件，通知释放组件向目标发送同来源空替换效果包。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillEffectsRemoved, ULxSkill*, SourceSkill,
	const TArray<AActor*>&, EffectTargets);

/** 完整技能类型，负责组织技能单元对象，并提供蓄力与释放入口。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="技能类型")
class LXARPG_API ULxSkill : public UObject
{
	GENERATED_BODY()

public:
	/** 更新本次释放上下文，并确保技能初始化事件在技能对象生命周期内只执行一次。 */
	void PrepareSkillForCast(const FLxSkillCastContext& InCastContext);

	/** 首次使用技能时初始化技能；持久技能单元应在这里创建并保存。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能|释放", DisplayName="初始化技能")
	void InitializeSkill(const FLxSkillCastContext& InCastContext);
	virtual void InitializeSkill_Implementation(const FLxSkillCastContext& InCastContext);

	/** 尝试开启技能蓄力，供技能释放组件调用并负责类型、冷却与蓄力状态校验。 */
	UFUNCTION(BlueprintCallable, Category="技能|蓄力", DisplayName="尝试开启技能蓄力")
	bool TryStartSkillCharge();

	/** 开启技能蓄力事件，只通知蓝图继续蓄力流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|蓄力|事件", DisplayName="开启技能蓄力")
	void StartSkillCharge();

	/** 尝试结束蓄力并释放技能，供技能释放组件调用并负责蓄力状态与冷却校验。 */
	UFUNCTION(BlueprintCallable, Category="技能|蓄力", DisplayName="尝试结束蓄力")
	bool TryEndSkillCharge();

	/** 结束蓄力事件，只通知蓝图继续技能释放流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|蓄力|事件", DisplayName="结束蓄力")
	void EndSkillCharge();

	/**
	 * 尝试直接释放技能。
	 * 供技能释放组件调用，负责检查冷却、记录释放时间并通知蓝图执行释放流程。
	 */
	UFUNCTION(BlueprintCallable, Category="技能|释放", DisplayName="尝试直接释放技能")
	bool TryReleaseSkillDirectly();

	/**
	 * 直接释放技能事件。
	 * 只用于把已通过逻辑校验的释放行为通知给蓝图，蓝图从该事件继续连接技能执行流程。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|释放|事件", DisplayName="直接释放技能")
	void ReleaseSkillDirectly();

	/** 尝试取消当前技能释放，供技能释放组件调用并负责清理蓄力和持久单元状态。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放", DisplayName="尝试取消技能释放")
	bool TryCancelSkillRelease();

	/** 取消技能释放事件，只通知蓝图执行额外清理流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|释放|事件", DisplayName="取消技能释放")
	void CancelSkillRelease();

	/** 尝试开始持续释放，供技能释放组件调用并负责类型、冷却与持久单元校验。 */
	UFUNCTION(BlueprintCallable, Category="技能|持续释放", DisplayName="尝试开始持续释放")
	bool TryStartSustainedRelease();

	/** 开始持续释放事件，只通知蓝图继续执行流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|持续释放|事件", DisplayName="开始持续释放")
	void StartSustainedRelease();

	/** 尝试停止持续释放，供技能释放组件调用并负责停止持久技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能|持续释放", DisplayName="尝试停止持续释放")
	bool TryStopSustainedRelease();

	/** 停止持续释放事件，只通知蓝图继续执行流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|持续释放|事件", DisplayName="停止持续释放")
	void StopSustainedRelease();

	/** 尝试取消持续释放，供技能释放组件调用并负责取消持久技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能|持续释放", DisplayName="尝试取消持续释放")
	bool TryCancelSustainedRelease();

	/** 取消持续释放事件，只通知蓝图继续执行流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|持续释放|事件", DisplayName="取消持续释放")
	void CancelSustainedRelease();

	/** 尝试更新持续释放变换，供技能释放组件调用并负责持久技能单元状态校验。 */
	UFUNCTION(BlueprintCallable, Category="技能|持续释放", DisplayName="尝试更新持续释放变换")
	bool TryUpdateSustainedReleaseTransform(const FTransform& InTransform);

	/** 更新持续释放变换事件，只通知蓝图继续执行流程。 */
	UFUNCTION(BlueprintImplementableEvent, Category="技能|持续释放|事件", DisplayName="更新持续释放变换")
	void UpdateSustainedReleaseTransform(const FTransform& InTransform);

	/** 接收近身战斗组件已经筛选为有效的武器命中，并交给对应技能单元中间层输出技能事件。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能|近战", DisplayName="接收近战武器命中")
	bool ReceiveMeleeWeaponHit(ULxSkillUnitGroup* InMeleeSkillUnitGroup, const FLxMeleeHitContext& InHitContext);
	virtual bool ReceiveMeleeWeaponHit_Implementation(ULxSkillUnitGroup* InMeleeSkillUnitGroup, const FLxMeleeHitContext& InHitContext);

	/** 接收单个技能命中目标和需要处理的词条包数组，并广播给技能释放组件。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能|效果", DisplayName="接收单目标技能词条")
	void ReceiveSkillEffectForTarget(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, AActor* HitTarget);
	virtual void ReceiveSkillEffectForTarget_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, AActor* HitTarget);

	/** 接收多个技能命中目标和需要处理的词条包数组，并广播给技能释放组件。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能|效果", DisplayName="接收多目标技能词条")
	void ReceiveSkillEffectForTargets(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets);
	virtual void ReceiveSkillEffectForTargets_Implementation(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages, const TArray<AActor*>& HitTargets);

	/** 技能命中词条准备完成事件，由技能释放组件监听并继续转交效果处理组件。 */
	UPROPERTY(BlueprintAssignable, Category="技能|效果", DisplayName="技能命中词条准备完成事件")
	FOnLxSkillHitEntriesReady OnSkillHitEntriesReady;

	/** 持续依附或持续光环命中时触发，要求效果层按技能来源缓存并支持后续解除。 */
	UPROPERTY(BlueprintAssignable, Category="技能|效果", DisplayName="持续技能效果准备完成事件")
	FOnLxSkillHitEntriesReady OnPersistentSkillHitEntriesReady;

	/** 持续依附或光环效果结束时需要解除效果的目标事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能|效果", DisplayName="技能效果解除事件")
	FOnLxSkillEffectsRemoved OnSkillEffectsRemoved;

	/** 获取技能类型默认配置的词条包数组。 */
	UFUNCTION(BlueprintPure, Category="技能|词条", DisplayName="获取技能词条包数组")
	TArray<FLxSkillEntryPackage> GetSkillEntryPackages() const { return SkillEntryPackages; }

	/** 获取技能唯一标签。 */
	UFUNCTION(BlueprintPure, Category="技能|基础信息", DisplayName="获取技能ID")
	FGameplayTag GetSkillIDTag() const { return SkillIDTag; }

	/** 获取技能释放类型，可用于判断技能是否支持蓄力。 */
	UFUNCTION(BlueprintPure, Category="技能", DisplayName="获取技能类型")
	ELxSkillReleaseType GetSkillReleaseType() const { return SkillReleaseType; }

	/** 判断释放组件是否需要等待技能显式通知释放结束。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="是否等待显式结束释放")
	bool ShouldHoldReleaseStateUntilExplicitFinish() const { return bHoldReleaseStateUntilExplicitFinish; }

	/** 判断技能是否可以蓄力。 */
	UFUNCTION(BlueprintPure, Category="技能", DisplayName="是否可以蓄力")
	bool CanSkillCharge() const { return SkillReleaseType == ELxSkillReleaseType::ChargeRelease; }

	/** 判断技能是否采用按住期间持续运行的释放方式。 */
	UFUNCTION(BlueprintPure, Category="技能|持续释放", DisplayName="是否持续释放技能")
	bool IsSustainedReleaseSkill() const { return SkillReleaseType == ELxSkillReleaseType::SustainedRelease; }

	/** 获取技能实际释放冷却，最低限制使用项目统一动作时间间隔。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取实际释放冷却")
	float GetEffectiveReleaseCooldown() const;

	/** 判断技能释放冷却是否已经结束。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="释放冷却是否结束")
	bool IsReleaseCooldownReady() const;

	/** 尝试开始一次技能释放；仅在冷却结束时记录释放时间并返回成功。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放", DisplayName="尝试开始技能释放")
	bool TryBeginSkillRelease();

	/** 记录本次技能释放时间，用于后续释放冷却判断。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放", DisplayName="记录技能释放时间")
	void MarkSkillReleased();

	/** 根据通用技能单元结果中的目标和位置创建直线投射物；输入为空时使用技能释放锚点。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|投射物", DisplayName="创建直线投射物",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateStraightProjectileUnits(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxStraightProjectileSkillUnitActor> SkillUnitClass,
		const FLxProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 根据通用技能单元结果中的目标和位置创建地面弹跳投射物。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|投射物", DisplayName="创建弹跳投射物",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateGroundBounceProjectileUnits(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxGroundBounceProjectileSkillUnitActor> SkillUnitClass,
		const FLxGroundBounceProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 根据通用技能单元结果中的目标和位置创建抛射投射物。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|投射物", DisplayName="创建抛射投射物",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateLobProjectileUnits(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxLobProjectileSkillUnitActor> SkillUnitClass,
		const FLxLobProjectileSkillUnitCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 在通用技能单元结果的每个位置分别创建一个直接命中型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|范围效果", DisplayName="创建直接命中型范围效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateDirectHitAreaEffects(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxDirectHitAreaSkillUnitActor> SkillUnitClass,
		const FLxDirectHitAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 在通用技能单元结果的每个位置分别创建一个持续型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|范围效果", DisplayName="创建持续型范围效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateDurationAreaEffects(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxDurationAreaSkillUnitActor> SkillUnitClass,
		const FLxDurationAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 在通用技能单元结果的每个位置分别创建一个缩放型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|范围效果", DisplayName="创建缩放型范围效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateScalingAreaEffects(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxScalingAreaSkillUnitActor> SkillUnitClass,
		const FLxScalingAreaEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 创建技能内唯一的近战效果；持久化时重复调用将返回原有中间层。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|近战", DisplayName="创建近战效果")
	ULxSkillUnitGroup* CreateMeleeEffect(TSubclassOf<ALxMeleeSkillUnitActor> SkillUnitClass,
		const FLxMeleeSkillUnitCreateParams& CreateParams, bool bPersistent = true);

	/** 根据通用技能单元结果中的每个位置创建一批单次射线效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|射线", DisplayName="创建单次射线效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateSingleRayEffectUnits(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxSingleRaySkillUnitActor> SkillUnitClass,
		const FLxSingleRayEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 创建可重复启停的持续射线；持久化时重复调用将返回原有中间层。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|射线", DisplayName="创建持续射线效果")
	ULxSkillUnitGroup* CreateContinuousRayEffectUnit(TSubclassOf<ALxContinuousRaySkillUnitActor> SkillUnitClass,
		const FLxContinuousRayEffectCreateParams& CreateParams, bool bPersistent = true);

	/** 根据通用结果中的目标列表，为每个有效目标创建持续生效依附效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|依附效果", DisplayName="创建持续依附效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreateContinuousAttachEffects(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxContinuousAttachEffectSkillUnitActor> SkillUnitClass,
		const FLxContinuousAttachEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 根据通用结果中的目标列表，为每个有效目标创建周期触发依附效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|依附效果", DisplayName="创建周期依附效果",
		meta=(AutoCreateRefTerm="InSourceResult", AdvancedDisplay="bActivateAfterCreate"))
	ULxSkillUnitGroup* CreatePeriodicAttachEffects(const FLxSkillUnitResult& InSourceResult,
		TSubclassOf<ALxPeriodicAttachEffectSkillUnitActor> SkillUnitClass,
		const FLxPeriodicAttachEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 创建依附于释放角色的持续型光环效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|光环效果", DisplayName="创建持续型光环效果")
	ULxSkillUnitGroup* CreateContinuousAuraEffectUnit(TSubclassOf<ALxContinuousAuraEffectSkillUnitActor> SkillUnitClass,
		const FLxContinuousAuraEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 创建依附于释放角色的周期触发型光环效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|光环效果", DisplayName="创建周期型光环效果")
	ULxSkillUnitGroup* CreatePeriodicAuraEffectUnit(TSubclassOf<ALxPeriodicAuraEffectSkillUnitActor> SkillUnitClass,
		const FLxPeriodicAuraEffectCreateParams& CreateParams, bool bActivateAfterCreate = true);

	/** 保存技能初始化时创建、后续可反复启停的唯一持久技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能|持久技能单元", DisplayName="设置持久技能单元")
	bool SetPersistentSkillUnitGroup(ULxSkillUnitGroup* InSkillUnitGroup);

	/** 获取技能初始化时创建的持久技能单元。 */
	UFUNCTION(BlueprintPure, Category="技能|持久技能单元", DisplayName="获取持久技能单元")
	ULxSkillUnitGroup* GetPersistentSkillUnitGroup() const { return PersistentSkillUnitGroup; }

	/** 判断持久技能单元当前是否正在运行。 */
	UFUNCTION(BlueprintPure, Category="技能|持久技能单元", DisplayName="持久技能单元是否正在运行")
	bool IsPersistentSkillUnitGroupActive() const;

	/** 创建并缓存一个运行时技能单元组，用于统一接收一次创建出的多个技能单元事件。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元组", DisplayName="创建技能单元组")
	ULxSkillUnitGroup* CreateSkillUnitGroup(const TArray<ALxSkillUnitActor*>& InSkillUnits);

	/** 释放一个运行时技能单元组缓存，并解绑它管理的技能单元事件。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元组", DisplayName="释放技能单元组")
	bool ReleaseSkillUnitGroup(ULxSkillUnitGroup* InSkillUnitGroup);

	/** 获取当前技能对象缓存的运行时技能单元组。 */
	UFUNCTION(BlueprintPure, Category="技能|技能单元组", DisplayName="获取技能单元组缓存")
	TArray<ULxSkillUnitGroup*> GetCachedSkillUnitGroups() const;

	/** 获取最近一次初始化写入的技能释放上下文。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取技能释放上下文")
	FLxSkillCastContext GetSkillCastContext() const { return CurrentCastContext; }

	/** 获取技能蓝图创建技能单元时可传入的世界上下文对象。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取世界上下文对象")
	UObject* GetSkillWorldContextObject() const { return CurrentCastContext.WorldContextObject; }

	/** 获取当前释放者。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取释放者")
	AActor* GetSkillCasterActor() const { return CurrentCastContext.CasterActor; }

	/** 获取当前释放目标。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取释放目标")
	AActor* GetSkillTargetActor() const { return CurrentCastContext.TargetActor; }

	/** 获取当前默认生成变换。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取默认生成变换")
	FTransform GetSkillSpawnTransform() const;

	/** 获取直接释放技能响应的输入状态。默认在开始释放时触发。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取直接释放响应状态")
	ELxSkillReleaseInputState GetDirectReleaseInputState() const { return DirectReleaseInputState; }

	virtual UWorld* GetWorld() const override;

protected:
	/** 将通用技能单元结果转换为逐项对齐的生成变换，并按选择应用前置结果方向。 */
	TArray<FTransform> BuildSpawnTransforms(const FLxSkillUnitResult& InSourceResult,
		ELxSkillResultDirectionType DirectionType = ELxSkillResultDirectionType::KeepSourceRotation) const;

	/** 获取前置结果中指定目标的生成方向，无法解析时返回零向量。 */
	FVector ResolveResultDirection(const FLxSkillUnitResult& InSourceResult, int32 TargetIndex,
		ELxSkillResultDirectionType DirectionType) const;

	/** 解析射线创建方向：显式方向优先，否则使用释放者指向目标的方向，最后回退到默认生成朝向。 */
	FVector ResolveRaySpawnDirection(const FVector& ExplicitDirection, AActor* PreferredTarget) const;

	UFUNCTION()
	void HandleCachedSkillUnitGroupFinished(ULxSkillUnitGroup* InSkillUnitGroup,
		const TArray<FVector>& InDestroyedLocations);

	/** 接收中间层转发的通用技能单元结果，并将其中的目标列表交给技能效果传递链路。 */
	UFUNCTION()
	void HandleSkillUnitGroupHit(ULxSkillUnitGroup* InSkillUnitGroup, const FLxSkillUnitResult& InSkillUnitResult);

	/** 接收中间层的持续效果解除目标，并转发到技能释放组件。 */
	UFUNCTION()
	void HandleSkillUnitGroupEffectsRemoved(ULxSkillUnitGroup* InSkillUnitGroup, const TArray<AActor*>& InEffectTargets);

	/** 技能唯一标签，用于标识技能所属分类和具体类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|基础信息", DisplayName="技能ID", meta=(Categories="技能"))
	FGameplayTag SkillIDTag;

	/** 技能释放类型，例如直接释放、持续释放或蓄力释放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能", DisplayName="技能类型")
	ELxSkillReleaseType SkillReleaseType = ELxSkillReleaseType::DirectRelease;

	/** 直接释放技能应该响应哪个输入阶段。通常按下释放，若希望松开才释放可改为结束释放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放", DisplayName="直接释放响应状态")
	ELxSkillReleaseInputState DirectReleaseInputState = ELxSkillReleaseInputState::Start;

	/** 技能释放冷却，实际生效值不会低于项目统一动作时间间隔。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放", DisplayName="释放冷却", meta=(ClampMin="0.1", UIMin="0.1"))
	float ReleaseCooldown = 1.0f;

	/** 直接释放或结束蓄力后，是否保持释放组件占用，直到技能显式通知结束。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放", DisplayName="等待显式结束释放")
	bool bHoldReleaseStateUntilExplicitFinish = false;

	/** 技能默认词条包数组，蓝图中可手动添加，命中时可选择一个或多个词条包传入命中函数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|词条", DisplayName="技能词条包数组")
	TArray<FLxSkillEntryPackage> SkillEntryPackages;

	/** 技能初始化时创建并长期复用的唯一技能单元，例如近战单元或持续射线单元。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|持久技能单元", DisplayName="持久技能单元")
	TObjectPtr<ULxSkillUnitGroup> PersistentSkillUnitGroup = nullptr;

	/** 技能长期复用的近战效果中间层。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|持久技能单元", DisplayName="持久近战效果")
	TObjectPtr<ULxSkillUnitGroup> PersistentMeleeSkillUnitGroup = nullptr;

	/** 技能长期复用的持续射线中间层。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|持久技能单元", DisplayName="持久持续射线")
	TObjectPtr<ULxSkillUnitGroup> PersistentContinuousRaySkillUnitGroup = nullptr;

	/** 技能对象动态缓存的运行时技能单元组，防止一次释放流程中的中间层被回收。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|技能单元组", DisplayName="运行时技能单元组缓存")
	TArray<TObjectPtr<ULxSkillUnitGroup>> RuntimeSkillUnitGroups;

	/** 技能对象是否已经执行过一次初始化事件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|初始化", DisplayName="技能已经初始化")
	bool bSkillInitialized = false;

	/** 当前是否正在蓄力。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能", DisplayName="正在蓄力")
	bool bCharging = false;

	/** 当前是否正在持续释放，用于校验停止、取消和变换更新请求。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|持续释放", DisplayName="正在持续释放")
	bool bSustainedReleasing = false;

	/** 最近一次释放或蓄力时由技能释放组件传入的上下文。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|释放", DisplayName="技能释放上下文")
	FLxSkillCastContext CurrentCastContext;

	/** 上一次成功释放技能的世界时间。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|释放", DisplayName="上次释放时间")
	float LastReleaseTime = -100000000.0f;
};
