#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxFunctionPageInteractionComponent.generated.h"

/** 打开指定系统功能页面的交互模块。 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="功能界面交互模块")
class LXARPG_API ULxFunctionPageInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	/** 创建功能界面模块。 */
	ULxFunctionPageInteractionComponent();

	/** 应用功能节点提供的功能界面初始配置。 */
	void ApplyConfig(const FLxFunctionPageInteractionConfig& InConfig);

	/** 获取需要打开的功能页面。 */
	UFUNCTION(BlueprintPure, Category="交互|功能界面", DisplayName="获取功能页面ID")
	ELxFunctionPageID GetFunctionPageID() const { return FunctionPageID; }

	/** 获取资产指定的自定义功能页面类。 */
	UFUNCTION(BlueprintPure, Category="交互|功能界面", DisplayName="获取功能页面类")
	TSubclassOf<ULxUIBaseObject> GetPageWidgetClass() const { return PageWidgetClass; }

	/** 没有配置有效页面类的节点不显示，避免进入没有界面的功能状态。 */
	virtual bool CheckInteractionRequirement_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) const override;
	/** 在本地交互UI管理器成功创建页面后才进入交互中状态。 */
	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

    /** 查询本功能是否提供装备强化与锻造。 */
    UFUNCTION(BlueprintPure, Category="交互|装备打造", DisplayName="是否装备打造页面")
    bool IsEquipmentCraftingPage() const { return FunctionPageID == ELxFunctionPageID::EquipmentEnhancement; }
    /** 获取NPC当前配置的打造倍率。 */
    UFUNCTION(BlueprintPure, Category="交互|装备打造", DisplayName="获取装备打造加成")
    FLxEquipmentCraftService GetCraftService() const { return CraftService; }

private:
    /** 每个NPC功能节点拥有自己的倍率，1表示无加成。 */
    UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="交互|装备打造", DisplayName="装备打造加成", meta=(AllowPrivateAccess="true"))
    FLxEquipmentCraftService CraftService;
	/** 由资产指定的功能页面类，每次交互创建独立界面实例。 */
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category="交互|功能界面", DisplayName="功能页面类", meta=(AllowPrivateAccess="true"))
	TSubclassOf<ULxUIBaseObject> PageWidgetClass;

	/** 功能节点执行成功后需要打开的页面。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|功能界面", DisplayName="功能页面ID", meta=(AllowPrivateAccess="true"))
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};
