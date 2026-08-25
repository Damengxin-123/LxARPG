#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxCharacterContentModuleBase.generated.h"

class AActor;
class ALxBaseCharacter;
class ULxCharacterContentComponent;

/** 角色内容模块通用数据变化事件。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxCharacterContentModuleDataChanged);

/**
 * 角色内容模块统一父类。
 *
 * 背包、装备、技能背包、Buff 和职业都作为该类型的 UObject 子类存在，
 * 由唯一的角色内容组件负责创建、初始化、网络注册和销毁。
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色内容模块基类")
class LXARPG_API ULxCharacterContentModuleBase : public UObject
{
	GENERATED_BODY()

public:
	/** 使用角色内容组件初始化当前模块；重复调用不会重复初始化业务数据。 */
	void InitializeModule(ULxCharacterContentComponent* InContentComponent);

	/** 关闭当前模块并释放计时器、事件绑定等运行时资源。 */
	void ShutdownModule();

	/** 获取承载全部内容模块的角色内容组件。 */
	ULxCharacterContentComponent* GetContentComponent() const { return ContentComponent; }

	/** 获取当前模块所属角色。 */
	ALxBaseCharacter* GetCharacterOwner() const { return CharacterOwner; }

	/** 获取当前模块所属 Actor，兼容原组件内部的 GetOwner 调用。 */
	AActor* GetOwner() const;

	/** 获取角色所在世界，使 UObject 模块能够使用计时器等世界功能。 */
	virtual UWorld* GetWorld() const override;

	/** 允许角色内容组件把该模块注册为网络复制子对象。 */
	virtual bool IsSupportedForNetworking() const override { return true; }

	/** 模块数据发生变化时广播。 */
	UPROPERTY(BlueprintAssignable, Category="角色内容|模块", DisplayName="模块数据变化事件")
	FOnLxCharacterContentModuleDataChanged OnDataChange;

protected:
	/** 子类的实际初始化入口。 */
	virtual void OnModuleInitialize() {}

	/** 子类的实际关闭入口。 */
	virtual void OnModuleShutdown() {}

	/** 当前模块所属角色内容组件。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxCharacterContentComponent> ContentComponent = nullptr;

	/** 当前模块所属角色。 */
	UPROPERTY(Transient)
	TObjectPtr<ALxBaseCharacter> CharacterOwner = nullptr;

private:
	/** 模块是否已经完成初始化。 */
	bool bModuleInitialized = false;
};
