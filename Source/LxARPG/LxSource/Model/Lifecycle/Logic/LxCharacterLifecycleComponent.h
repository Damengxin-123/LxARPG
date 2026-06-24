#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxCharacterLifecycleComponent.generated.h"

class ALxBaseCharacter;
class ULxCharacterStateComponent;

/** 角色生命周期状态变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxCharacterLifecycleStateChanged, bool, bIsAlive, FGameplayTag, LifecycleStateTag);

/**
 * 角色生命周期组件。
 *
 * 负责管理角色存活、死亡等生命周期状态，并将状态同步到角色状态组件。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色生命周期组件")
class LXARPG_API ULxCharacterLifecycleComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色生命周期组件。 */
	ULxCharacterLifecycleComponent();

	virtual void BaseComponentInitialize() override;

	/** 判断角色当前是否存活。 */
	UFUNCTION(BlueprintPure, Category="角色生命周期", DisplayName="角色是否存活")
	bool IsCharacterAlive() const { return bIsAlive; }

	/** 将角色切换为存活状态。 */
	UFUNCTION(BlueprintCallable, Category="角色生命周期", DisplayName="设置角色存活")
	void SetCharacterAlive();

	/** 将角色切换为死亡状态。 */
	UFUNCTION(BlueprintCallable, Category="角色生命周期", DisplayName="设置角色死亡")
	void SetCharacterDead();

	/** 设置角色生命周期状态。 */
	UFUNCTION(BlueprintCallable, Category="角色生命周期", DisplayName="设置角色生命周期状态")
	void SetCharacterAliveState(bool bInAlive);

	/** 获取当前生命周期状态标签。 */
	UFUNCTION(BlueprintPure, Category="角色生命周期", DisplayName="获取当前生命周期状态标签")
	FGameplayTag GetCurrentLifecycleStateTag() const;

	/** 角色生命周期状态变化事件。 */
	UPROPERTY(BlueprintAssignable, Category="角色生命周期", DisplayName="角色生命周期状态变化事件")
	FOnLxCharacterLifecycleStateChanged OnLifecycleStateChanged;

protected:
	/** 死亡时是否禁用角色移动。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色生命周期", DisplayName="死亡时禁用移动")
	bool bDisableMovementWhenDead = true;

	/** 当前角色是否存活。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="角色生命周期", DisplayName="角色是否存活")
	bool bIsAlive = true;

	/** 存活状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色生命周期", DisplayName="存活状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag AliveStateTag;

	/** 死亡状态标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色生命周期", DisplayName="死亡状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag DeadStateTag;

private:
	void CacheOwnerComponents();
	void SyncLifecycleStateToStateComponent();
	void ApplyMovementControlByLifecycle();

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> OwnerCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<ULxCharacterStateComponent> StateComponent = nullptr;
};
