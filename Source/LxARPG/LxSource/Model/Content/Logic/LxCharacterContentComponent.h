#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxCharacterContentComponent.generated.h"

class ULxCharacterBackpackModule;
class ULxCharacterBuffModule;
class ULxCharacterEquipmentModule;
class ULxCharacterProfessionModule;
class ULxSkillBackpackModule;

/**
 * 角色内容组件。
 *
 * 角色只挂载该组件；背包、装备、技能背包、Buff 和职业作为独立 UObject 模块由它统一持有。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色内容组件")
class LXARPG_API ULxCharacterContentComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建角色内容组件及五个默认内容模块。 */
	ULxCharacterContentComponent();

	/** 按固定顺序初始化全部内容模块并注册网络复制子对象。 */
	virtual void BaseComponentInitialize() override;

	/** 组件结束时关闭全部内容模块。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取背包模块。 */
	UFUNCTION(BlueprintPure, Category="角色|内容", DisplayName="获取背包模块")
	ULxCharacterBackpackModule* GetBackpackModule() const { return BackpackModule; }

	/** 获取装备模块。 */
	UFUNCTION(BlueprintPure, Category="角色|内容", DisplayName="获取装备模块")
	ULxCharacterEquipmentModule* GetEquipmentModule() const { return EquipmentModule; }

	/** 获取技能背包模块。 */
	UFUNCTION(BlueprintPure, Category="角色|内容", DisplayName="获取技能背包模块")
	ULxSkillBackpackModule* GetSkillBackpackModule() const { return SkillBackpackModule; }

	/** 获取Buff模块。 */
	UFUNCTION(BlueprintPure, Category="角色|内容", DisplayName="获取Buff模块")
	ULxCharacterBuffModule* GetBuffModule() const { return BuffModule; }

	/** 获取职业模块。 */
	UFUNCTION(BlueprintPure, Category="角色|内容", DisplayName="获取职业模块")
	ULxCharacterProfessionModule* GetProfessionModule() const { return ProfessionModule; }

	/** 服务端批量添加背包物品。 */
	UFUNCTION(Server, Reliable)
	void ServerAddBackpackItems(const TArray<FLxItemQuote>& InItems);

protected:
	/** 背包功能模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|内容", DisplayName="背包模块")
	TObjectPtr<ULxCharacterBackpackModule> BackpackModule;

	/** 装备功能模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|内容", DisplayName="装备模块")
	TObjectPtr<ULxCharacterEquipmentModule> EquipmentModule;

	/** 技能背包功能模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|内容", DisplayName="技能背包模块")
	TObjectPtr<ULxSkillBackpackModule> SkillBackpackModule;

	/** Buff功能模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|内容", DisplayName="Buff模块")
	TObjectPtr<ULxCharacterBuffModule> BuffModule;

	/** 职业功能模块。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Instanced, Category="角色|内容", DisplayName="职业模块")
	TObjectPtr<ULxCharacterProfessionModule> ProfessionModule;

private:
	/** 注册需要由该组件复制的 UObject 内容模块。 */
	void RegisterReplicatedModules();

	/** 角色内容组件是否已经初始化。 */
	bool bContentInitialized = false;
};
