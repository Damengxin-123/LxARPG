#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterCombatModuleBase.generated.h"

class ALxBaseCharacter;
class ULxCharacterCombatComponent;

/** 角色战斗 UObject 模块基类，提供统一组件、角色和世界访问。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色战斗模块基类")
class LXARPG_API ULxCharacterCombatModuleBase : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定统一角色战斗组件并初始化模块。 */
	virtual void InitializeModule(ULxCharacterCombatComponent* InOwnerComponent);

	/** 解除模块运行期资源与统一组件绑定。 */
	virtual void ShutdownModule();

	/** 获取统一角色战斗组件。 */
	UFUNCTION(BlueprintPure, Category="角色|战斗", DisplayName="获取角色战斗组件")
	ULxCharacterCombatComponent* GetCombatComponent() const { return OwnerComponent; }

	/** 获取模块所属角色。 */
	UFUNCTION(BlueprintPure, Category="角色|战斗", DisplayName="获取战斗模块所属角色")
	ALxBaseCharacter* GetCharacterOwner() const;

	/** 获取模块所属 Actor。 */
	AActor* GetOwner() const;

	/** 获取模块使用的世界。 */
	virtual UWorld* GetWorld() const override;

	/** 允许战斗模块作为统一组件的网络复制子对象。 */
	virtual bool IsSupportedForNetworking() const override { return true; }

protected:
	/** 通知统一角色战斗组件模块数据已经变化。 */
	void BroadcastModuleDataChanged() const;

	/** 当前模块所属的统一角色战斗组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|战斗", DisplayName="所属角色战斗组件")
	TObjectPtr<ULxCharacterCombatComponent> OwnerComponent;
};
