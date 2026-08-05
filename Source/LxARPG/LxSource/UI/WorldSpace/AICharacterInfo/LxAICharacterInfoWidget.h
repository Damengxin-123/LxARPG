#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxAICharacterInfoWidget.generated.h"

/** 放置在游戏场景中的AI角色信息界面，用于显示行为、生命值和受伤数字。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="AI角色信息界面")
class LXARPG_API ULxAICharacterInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 接收AI当前行为文本，并通知UI蓝图刷新行为显示。 */
	UFUNCTION(BlueprintCallable, Category="场景UI|AI角色信息", DisplayName="更新AI行为文本")
	void UpdateAIBehaviorText(const FString& InBehaviorText);

	/** 接收AI当前生命值百分比，并通知UI蓝图刷新生命值显示。 */
	UFUNCTION(BlueprintCallable, Category="场景UI|AI角色信息", DisplayName="更新AI生命值百分比")
	void UpdateAIHealthPercent(float InHealthPercent);

	/** 接收AI受到的伤害数值文本，并通知UI蓝图显示本次伤害。 */
	UFUNCTION(BlueprintCallable, Category="场景UI|AI角色信息", DisplayName="显示AI受到的伤害文本")
	void ShowAIReceivedDamageText(const FString& InDamageText);

protected:
	/** 行为文本更新时由UI蓝图实现具体文本控件刷新。 */
	UFUNCTION(BlueprintImplementableEvent, Category="场景UI|AI角色信息|事件", DisplayName="AI行为文本更新事件")
	void OnAIBehaviorTextUpdated(const FString& BehaviorText);

	/** 生命值百分比更新时由UI蓝图实现进度条或文本刷新。 */
	UFUNCTION(BlueprintImplementableEvent, Category="场景UI|AI角色信息|事件", DisplayName="AI生命值百分比更新事件")
	void OnAIHealthPercentUpdated(float HealthPercent);

	/** 收到伤害文本时由UI蓝图实现飘字、日志或固定文本显示。 */
	UFUNCTION(BlueprintImplementableEvent, Category="场景UI|AI角色信息|事件", DisplayName="AI受到伤害文本事件")
	void OnAIReceivedDamageText(const FString& DamageText);
};
