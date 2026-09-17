#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxQuestSummaryListItemWidget.generated.h"

/** 专供任务简要 ListView 使用的条目，子类蓝图显示名称和完成状态。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="任务简要列表项控件")
class LXARPG_API ULxQuestSummaryListItemWidget : public ULxUIBaseObject, public IUserObjectListEntry
{
	GENERATED_BODY()
public:
	/** 将虚拟列表分配的数据发送给蓝图。 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	/** 归还复用池时清空条目显示。 */
	virtual void NativeOnEntryReleased() override;
protected:
	/** 无有效数据时名称为空、状态为未接取。 */
	UFUNCTION(BlueprintImplementableEvent, Category="任务UI|简要", DisplayName="任务简要条目更新")
	void OnQuestSummaryItemUpdated(const FText& QuestName, ELxQuestRuntimeState State);
};
