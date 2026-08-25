#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatModuleBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillCastContext.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEnum.h"
#include "LxSkillCastComponent.generated.h"

class ULxPlayerAimModule;
class ULxSkill;
struct FLxPlayerAimResult;
struct FLxSkillEntryPackage;
class ULxSkillItem;

/** 释放时间轴到达中点时需要执行的技能事件类型。 */
enum class ELxPendingSkillReleaseExecution : uint8
{
	None,
	Direct,
	Charge,
	Sustained
};

/** 技能释放模块，解释开始、结束、取消等释放输入并调用对应技能流程。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="技能释放模块")
class LXARPG_API ULxSkillCastModule : public ULxCharacterCombatModuleBase
{
	GENERATED_BODY()

public:
	/** 创建技能释放模块。 */
	ULxSkillCastModule();

	/** 绑定统一战斗组件并建立初始释放上下文。 */
	virtual void InitializeModule(ULxCharacterCombatComponent* InOwnerComponent) override;

	/** 取消当前技能并解除瞄准事件。 */
	virtual void ShutdownModule() override;

	/** 构建技能释放上下文。没有传入释放者时默认使用组件拥有者。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="构建技能释放上下文")
	FLxSkillCastContext MakeSkillCastContext(UObject* SourceObject = nullptr, AActor* TargetActor = nullptr,
		FVector AimLocation = FVector::ZeroVector, bool bHasAimLocation = false,
		FVector AimDirection = FVector::ForwardVector, bool bHasAimDirection = false) const;

	/** 用指定上下文初始化技能。技能蓝图可在“初始化技能”事件中缓存或转换释放参数。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="初始化技能")
	bool InitializeSkillForCast(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 直接释放指定技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="直接释放技能")
	bool ReleaseSkillDirectly(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 开始蓄力指定技能。非蓄力技能会返回 false。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="开始技能蓄力")
	bool StartSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 结束当前蓄力技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="结束技能蓄力")
	bool EndSkillCharge(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 开始持续释放指定技能，并独占当前持续释放槽位。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="开始持续释放技能")
	bool StartSustainedRelease(ULxSkill* InSkill, const FLxSkillCastContext& InCastContext);

	/** 正常停止当前持续释放技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="停止持续释放技能")
	bool StopSustainedRelease(ULxSkill* InSkill);

	/** 异常取消当前持续释放技能。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="取消持续释放技能")
	bool CancelSustainedRelease(ULxSkill* InSkill);

	/** 取消当前占用释放组件的技能流程，用于死亡、换装、打断等意外终止。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="取消当前技能释放")
	bool CancelCurrentSkillRelease();

	/** 由异步技能流程显式通知释放组件解除当前技能占用。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="完成当前技能释放")
	bool FinishCurrentSkillRelease(ULxSkill* InSkill);

	/** 处理一次技能释放输入。外部模块只需要传入技能对象、开始/结束/取消状态和释放上下文。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="处理技能释放输入")
	bool HandleSkillReleaseInput(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState, const FLxSkillCastContext& InCastContext);

	/** 直接释放技能物品。玩家和 AI 都可以通过此接口使用技能物品。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="直接释放技能物品")
	bool ReleaseSkillItemDirectly(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 开始使用技能物品。蓄力技能会开始蓄力，非蓄力技能会立即释放。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="开始使用技能物品")
	bool StartUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 结束使用技能物品。蓄力技能会在这里结束蓄力并释放。 */
	UFUNCTION(BlueprintCallable, Category="技能|释放组件", DisplayName="结束使用技能物品")
	bool EndUseSkillItem(ULxSkillItem* InSkillItem, const FLxSkillCastContext& InCastContext);

	/** 获取当前正在蓄力的技能。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件", DisplayName="获取当前蓄力技能")
	ULxSkill* GetChargingSkill() const { return ChargingSkill; }

	/** 获取当前正在持续释放的技能。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件", DisplayName="获取当前持续释放技能")
	ULxSkill* GetSustainedSkill() const { return SustainedSkill; }

	/** 获取当前技能释放占用状态。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件|状态", DisplayName="获取技能释放状态")
	ELxSkillCastState GetSkillCastState() const { return SkillCastState; }

	/** 获取当前占用释放组件的技能。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件|状态", DisplayName="获取当前释放技能")
	ULxSkill* GetCurrentCastingSkill() const { return CurrentCastingSkill; }

	/** 判断技能释放组件当前是否允许开始新的技能。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件|状态", DisplayName="技能释放组件是否空闲")
	bool IsSkillCastIdle() const { return SkillCastState == ELxSkillCastState::Idle; }

	/** 获取最近一次释放上下文。 */
	UFUNCTION(BlueprintPure, Category="技能|释放组件", DisplayName="获取当前释放上下文")
	FLxSkillCastContext GetCurrentCastContext() const { return CurrentCastContext; }

private:
	/** 执行已经到达服务端的技能物品释放输入。 */
	void HandleSkillItemReleaseInputFromServer(FGameplayTag InSkillItemIDTag, ELxSkillReleaseInputState InInputState,
		AActor* InTargetActor, FVector_NetQuantize InAimLocation, bool bInHasAimLocation,
		FVector_NetQuantizeNormal InAimDirection, bool bInHasAimDirection);

	/** 在本机播放技能动作动画。 */
	void PlaySkillActionAnimation(float InSkillReleaseDuration);

	/** 在本机停止技能动作动画。 */
	void StopSkillActionAnimation();

	/** 根据技能对象反查所属技能物品标签，避免通过网络传递不可复制的 UObject。 */
	FGameplayTag ResolveSkillItemIDTag(const ULxSkill* InSkill) const;

	/** 在权威端执行已经通过网络入口校验的技能输入。 */
	bool HandleSkillReleaseInputAuthority(ULxSkill* InSkill, ELxSkillReleaseInputState InInputState,
		const FLxSkillCastContext& InCastContext);

	/** 启动技能释放时间轴，在百分之五十执行技能事件，在百分之百记录冷却起点。 */
	void BeginTimedSkillRelease(ULxSkill* InSkill, ELxPendingSkillReleaseExecution InExecutionType);

	/** 释放时间轴到达百分之五十时执行对应技能蓝图事件。 */
	void ExecuteTimedSkillRelease();

	/** 释放时间轴结束时记录技能冷却起点并解除一次性技能占用。 */
	void CompleteTimedSkillRelease();

	/** 清理释放时间轴定时器，可选择同时取消技能内部释放占用。 */
	void ClearTimedSkillRelease(bool bCancelSkillTiming);

	FLxSkillCastContext NormalizeCastContext(const FLxSkillCastContext& InCastContext, UObject* SourceObject = nullptr) const;
	void BeginSustainedAimTracking();
	void EndSustainedAimTracking();

	/** 清理当前技能、技能物品和占用状态。 */
	void ResetSkillCastState();

	/** 更新技能占用状态，并同步通知统一角色行为控制组件。 */
	void SetSkillCastState(ELxSkillCastState InNewState);

	/** 接收持续计算出的玩家技能起点和方向，并更新当前持久技能单元。 */
	UFUNCTION()
	void HandleAimResultChanged(const FLxPlayerAimResult& AimResult);

	/** 接收技能命中词条事件，并转交给角色效果处理组件。 */
	UFUNCTION()
	void HandleSkillHitEntriesReady(ULxSkill* SourceSkill, const TArray<FLxSkillEntryPackage>& SkillEntryPackages, const TArray<AActor*>& HitTargets);

	/** 将持续技能命中词条按可替换来源传递给效果处理组件。 */
	UFUNCTION()
	void HandlePersistentSkillHitEntriesReady(ULxSkill* SourceSkill, ALxSkillUnitActor* SourceSkillUnit,
		const TArray<FLxSkillEntryPackage>& SkillEntryPackages, const TArray<AActor*>& HitTargets);

	/** 将持续技能效果解除目标传递给效果处理组件。 */
	UFUNCTION()
	void HandleSkillEffectsRemoved(ULxSkill* SourceSkill, ALxSkillUnitActor* SourceSkillUnit,
		const TArray<AActor*>& EffectTargets);

	/** 当前技能释放组件的互斥占用状态。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|释放组件|状态", DisplayName="技能释放状态", meta=(AllowPrivateAccess="true"))
	ELxSkillCastState SkillCastState = ELxSkillCastState::Idle;

	/** 当前占用技能释放组件的技能。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|释放组件|状态", DisplayName="当前释放技能", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxSkill> CurrentCastingSkill = nullptr;

	/** 当前正在蓄力的技能。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> ChargingSkill = nullptr;

	/** 当前正在蓄力的技能物品。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkillItem> ChargingSkillItem = nullptr;

	/** 当前正在持续释放的技能。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> SustainedSkill = nullptr;

	/** 当前正在持续释放的技能物品。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkillItem> SustainedSkillItem = nullptr;

	/** 当前提供持续技能起点和方向变化的玩家瞄准组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerAimModule> SustainedAimComponent = nullptr;

	/** 最近一次释放上下文。 */
	UPROPERTY(Transient)
	FLxSkillCastContext CurrentCastContext;

	/** 当前释放时间轴中点需要执行的技能事件类型。 */
	ELxPendingSkillReleaseExecution PendingSkillReleaseExecution = ELxPendingSkillReleaseExecution::None;

	/** 技能释放时间轴百分之五十执行点定时器。 */
	FTimerHandle SkillReleaseExecutionTimerHandle;

	/** 技能释放时间轴结束点定时器。 */
	FTimerHandle SkillReleaseCompletionTimerHandle;

	/** 允许统一战斗组件调用模块的网络入口与本机动画处理。 */
	friend class ULxCharacterCombatComponent;
};
