#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Damage/DataType/LxDamageCalculationTypes.h"
#include "LxCharacterEffectComponent.generated.h"

class ULxCharacterEffectCacheModule;
class ULxCharacterEffectProcessModule;
class ULxCharacterEffectTransferModule;

/** 角色效果组件转发的实际承伤结果事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterEffectDamageReceived, const FLxDamageReceiveResult&, DamageReceiveResult, AActor*, AttackerActor);

/**
 * 角色效果组件。
 *
 * 角色只挂载该组件；效果处理、效果缓存和效果传递作为独立 UObject 模块由它统一持有。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色效果组件")
class LXARPG_API ULxCharacterEffectComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色效果组件及三个默认效果模块。 */
	ULxCharacterEffectComponent();

	/** 按固定顺序初始化全部效果模块。 */
	virtual void BaseComponentInitialize() override;

	/** 组件结束时关闭全部效果模块。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取效果处理模块。 */
	UFUNCTION(BlueprintPure, Category="角色|效果", DisplayName="获取效果处理模块")
	ULxCharacterEffectProcessModule* GetProcessModule() const { return ProcessModule; }

	/** 获取效果缓存模块。 */
	UFUNCTION(BlueprintPure, Category="角色|效果", DisplayName="获取效果缓存模块")
	ULxCharacterEffectCacheModule* GetCacheModule() const { return CacheModule; }

	/** 获取效果传递模块。 */
	UFUNCTION(BlueprintPure, Category="角色|效果", DisplayName="获取效果传递模块")
	ULxCharacterEffectTransferModule* GetTransferModule() const { return TransferModule; }

	/** 角色受到伤害后的实际承受结果事件，由效果处理模块统一转发。 */
	UPROPERTY(BlueprintAssignable, Category="角色|效果", DisplayName="受到伤害事件")
	FOnLxCharacterEffectDamageReceived OnCharacterDamageReceived;

protected:
	/** 负责解析技能词条、计算伤害并生成最终效果包的模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|效果", DisplayName="效果处理模块")
	TObjectPtr<ULxCharacterEffectProcessModule> ProcessModule;

	/** 负责缓存可撤回持续效果并刷新实际生效模块的模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|效果", DisplayName="效果缓存模块")
	TObjectPtr<ULxCharacterEffectCacheModule> CacheModule;

	/** 负责角色之间最终效果包发送与接收的模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|效果", DisplayName="效果传递模块")
	TObjectPtr<ULxCharacterEffectTransferModule> TransferModule;

private:
	/** 接收效果处理模块的承伤事件并通过统一组件入口转发。 */
	UFUNCTION()
	void HandleCharacterDamageReceived(const FLxDamageReceiveResult& DamageReceiveResult, AActor* AttackerActor);

	/** 角色效果组件是否已经初始化。 */
	bool bEffectInitialized = false;
};
