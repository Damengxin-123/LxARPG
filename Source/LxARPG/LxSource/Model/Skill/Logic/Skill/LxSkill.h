#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillCastContext.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillEnum.h"
#include "LxSkill.generated.h"

class ALxSkillUnitActor;

/** 完整技能类型，负责组织技能单元对象，并提供蓄力与释放入口。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="技能类型")
class LXARPG_API ULxSkill : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化技能释放上下文。技能释放组件会在直接释放、开始蓄力或结束蓄力前调用。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能|释放", DisplayName="初始化技能")
	void InitializeSkill(const FLxSkillCastContext& InCastContext);
	virtual void InitializeSkill_Implementation(const FLxSkillCastContext& InCastContext);

	/** 开启技能蓄力。调用后进入技能蓄力流程，具体表现可在蓝图中继承实现。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能", DisplayName="开启技能蓄力")
	void StartSkillCharge();
	virtual void StartSkillCharge_Implementation();

	/** 结束蓄力并释放技能。调用后退出蓄力状态，具体释放流程可在蓝图中继承实现。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能", DisplayName="结束蓄力")
	void EndSkillCharge();
	virtual void EndSkillCharge_Implementation();

	/** 直接释放技能。适合不需要蓄力的技能，具体释放流程可在蓝图中继承实现。 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="技能", DisplayName="直接释放技能")
	void ReleaseSkillDirectly();
	virtual void ReleaseSkillDirectly_Implementation();

	/** 获取技能释放类型，可用于判断技能是否支持蓄力。 */
	UFUNCTION(BlueprintPure, Category="技能", DisplayName="获取技能类型")
	ELxSkillReleaseType GetSkillReleaseType() const { return SkillReleaseType; }

	/** 判断技能是否可以蓄力。 */
	UFUNCTION(BlueprintPure, Category="技能", DisplayName="是否可以蓄力")
	bool CanSkillCharge() const { return SkillReleaseType == ELxSkillReleaseType::ChargeRelease; }

	/** 添加一个技能单元对象到长期保存列表中。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元", DisplayName="添加技能单元")
	void AddSkillUnit(ALxSkillUnitActor* InSkillUnit);

	/** 从长期保存列表中移除一个技能单元对象。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元", DisplayName="移除技能单元")
	void RemoveSkillUnit(ALxSkillUnitActor* InSkillUnit);

	/** 清空长期保存的技能单元对象列表。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元", DisplayName="清空技能单元列表")
	void ClearSkillUnits();

	/** 获取长期保存的技能单元对象列表。 */
	UFUNCTION(BlueprintPure, Category="技能|技能单元", DisplayName="获取技能单元列表")
	TArray<ALxSkillUnitActor*> GetSkillUnits() const;

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
	FTransform GetSkillSpawnTransform() const { return CurrentCastContext.SpawnTransform; }

	/** 获取直接释放技能响应的输入状态。默认在开始释放时触发。 */
	UFUNCTION(BlueprintPure, Category="技能|释放", DisplayName="获取直接释放响应状态")
	ELxSkillReleaseInputState GetDirectReleaseInputState() const { return DirectReleaseInputState; }

	virtual UWorld* GetWorld() const override;

protected:
	/** 技能释放类型，例如直接释放或蓄力释放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能", DisplayName="技能类型")
	ELxSkillReleaseType SkillReleaseType = ELxSkillReleaseType::DirectRelease;

	/** 直接释放技能应该响应哪个输入阶段。通常按下释放，若希望松开才释放可改为结束释放。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|释放", DisplayName="直接释放响应状态")
	ELxSkillReleaseInputState DirectReleaseInputState = ELxSkillReleaseInputState::Start;

	/** 技能单元列表，用于长期保存和组织此技能创建出来的技能单元对象。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能|技能单元", DisplayName="技能单元列表")
	TArray<TObjectPtr<ALxSkillUnitActor>> SkillUnitList;

	/** 当前是否正在蓄力。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能", DisplayName="正在蓄力")
	bool bCharging = false;

	/** 最近一次释放或蓄力时由技能释放组件传入的上下文。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="技能|释放", DisplayName="技能释放上下文")
	FLxSkillCastContext CurrentCastContext;
};
