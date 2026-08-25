#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterEffectModuleBase.generated.h"

class AActor;
class ALxBaseCharacter;
class ULxCharacterEffectComponent;

/**
 * 角色效果模块统一父类。
 *
 * 效果处理、效果缓存和效果传递均作为 UObject 模块，由唯一的角色效果组件负责创建、初始化和关闭。
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色效果模块基类")
class LXARPG_API ULxCharacterEffectModuleBase : public UObject
{
	GENERATED_BODY()

public:
	/** 使用角色效果组件初始化当前模块；重复调用不会重复初始化业务数据。 */
	void InitializeModule(ULxCharacterEffectComponent* InEffectComponent);

	/** 关闭当前模块并释放运行时资源。 */
	void ShutdownModule();

	/** 获取承载全部效果模块的角色效果组件。 */
	ULxCharacterEffectComponent* GetEffectComponent() const { return EffectComponent; }

	/** 获取当前模块所属角色。 */
	ALxBaseCharacter* GetCharacterOwner() const { return CharacterOwner; }

	/** 获取当前模块所属 Actor，兼容原组件内部的 GetOwner 调用。 */
	AActor* GetOwner() const;

	/** 获取角色所在世界，使 UObject 模块能够使用世界相关功能。 */
	virtual UWorld* GetWorld() const override;

protected:
	/** 子类的实际初始化入口。 */
	virtual void OnModuleInitialize() {}

	/** 子类的实际关闭入口。 */
	virtual void OnModuleShutdown() {}

	/** 当前模块所属角色效果组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterEffectComponent> EffectComponent = nullptr;

	/** 当前模块所属角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxBaseCharacter> CharacterOwner = nullptr;

private:
	/** 模块是否已经完成初始化。 */
	bool bModuleInitialized = false;
};
