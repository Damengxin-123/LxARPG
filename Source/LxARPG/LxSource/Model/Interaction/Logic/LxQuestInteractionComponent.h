#pragma once

#include "CoreMinimal.h"
#include "LxInteractionActionComponentBase.h"
#include "LxQuestInteractionComponent.generated.h"

class ULxCharacterDataTransferComponent;
class ULxPlayerInteractionModule;

/** NPC任务交互功能模块，根据玩家任务状态自动执行接取或提交。 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="任务交互模块")
class LXARPG_API ULxQuestInteractionComponent : public ULxInteractionActionComponentBase
{
	GENERATED_BODY()

public:
	/** 创建不打开独立功能界面的即时任务交互模块。 */
	ULxQuestInteractionComponent();

	/** 应用所属任务节点独立保存的任务配置。 */
	void ApplyConfig(const FLxQuestInteractionConfig& InConfig);

	/** 按配置读取任务可视化名称或自定义文本；任务名称在显示时查询以避免初始化时序问题。 */
	virtual FText GetPromptText() const override;

	/** 获取当前节点使用的任务交互配置。 */
	UFUNCTION(BlueprintPure, Category="交互|任务", DisplayName="获取任务交互配置")
	FLxQuestInteractionConfig GetQuestInteractionConfig() const { return QuestConfig; }

	virtual bool CheckInteractionRequirement_Implementation(
		ULxPlayerInteractionModule* PlayerInteractionComponent) const override;
	virtual bool ExecuteInteraction_Implementation(
		ULxPlayerInteractionModule* PlayerInteractionComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** 从交互发起者获取统一的角色数据中转组件。 */
	static ULxCharacterDataTransferComponent* ResolveDataTransferComponent(
		ULxPlayerInteractionModule* PlayerInteractionComponent);

	/** 在存在游戏实例静态数据服务时验证任务系列和任务节点确实已经登记。 */
	bool IsStaticQuestConfigurationAvailable() const;

	/** 当前功能节点独立引用的任务系列和任务ID。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Replicated, Category="交互|任务",
		DisplayName="任务交互配置", meta=(AllowPrivateAccess="true"))
	FLxQuestInteractionConfig QuestConfig;
};
