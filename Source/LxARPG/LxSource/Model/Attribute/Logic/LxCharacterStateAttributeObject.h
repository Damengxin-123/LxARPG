#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxCharacterSpecialAttributeObject.h"
#include "LxCharacterStateAttributeObject.generated.h"

/** 角色状态属性对象，保存并同步各业务分类的状态标签。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色状态属性")
class LXARPG_API ULxCharacterStateAttributeObject : public ULxCharacterSpecialAttributeObject
{
	GENERATED_BODY()

public:
	/** 注册状态标签的网络复制字段。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 获取指定分类下的状态标签。 */
	bool GetStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const;

	/** 设置指定分类下的状态标签。 */
	bool SetStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 添加状态标签。 */
	bool AddStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 移除状态标签。 */
	bool RemoveStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 判断是否拥有指定状态标签。 */
	bool HasStateTag(FGameplayTag InStateTag) const;

	/** 判断指定分类下是否拥有状态标签。 */
	bool HasStateTagInCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const;

	/** 获取全部状态标签。 */
	void GetAllStateTags(FGameplayTagContainer& OutStateTags) const;

	/** 清空指定分类下的全部状态标签。 */
	bool ClearStateTagsByCategory(FGameplayTag InStateCategoryTag);

private:
	/** 按分类查找状态标签容器。 */
	FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag);

	/** 按分类查找只读状态标签容器。 */
	const FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag) const;

	/** 通知统一属性组件状态标签已经变化。 */
	void NotifyStateTagsChanged(FGameplayTag InStateCategoryTag) const;

	/** 元素异常状态标签复制回调。 */
	UFUNCTION(Category="角色|属性|状态|网络", DisplayName="同步元素异常状态")
	void OnRep_ElementAbnormalStateTags();

	/** 生命周期状态标签复制回调。 */
	UFUNCTION(Category="角色|属性|状态|网络", DisplayName="同步生命周期状态")
	void OnRep_LifecycleStateTags();

	/** 移动状态标签复制回调。 */
	UFUNCTION(Category="角色|属性|状态|网络", DisplayName="同步移动状态")
	void OnRep_MovementStateTags();

	/** 战斗状态标签复制回调。 */
	UFUNCTION(Category="角色|属性|状态|网络", DisplayName="同步战斗状态")
	void OnRep_CombatStateTags();

	/** 元素异常状态标签。 */
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_ElementAbnormalStateTags, Category="角色|属性|状态", DisplayName="元素异常状态标签", meta=(Categories="角色状态.元素异常状态"))
	FGameplayTagContainer ElementAbnormalStateTags;

	/** 生命周期状态标签。 */
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_LifecycleStateTags, Category="角色|属性|状态", DisplayName="生命周期状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTagContainer LifecycleStateTags;

	/** 角色移动状态标签。 */
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_MovementStateTags, Category="角色|属性|状态", DisplayName="角色移动状态标签", meta=(Categories="角色状态.角色移动状态"))
	FGameplayTagContainer MovementStateTags;

	/** 战斗状态标签。 */
	UPROPERTY(EditAnywhere, ReplicatedUsing=OnRep_CombatStateTags, Category="角色|属性|状态", DisplayName="战斗状态标签", meta=(Categories="角色状态.战斗状态"))
	FGameplayTagContainer CombatStateTags;
};
