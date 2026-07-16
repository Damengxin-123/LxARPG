#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterSpecialAttributeObject.generated.h"

class ALxBaseCharacter;
class ULxCharacterSpecialAttributeComponent;

/** 角色特殊属性业务对象基类，用于承载不适合拆成独立组件的零散业务。 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, DisplayName="角色特殊属性业务对象")
class LXARPG_API ULxCharacterSpecialAttributeObject : public UObject
{
	GENERATED_BODY()

public:
	/** 绑定所属特殊属性组件并初始化业务对象。 */
	virtual void InitializeSpecialAttributeObject(ULxCharacterSpecialAttributeComponent* InOwnerComponent);

	/** 解除业务对象与特殊属性组件的绑定。 */
	virtual void DeinitializeSpecialAttributeObject();

	/** 获取所属角色特殊属性组件。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性", DisplayName="获取特殊属性组件")
	ULxCharacterSpecialAttributeComponent* GetSpecialAttributeComponent() const { return OwnerComponent; }

	/** 获取所属角色。 */
	UFUNCTION(BlueprintPure, Category="角色|特殊属性", DisplayName="获取所属角色")
	ALxBaseCharacter* GetCharacterOwner() const;

protected:
	/** 当前业务对象所属的特殊属性组件。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="角色|特殊属性", DisplayName="所属特殊属性组件")
	TObjectPtr<ULxCharacterSpecialAttributeComponent> OwnerComponent;
};
