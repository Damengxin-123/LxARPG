#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxQuestSummaryViewData.generated.h"

/** 任务简要 ListView 的独立数据对象。 */
UCLASS(BlueprintType, DisplayName="任务简要视图数据")
class LXARPG_API ULxQuestSummaryViewData : public UObject
{
	GENERATED_BODY()
public:
	/** 玩家可见的任务名称。 */
	UPROPERTY(BlueprintReadOnly, Category="任务UI|简要", DisplayName="任务名称")
	FText QuestName;
	/** 当前完成状态；简要界面只传入可提交或进行中。 */
	UPROPERTY(BlueprintReadOnly, Category="任务UI|简要", DisplayName="任务状态")
	ELxQuestRuntimeState State = ELxQuestRuntimeState::NotAccepted;
};
