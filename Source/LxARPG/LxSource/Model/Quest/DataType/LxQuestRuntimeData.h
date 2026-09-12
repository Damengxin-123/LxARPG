#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxQuestRuntimeData.generated.h"

/** 玩家持有的单个任务运行状态。 */
UENUM(BlueprintType)
enum class ELxQuestRuntimeState : uint8
{
	/** 玩家尚未接取任务，通常不会为其保存运行时记录。 */
	NotAccepted UMETA(DisplayName="未接取"),
	/** 玩家已经接取任务，但尚未满足提交条件。 */
	InProgress UMETA(DisplayName="进行中"),
	/** 玩家已经满足当前任务的提交条件。 */
	ReadyToSubmit UMETA(DisplayName="可提交"),
	/** 玩家已经提交并永久完成当前任务。 */
	Completed UMETA(DisplayName="已完成"),
};

/** 玩家任务模块保存和复制的单个任务运行时记录。 */
USTRUCT(BlueprintType, DisplayName="任务运行时记录")
struct LXARPG_API FLxQuestRuntimeRecord
{
	GENERATED_BODY()

	/** 任务所属系列的标签ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|运行时", DisplayName="任务系列ID", meta=(Categories="任务"))
	FGameplayTag QuestSeriesId;

	/** 当前任务的标签ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|运行时", DisplayName="任务ID", meta=(Categories="任务"))
	FGameplayTag QuestId;

	/** 当前任务在玩家角色上的运行状态。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="任务|运行时", DisplayName="任务状态")
	ELxQuestRuntimeState State = ELxQuestRuntimeState::NotAccepted;
};
