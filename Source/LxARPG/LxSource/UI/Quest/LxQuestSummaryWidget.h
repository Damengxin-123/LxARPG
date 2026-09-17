#pragma once

#include "CoreMinimal.h"
#include "LxQuestWidgetBase.h"
#include "LxQuestSummaryWidget.generated.h"

class ULxQuestSummaryViewData;

/** 游戏 HUD 中常驻的任务简要界面，无开启按键。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="任务简要界面")
class LXARPG_API ULxQuestSummaryWidget : public ULxQuestWidgetBase
{
	GENERATED_BODY()
public:
	/** 蓝图将数据交给使用任务简要列表项控件的 ListView。 */
	UFUNCTION(BlueprintImplementableEvent, Category="任务UI|简要", DisplayName="任务简要列表更新")
	void OnQuestSummaryUpdated(const TArray<ULxQuestSummaryViewData*>& Quests);
protected:
	/** 排除已完成任务，向蓝图发送新的简要列表。 */
	virtual void RefreshQuestDisplay() override;
private:
	/** 维持当前列表数据的生命周期。 */
	UPROPERTY(Transient)
	TArray<ULxQuestSummaryViewData*> SummaryItems;
};
