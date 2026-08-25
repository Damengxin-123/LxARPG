#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterSpecialAttributeObject.generated.h"

class ALxBaseCharacter;
class ULxCharacterAttributeComponent;

/** 角色特殊属性业务对象基类，用于承载不适合拆成独立组件的零散业务。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色特殊属性业务对象")
class LXARPG_API ULxCharacterSpecialAttributeObject : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定所属统一属性组件并初始化属性对象。 */
	virtual void InitializeSpecialAttributeObject(ULxCharacterAttributeComponent* InOwnerComponent);

	/** 解除业务对象与特殊属性组件的绑定。 */
	virtual void DeinitializeSpecialAttributeObject();

	/** 获取所属统一角色属性组件。 */
	UFUNCTION(BlueprintPure, Category="角色|属性", DisplayName="获取角色属性组件")
	ULxCharacterAttributeComponent* GetAttributeComponent() const { return OwnerComponent; }

	/** 获取所属角色。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性", DisplayName="获取所属角色")
	ALxBaseCharacter* GetCharacterOwner() const;

protected:
	/** 当前属性对象所属的统一角色属性组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|属性", DisplayName="所属角色属性组件")
	TObjectPtr<ULxCharacterAttributeComponent> OwnerComponent;

public:
	/** 允许属性 UObject 作为角色属性组件的网络复制子对象。 */
	virtual bool IsSupportedForNetworking() const override { return true; }
};
