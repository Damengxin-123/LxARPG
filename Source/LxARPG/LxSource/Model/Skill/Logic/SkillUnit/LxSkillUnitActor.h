#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEntryPackage.h"
#include "LxSkillUnitActor.generated.h"

class ULxSkillDetectionComponent;
class ULxSkillLifeComponent;
class ULxSkillMovementComponent;
class ULxSkillPropagationComponent;
class ULxSkillTriggerComponent;
class ALxSkillUnitActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillUnitResultEvent, ALxSkillUnitActor*, SkillUnit, const FLxSkillUnitResult&, SkillUnitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillUnitTriggerEvent, ALxSkillUnitActor*, SkillUnit, const FLxSkillTriggerResult&, TriggerResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSkillUnitPropagationEvent, ALxSkillUnitActor*, SkillUnit, const FLxSkillPropagationResult&, PropagationResult);

/** 技能单元Actor基类，负责协调能力组件和对外提供统一生命周期接口。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="技能单元基类")
class LXARPG_API ALxSkillUnitActor : public AActor
{
	GENERATED_BODY()

public:
	ALxSkillUnitActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 初始化技能单元参数；只分发运行时参数，不自动激活技能单元。 */
	UFUNCTION(BlueprintCallable, Category="技能单元", DisplayName="初始化技能单元")
	virtual void InitializeSkillUnit(const FLxSkillUnitSpec& InSkillUnitSpec);

	/** 在技能单元激活前覆盖目标筛选和命中限制参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|目标规则", DisplayName="设置目标筛选和命中限制")
	void SetTargetRules(const FLxSkillTargetFilterSpec& InTargetFilterSpec,
		const FLxSkillHitLimitSpec& InHitLimitSpec);

	/** 激活技能单元。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元", DisplayName="激活技能单元")
	void ActivateSkillUnit();
	virtual void ActivateSkillUnit_Implementation();

	/** 正常停止技能单元运行，保留 Actor 以便持久技能单元再次激活。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元", DisplayName="停止技能单元")
	void StopSkillUnit();
	virtual void StopSkillUnit_Implementation();

	/** 异常取消技能单元运行。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元", DisplayName="取消技能单元")
	void CancelSkillUnit();
	virtual void CancelSkillUnit_Implementation();

	/** 更新正在运行的技能单元位置和方向，特殊单元可重写以同步额外表现。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元|运行", DisplayName="更新技能单元变换")
	void UpdateSkillUnitTransform(const FTransform& InTransform);
	virtual void UpdateSkillUnitTransform_Implementation(const FTransform& InTransform);

	/** 判断技能单元当前是否正在运行。 */
	UFUNCTION(BlueprintPure, Category="技能单元|运行", DisplayName="技能单元是否正在运行")
	bool IsSkillUnitActive() const { return bSkillUnitActive; }

	/** 设置技能单元是否在 BeginPlay 时自动激活；工厂创建流程会关闭自动激活并交由技能单元组统一控制。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|运行", DisplayName="设置技能单元自动激活")
	void SetAutoActivateSkillUnit(bool bInAutoActivate) { bAutoActivateSkillUnit = bInAutoActivate; }

	/** 设置技能单元创建规则。 */
	UFUNCTION(BlueprintCallable, Category="技能单元", DisplayName="设置技能单元创建规则")
	void SetSkillUnitSpawnSpec(const FLxSkillUnitSpawnSpec& InSpawnSpec);

	/** 设置技能单元携带的效果对象；效果系统接入前仅作为预留数据。 */
	UFUNCTION(BlueprintCallable, Category="技能单元", DisplayName="设置技能单元词条包")
	void SetSkillUnitEntryPackages(const TArray<FLxSkillEntryPackage>& InSkillEntryPackages);

	/** 获取技能单元携带的词条包数组。 */
	UFUNCTION(BlueprintPure, Category="技能单元|词条", DisplayName="获取技能单元词条包数组")
	TArray<FLxSkillEntryPackage> GetSkillUnitEntryPackages() const { return SkillEntryPackages; }

	/** 技能单元参数限制初始化，具体类型可重写以声明自身支持哪些参数。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元", DisplayName="技能单元参数限制初始化")
	void InitializeSkillUnitParameterLimits();
	virtual void InitializeSkillUnitParameterLimits_Implementation();

	/** 技能单元参数默认设置，具体类型可重写以设置组件默认参数。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能单元", DisplayName="技能单元参数默认设置")
	void InitializeSkillUnitDefaultParameters();
	virtual void InitializeSkillUnitDefaultParameters_Implementation();

	/** 获取运动能力组件。 */
	UFUNCTION(BlueprintPure, Category="技能单元|组件", DisplayName="获取运动能力组件")
	ULxSkillMovementComponent* GetSkillMovementComponent() const;

	/** 获取目标检测组件。 */
	UFUNCTION(BlueprintPure, Category="技能单元|组件", DisplayName="获取目标检测组件")
	ULxSkillDetectionComponent* GetSkillDetectionComponent() const;

	/** 获取生命周期组件。 */
	UFUNCTION(BlueprintPure, Category="技能单元|组件", DisplayName="获取生命周期组件")
	ULxSkillLifeComponent* GetSkillLifeComponent() const;

	/** 获取触发能力组件。 */
	UFUNCTION(BlueprintPure, Category="技能单元|组件", DisplayName="获取触发能力组件")
	ULxSkillTriggerComponent* GetSkillTriggerComponent() const;

	/** 获取传播能力组件。 */
	UFUNCTION(BlueprintPure, Category="技能单元|组件", DisplayName="获取传播能力组件")
	ULxSkillPropagationComponent* GetSkillPropagationComponent() const;

	/** 技能单元激活事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元激活事件")
	FOnLxSkillUnitResultEvent OnSkillUnitActivated;

	/** 技能单元确认命中事件，统一为后续技能单元提供可串接的命中结果。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元命中事件")
	FOnLxSkillUnitResultEvent OnSkillUnitHit;

	/** 技能单元结束事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元结束事件")
	FOnLxSkillUnitResultEvent OnSkillUnitFinished;

	/** 技能单元取消事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元取消事件")
	FOnLxSkillUnitResultEvent OnSkillUnitCancelled;

	/** 技能单元触发事件，由触发组件转发。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元触发事件")
	FOnLxSkillUnitTriggerEvent OnSkillUnitTriggered;

	/** 技能单元传播评估事件，由传播组件转发。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|事件", DisplayName="技能单元传播评估事件")
	FOnLxSkillUnitPropagationEvent OnSkillUnitPropagationEvaluated;

protected:
	/** 技能单元参数完成网络同步后刷新客户端表现配置。 */
	UFUNCTION(Category="技能|技能单元|网络", DisplayName="技能单元参数同步")
	void OnRep_SkillUnitSpec();

	/** 技能单元运行状态完成网络同步后驱动客户端表现，不在客户端执行命中逻辑。 */
	UFUNCTION(Category="技能|技能单元|网络", DisplayName="技能单元状态同步")
	void OnRep_SkillUnitActive(bool bOldSkillUnitActive);

	/** 停止技能单元上所有已启动的能力组件。 */
	void StopSkillUnitComponents();

	/** 将当前技能单元参数分发给已装配的能力组件。 */
	virtual void ApplySkillUnitSpecToComponents();

	/** 绑定已装配能力组件的事件。 */
	virtual void BindSkillUnitComponentEvents();

	/** 结束技能单元并广播结果。 */
	UFUNCTION(BlueprintCallable, Category="技能单元", DisplayName="完成技能单元")
	virtual void FinishSkillUnit(const FLxSkillUnitResult& InResult);

	/** 构造一个基础运行结果。 */
	FLxSkillUnitResult MakeSkillUnitResult(ELxSkillUnitResultType InResultType, bool bSuccess) const;

	/** 广播由具体技能单元确认过的通用命中结果。 */
	void PublishSkillUnitHitResult(const FLxSkillUnitResult& HitResult);

	UFUNCTION()
	virtual void HandleLifeStateChanged(ELxSkillAbilityComponentState OldState, ELxSkillAbilityComponentState NewState);

	UFUNCTION()
	virtual void HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult);

	UFUNCTION()
	virtual void HandlePropagationEvaluated(const FLxSkillPropagationResult& PropagationResult);

	/** 技能单元运行时参数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_SkillUnitSpec, Category="技能单元", DisplayName="技能单元参数")
	FLxSkillUnitSpec SkillUnitSpec;

	/** 技能单元携带的词条包，命中目标后可转换为技能效果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|词条", DisplayName="技能单元词条包数组")
	TArray<FLxSkillEntryPackage> SkillEntryPackages;

	/** 是否在BeginPlay时自动激活。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元", DisplayName="自动激活")
	bool bAutoActivateSkillUnit = false;

	bool bSkillUnitInitialized = false;
	/** 由服务端维护并复制的技能单元运行状态。 */
	UPROPERTY(ReplicatedUsing=OnRep_SkillUnitActive, VisibleAnywhere, BlueprintReadOnly, Category="技能|技能单元|网络", DisplayName="技能单元运行状态")
	bool bSkillUnitActive = false;
};
