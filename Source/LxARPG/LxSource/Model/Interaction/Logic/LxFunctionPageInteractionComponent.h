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

	virtual bool ExecuteInteraction_Implementation(ULxPlayerInteractionModule* PlayerInteractionComponent) override;

private:
	/** 功能节点执行成功后需要打开的页面。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="交互|功能界面", DisplayName="功能页面ID", meta=(AllowPrivateAccess="true"))
	ELxFunctionPageID FunctionPageID = ELxFunctionPageID::EquipmentEnhancement;
};
