#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxCharacterSpecialAttributeObject.h"
#include "LxCharacterLifecycleAttributeObject.generated.h"

/** 角色生命周期特殊属性业务对象，负责死亡与复活时的移动控制。 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色生命周期特殊属性")
class LXARPG_API ULxCharacterLifecycleAttributeObject : public ULxCharacterSpecialAttributeObject
{
	GENERATED_BODY()

public:
	/** 根据当前存活状态应用角色移动控制。 */
	void ApplyMovementControl(bool bInAlive) const;

	/** 获取存活状态标签。 */
	FGameplayTag GetAliveStateTag() const { return AliveStateTag; }

	/** 获取死亡状态标签。 */
	FGameplayTag GetDeadStateTag() const { return DeadStateTag; }

protected:
	/** 死亡时是否禁用角色移动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡时禁用移动")
	bool bDisableMovementWhenDead = true;

	/** 存活状态标签。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="存活状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag AliveStateTag;

	/** 死亡状态标签。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="角色|特殊属性|生命周期", DisplayName="死亡状态标签", meta=(Categories="角色状态.生命周期状态"))
	FGameplayTag DeadStateTag;

public:
	/** 创建生命周期特殊属性并设置默认状态标签。 */
	ULxCharacterLifecycleAttributeObject();
};
