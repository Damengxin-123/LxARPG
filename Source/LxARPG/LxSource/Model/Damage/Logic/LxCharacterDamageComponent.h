#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxCharacterDamageComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxCharacterLifecycleComponent;
class ULxDamageCalculationFlow;

/** 角色受到伤害后的实际承受结果事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterDamageReceived, const FLxDamageReceiveResult&, DamageReceiveResult, AActor*, AttackerActor);

/** 角色伤害计算组件，负责伤害输出计算和伤害接收计算。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色伤害计算组件")
class LXARPG_API ULxCharacterDamageComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色伤害计算组件。 */
	ULxCharacterDamageComponent();

	virtual void BaseComponentInitialize() override;

	/** 构建当前角色对目标的输出伤害效果包。 */
	UFUNCTION(BlueprintCallable, Category="角色伤害", DisplayName="构建输出伤害效果包")
	bool BuildOutgoingDamagePackage(AActor* TargetActor, FLxEffectPackage& OutDamagePackage);

	/** 基于已有的效果包计算当前角色对目标的输出伤害。 */
	UFUNCTION(BlueprintCallable, Category="角色伤害", DisplayName="基于效果包构建输出伤害")
	bool BuildOutgoingDamagePackageFromEffectPackage(const FLxEffectPackage& InSourceEffectPackage, AActor* TargetActor, FLxEffectPackage& OutDamagePackage);

	/** 接收伤害效果包，计算最终承受结果并按需应用到属性组件。 */
	UFUNCTION(BlueprintCallable, Category="角色伤害", DisplayName="接收伤害效果包")
	bool ReceiveIncomingDamagePackage(const FLxEffectPackage& InDamagePackage, FLxDamageReceiveResult& OutDamageReceiveResult, bool bApplyResult = true);

	/** 获取伤害计算流程。 */
	UFUNCTION(BlueprintPure, Category="角色伤害", DisplayName="获取伤害计算流程")
	ULxDamageCalculationFlow* GetDamageCalculationFlow() const { return DamageCalculationFlow; }

	/** 角色受到伤害后的实际承受结果事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色伤害", DisplayName="受到伤害事件")
	FOnLxCharacterDamageReceived OnCharacterDamageReceived;

protected:
	/** 当前角色运行时使用的伤害计算流程实例，由游戏设置中的全局流程类型创建。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="角色伤害", DisplayName="运行时伤害计算流程")
	TObjectPtr<ULxDamageCalculationFlow> DamageCalculationFlow;

private:
	void CacheOwnerComponents();
	void EnsureDamageCalculationFlow();
	void RefreshLifecycleAfterDamage();

	/** 将最终承伤结果应用到当前角色属性。 */
	void ApplyDamageReceiveResultToTarget(const FLxDamageReceiveResult& InDamageReceiveResult);

	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> DataTransferComponent = nullptr;

	UPROPERTY()
	TObjectPtr<ULxCharacterLifecycleComponent> LifecycleComponent = nullptr;
};
