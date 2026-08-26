#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitCreateParams.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxSkillUnitCreateAsyncAction.generated.h"

class ALxScalingAreaSkillUnitActor;
class ALxStraightProjectileSkillUnitActor;
class ULxSkill;
class ULxSkillUnitGroup;

	/** 异步技能单元事件，使用技能单元结果统一传递目标与各类位置数组。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxAsyncSkillUnitEvent,
	const FLxSkillUnitResult&, SkillUnitResult);

/**
 * 技能单元异步创建节点，在绑定蓝图执行出口后创建并激活技能单元组。
 * 命中出口可以反复触发，失效出口仅在整组技能单元全部结束后触发一次。
 */
UCLASS(DisplayName="技能单元异步创建节点")
class LXARPG_API ULxSkillUnitCreateAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** 异步创建并监听一组直线投射物。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|异步|投射物", DisplayName="创建直线投射物（异步）",
		meta=(BlueprintInternalUseOnly="true", DefaultToSelf="InSkill", HidePin="InSkill",
			AutoCreateRefTerm="InSourceResult,InTargetFilterSpec,InHitLimitSpec"))
	static ULxSkillUnitCreateAsyncAction* CreateStraightProjectileUnitsAsync(
		UPARAM(DisplayName="技能对象") ULxSkill* InSkill,
		UPARAM(DisplayName="前置技能单元结果") const FLxSkillUnitResult& InSourceResult,
		UPARAM(DisplayName="技能子单元类型") TSubclassOf<ALxStraightProjectileSkillUnitActor> InSkillUnitClass,
		UPARAM(DisplayName="创建参数") const FLxProjectileSkillUnitCreateParams& InCreateParams,
		UPARAM(DisplayName="目标筛选参数") const FLxSkillTargetFilterSpec& InTargetFilterSpec,
		UPARAM(DisplayName="命中限制参数") const FLxSkillHitLimitSpec& InHitLimitSpec,
		UPARAM(DisplayName="单元创建位置") ELxSkillUnitResultSpawnLocationType InSpawnLocationType);

	/** 异步创建并监听一组缩放型范围效果。 */
	UFUNCTION(BlueprintCallable, Category="技能|技能单元创建|异步|范围效果", DisplayName="创建缩放型范围效果（异步）",
		meta=(BlueprintInternalUseOnly="true", DefaultToSelf="InSkill", HidePin="InSkill",
			AutoCreateRefTerm="InSourceResult,InTargetFilterSpec,InHitLimitSpec"))
	static ULxSkillUnitCreateAsyncAction* CreateScalingAreaEffectsAsync(
		UPARAM(DisplayName="技能对象") ULxSkill* InSkill,
		UPARAM(DisplayName="前置技能单元结果") const FLxSkillUnitResult& InSourceResult,
		UPARAM(DisplayName="技能子单元类型") TSubclassOf<ALxScalingAreaSkillUnitActor> InSkillUnitClass,
		UPARAM(DisplayName="创建参数") const FLxScalingAreaEffectCreateParams& InCreateParams,
		UPARAM(DisplayName="目标筛选参数") const FLxSkillTargetFilterSpec& InTargetFilterSpec,
		UPARAM(DisplayName="命中限制参数") const FLxSkillHitLimitSpec& InHitLimitSpec,
		UPARAM(DisplayName="单元创建位置") ELxSkillUnitResultSpawnLocationType InSpawnLocationType);

	/** 每次技能单元组命中目标或障碍物时执行，可在技能单元存续期间反复执行。 */
	UPROPERTY(BlueprintAssignable, Category="技能|技能单元创建|异步事件", DisplayName="技能命中")
	FOnLxAsyncSkillUnitEvent OnHitTarget;

	/** 整组技能单元全部结束且从未命中目标或障碍物时执行一次。 */
	UPROPERTY(BlueprintAssignable, Category="技能|技能单元创建|异步事件", DisplayName="技能失效")
	FOnLxAsyncSkillUnitEvent OnSkillInvalidated;

	/** 创建技能单元组、绑定事件并在绑定完成后自动激活。 */
	virtual void Activate() override;

private:
	/** 当前异步节点需要创建的技能单元类别。 */
	enum class EAsyncCreateType : uint8
	{
		StraightProjectile,
		ScalingAreaEffect
	};

	/** 创建一个异步节点对象并注册到技能对象所在的游戏实例。 */
	static ULxSkillUnitCreateAsyncAction* CreateAction(ULxSkill* InSkill,
		const FLxSkillUnitResult& InSourceResult, const FLxSkillTargetFilterSpec& InTargetFilterSpec,
		const FLxSkillHitLimitSpec& InHitLimitSpec, EAsyncCreateType InCreateType);

	/** 接收技能单元组的单次命中并转换为精简的蓝图输出参数。 */
	UFUNCTION()
	void HandleSkillUnitGroupHit(ULxSkillUnitGroup* InSkillUnitGroup,
		const FLxSkillUnitResult& InSkillUnitResult);

	/** 接收技能单元组完成事件，广播累计技能单元结果并结束异步节点。 */
	UFUNCTION()
	void HandleSkillUnitGroupFinished(ULxSkillUnitGroup* InSkillUnitGroup,
		const FLxSkillUnitResult& InSkillUnitResult);

	/** 解除当前技能单元组事件绑定。 */
	void UnbindSkillUnitGroupEvents();

	/** 广播一次空的技能失效结果并结束创建失败的异步节点。 */
	void FinishAsCreateFailed();

	/** 当前节点所属技能对象。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkill> Skill = nullptr;

	/** 当前节点监听的技能单元组。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxSkillUnitGroup> SkillUnitGroup = nullptr;

	/** 用于决定具体创建流程的内部类型。 */
	EAsyncCreateType CreateType = EAsyncCreateType::StraightProjectile;

	/** 供后续技能单元决定生成位置和朝向的前置结果。 */
	UPROPERTY(Transient)
	FLxSkillUnitResult SourceResult;

	/** 本次创建使用的目标筛选参数。 */
	UPROPERTY(Transient)
	FLxSkillTargetFilterSpec TargetFilterSpec;

	/** 本次创建使用的命中限制参数。 */
	UPROPERTY(Transient)
	FLxSkillHitLimitSpec HitLimitSpec;

	/** 使用前置技能单元结果创建本组单元时采用的位置来源。 */
	ELxSkillUnitResultSpawnLocationType SpawnLocationType = ELxSkillUnitResultSpawnLocationType::CasterLocation;

	/** 直线投射物蓝图类型。 */
	UPROPERTY(Transient)
	TSubclassOf<ALxStraightProjectileSkillUnitActor> StraightProjectileClass;

	/** 直线投射物创建参数。 */
	UPROPERTY(Transient)
	FLxProjectileSkillUnitCreateParams ProjectileCreateParams;

	/** 缩放型范围效果蓝图类型。 */
	UPROPERTY(Transient)
	TSubclassOf<ALxScalingAreaSkillUnitActor> ScalingAreaEffectClass;

	/** 缩放型范围效果创建参数。 */
	UPROPERTY(Transient)
	FLxScalingAreaEffectCreateParams ScalingAreaCreateParams;

};
