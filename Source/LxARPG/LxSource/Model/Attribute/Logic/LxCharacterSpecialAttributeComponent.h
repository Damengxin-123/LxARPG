#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxCharacterSpecialAttributeComponent.generated.h"

class ULxCharacterLifecycleAttributeObject;
class ULxCharacterSpecialAttributeObject;

/** 特殊属性状态标签变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSpecialAttributeStateTagsChanged, FGameplayTag, StateCategoryTag, const FGameplayTagContainer&, StateTags);

/** 特殊属性生命周期状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxSpecialAttributeLifecycleChanged, bool, bIsAlive, FGameplayTag, LifecycleStateTag);

/** 角色特殊属性组件，用一个容器组件管理状态数据和轻量 UObject 业务对象。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色特殊属性组件")
class LXARPG_API ULxCharacterSpecialAttributeComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建特殊属性组件并注册默认业务对象类型。 */
	ULxCharacterSpecialAttributeComponent();

	/** 创建业务对象并初始化特殊属性状态。 */
	virtual void BaseComponentInitialize() override;

	/** 释放运行时业务对象。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 注册需要网络同步的特殊属性快照。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 获取所属角色。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性", DisplayName="获取特殊属性所属角色")
	ALxBaseCharacter* GetCharacterOwner() const { return ULxCharacterComponentBase::GetCharacterOwner(); }

	/** 获取指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="获取指定分类状态标签", meta=(Categories="角色状态"))
	bool GetStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const;

	/** 设置指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="设置指定分类状态标签", meta=(Categories="角色状态"))
	bool SetStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 添加状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="添加状态标签", meta=(Categories="角色状态"))
	bool AddStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 移除状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="移除状态标签", meta=(Categories="角色状态"))
	bool RemoveStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 判断是否拥有指定状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|状态", DisplayName="是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTag(FGameplayTag InStateTag) const;

	/** 判断指定分类下是否拥有状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|状态", DisplayName="指定分类是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTagInCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const;

	/** 获取全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="获取全部状态标签")
	void GetAllStateTags(FGameplayTagContainer& OutStateTags) const;

	/** 清空指定分类下的全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|状态", DisplayName="清空指定分类状态标签", meta=(Categories="角色状态"))
	bool ClearStateTagsByCategory(FGameplayTag InStateCategoryTag);

	/** 判断角色当前是否存活。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|生命周期", DisplayName="角色是否存活")
	bool IsCharacterAlive() const { return bIsAlive; }

	/** 设置角色存活。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|生命周期", DisplayName="设置角色存活")
	void SetCharacterAlive();

	/** 设置角色死亡。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|生命周期", DisplayName="设置角色死亡")
	void SetCharacterDead();

	/** 设置角色生命周期状态。 */
	UFUNCTION(BlueprintCallable, Category="角色|特殊属性|生命周期", DisplayName="设置角色生命周期状态")
	void SetCharacterAliveState(bool bInAlive);

	/** 获取当前生命周期状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|生命周期", DisplayName="获取当前生命周期状态标签")
	FGameplayTag GetCurrentLifecycleStateTag() const;

	/** 按类型查询运行时特殊属性业务对象。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性", DisplayName="查询特殊属性业务对象", meta=(DeterminesOutputType="InObjectClass"))
	ULxCharacterSpecialAttributeObject* FindSpecialAttributeObject(TSubclassOf<ULxCharacterSpecialAttributeObject> InObjectClass) const;

	/** 状态标签变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|特殊属性|状态", DisplayName="状态标签变化事件")
	FOnLxSpecialAttributeStateTagsChanged OnStateTagsChanged;

	/** 生命周期状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|特殊属性|生命周期", DisplayName="生命周期状态变化事件")
	FOnLxSpecialAttributeLifecycleChanged OnLifecycleStateChanged;

protected:
	/** 需要创建的特殊属性业务对象类型列表。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性", DisplayName="特殊属性业务对象类型")
	TArray<TSubclassOf<ULxCharacterSpecialAttributeObject>> SpecialAttributeObjectClasses;

	/** 当前运行时创建的特殊属性业务对象。 */
	UPROPERTY(Transient, VisibleAnywhere, Instanced, BlueprintReadOnly, Category="角色|特殊属性", DisplayName="运行时特殊属性业务对象")
	TArray<TObjectPtr<ULxCharacterSpecialAttributeObject>> RuntimeSpecialAttributeObjects;

	/** 元素异常状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ElementAbnormalStateTags, Category="角色|特殊属性|状态", DisplayName="元素异常状态标签", meta=(Categories="角色状态.元素异常状态"))
	FGameplayTagContainer ElementAbnormalStateTags;

	/** 生命周期状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_LifecycleStateTags, Category="角色|特殊属性|状态", DisplayName="生命周期状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTagContainer LifecycleStateTags;

	/** 角色移动状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_MovementStateTags, Category="角色|特殊属性|状态", DisplayName="角色移动状态标签", meta=(Categories="角色状态.角色移动状态"))
	FGameplayTagContainer MovementStateTags;

	/** 战斗状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_CombatStateTags, Category="角色|特殊属性|状态", DisplayName="战斗状态标签", meta=(Categories="角色状态.战斗状态"))
	FGameplayTagContainer CombatStateTags;

	/** 当前角色是否存活。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_IsAlive, Category="角色|特殊属性|生命周期", DisplayName="角色是否存活")
	bool bIsAlive = true;

private:
	/** 创建配置的运行时业务对象。 */
	void CreateRuntimeSpecialAttributeObjects();

	/** 获取生命周期业务对象。 */
	ULxCharacterLifecycleAttributeObject* GetLifecycleAttributeObject() const;

	/** 按分类查找状态标签容器。 */
	FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag);

	/** 按分类查找只读状态标签容器。 */
	const FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag) const;

	/** 广播指定分类状态标签变化。 */
	void BroadcastStateTagsChanged(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 同步生命周期标签并应用生命周期业务。 */
	void ApplyLifecycleState();

	/** 元素异常状态标签复制回调。 */
	UFUNCTION()
	void OnRep_ElementAbnormalStateTags();

	/** 生命周期状态标签复制回调。 */
	UFUNCTION()
	void OnRep_LifecycleStateTags();

	/** 移动状态标签复制回调。 */
	UFUNCTION()
	void OnRep_MovementStateTags();

	/** 战斗状态标签复制回调。 */
	UFUNCTION()
	void OnRep_CombatStateTags();

	/** 存活状态复制回调。 */
	UFUNCTION()
	void OnRep_IsAlive();
};
