#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxCharacterStateComponent.generated.h"

/** 角色状态标签变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterStateTagsChanged, FGameplayTag, StateCategoryTag, const FGameplayTagContainer&, StateTags);

/**
 * 角色状态组件。
 *
 * 按状态大类保存多个 GameplayTag，用于表达燃烧、冰冻、移动、跳跃、瞄准等可并存状态。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色状态组件")
class LXARPG_API ULxCharacterStateComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色状态组件，并关闭 Tick。 */
	ULxCharacterStateComponent();

	/** 初始化角色状态组件。 */
	virtual void BaseComponentInitialize() override;

	/** 注册需要网络同步的状态标签容器。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 角色状态标签变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色|状态", DisplayName="角色状态标签变化事件")
	FOnLxCharacterStateTagsChanged OnStateTagsChanged;

	/** 获取指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="获取指定分类状态标签", meta=(Categories="角色状态"))
	bool GetStateTagsByCategory(FGameplayTag InStateCategoryTag, FGameplayTagContainer& OutStateTags) const;

	/** 设置指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="设置指定分类状态标签", meta=(Categories="角色状态"))
	bool SetStateTagsByCategory(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 添加指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="添加状态标签", meta=(Categories="角色状态"))
	bool AddStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 移除指定分类下的状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="移除状态标签", meta=(Categories="角色状态"))
	bool RemoveStateTag(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag);

	/** 检查角色是否拥有指定状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|状态", DisplayName="是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTag(FGameplayTag InStateTag) const;

	/** 检查指定分类下是否拥有状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色|状态", DisplayName="指定分类是否拥有状态标签", meta=(Categories="角色状态"))
	bool HasStateTagInCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const;

	/** 获取角色当前拥有的全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="获取全部状态标签")
	void GetAllStateTags(FGameplayTagContainer& OutStateTags) const;

	/** 清空指定分类下的全部状态标签。 */
	UFUNCTION(BlueprintCallable, Category="角色|状态", DisplayName="清空指定分类状态标签", meta=(Categories="角色状态"))
	bool ClearStateTagsByCategory(FGameplayTag InStateCategoryTag);

protected:
	/** 元素异常状态标签，例如燃烧、冰冻、中毒等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_ElementAbnormalStateTags, Category="角色|状态", DisplayName="元素异常状态标签", meta=(Categories="角色状态.元素异常状态"))
	FGameplayTagContainer ElementAbnormalStateTags;

	/** 生命周期状态标签，例如存活、死亡、复活保护等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_LifecycleStateTags, Category="角色|状态", DisplayName="生命周期状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTagContainer LifecycleStateTags;

	/** 角色移动状态标签，例如移动、跳跃、冲刺等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_MovementStateTags, Category="角色|状态", DisplayName="角色移动状态标签", meta=(Categories="角色状态.角色移动状态"))
	FGameplayTagContainer MovementStateTags;

	/** 战斗状态标签，例如瞄准、释放技能、蓄力等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_CombatStateTags, Category="角色|状态", DisplayName="战斗状态标签", meta=(Categories="角色状态.战斗状态"))
	FGameplayTagContainer CombatStateTags;

private:
	/** 根据状态分类标签查找对应状态容器。 */
	FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag);

	/** 根据状态分类标签查找对应只读状态容器。 */
	const FGameplayTagContainer* FindStateContainer(FGameplayTag InStateCategoryTag) const;

	/** 判断状态标签是否属于指定状态分类。 */
	bool IsStateTagAllowedForCategory(FGameplayTag InStateCategoryTag, FGameplayTag InStateTag) const;

	/** 广播指定分类的状态标签变化。 */
	void BroadcastStateTagsChanged(FGameplayTag InStateCategoryTag, const FGameplayTagContainer& InStateTags);

	/** 元素异常状态标签复制回调。 */
	UFUNCTION()
	void OnRep_ElementAbnormalStateTags();

	/** 生命周期状态标签复制回调。 */
	UFUNCTION()
	void OnRep_LifecycleStateTags();

	/** 角色移动状态标签复制回调。 */
	UFUNCTION()
	void OnRep_MovementStateTags();

	/** 战斗状态标签复制回调。 */
	UFUNCTION()
	void OnRep_CombatStateTags();
};
