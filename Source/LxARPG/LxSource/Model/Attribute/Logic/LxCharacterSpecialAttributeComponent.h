#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxSpecialAttributeTypes.h"
#include "LxCharacterSpecialAttributeComponent.generated.h"

class ULxCharacterLifecycleAttributeObject;
class ULxCharacterSpecialAttributeObject;
class ALxBaseCharacter;

/** 旧版角色特殊属性组件，仅为已保存资产保留；新角色统一使用角色属性组件。 */
UCLASS(ClassGroup=(Custom), NotBlueprintable, DisplayName="旧版角色特殊属性组件")
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

	/** 根据目标阵营标签集合判断友方、敌方或中立关系；友方与敌方配置重叠时优先视为友方。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|阵营", DisplayName="判断阵营关系")
	ELxCharacterFactionRelation GetFactionRelation(const FGameplayTagContainer& InTargetFactionTags) const;

	/** 使用双方角色特殊属性组件中的阵营标签判断目标角色关系。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性|阵营", DisplayName="判断目标角色阵营关系")
	ELxCharacterFactionRelation GetCharacterFactionRelation(const ALxBaseCharacter* InTargetCharacter) const;

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

	/** 当前角色用于判断友方、敌对方和其他中立方的阵营标签配置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="角色|特殊属性|阵营", DisplayName="角色阵营")
	FLxCharacterFactionData CharacterFaction;

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
