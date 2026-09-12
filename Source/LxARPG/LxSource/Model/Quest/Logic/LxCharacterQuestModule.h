#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Quest/DataType/LxQuestRuntimeData.h"
#include "LxCharacterQuestModule.generated.h"

/** 玩家任务进度发生变化时触发，供数据中转、交互和任务界面刷新。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxQuestProgressChanged);

/** 角色内容组件持有的任务模块，负责保存任务运行状态和执行基础状态迁移。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色任务模块")
class LXARPG_API ULxCharacterQuestModule : public ULxCharacterContentModuleBase
{
	GENERATED_BODY()

public:
	/** 获取指定任务当前状态；没有记录时返回未接取。 */
	UFUNCTION(BlueprintPure, Category="角色|任务", DisplayName="获取任务状态")
	ELxQuestRuntimeState GetQuestState(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const;

	/** 判断任务是否未接取且实际任务配置中的全部直接前置任务均已完成。 */
	UFUNCTION(BlueprintPure, Category="角色|任务", DisplayName="能否接取任务")
	bool CanAcceptQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const;

	/** 接取无目标的对话型任务，并立即把任务设为可提交。 */
	UFUNCTION(BlueprintCallable, Category="角色|任务", DisplayName="接取对话型任务")
	bool AcceptDialogueQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId);

	/** 判断指定任务当前是否允许提交。 */
	UFUNCTION(BlueprintPure, Category="角色|任务", DisplayName="能否提交任务")
	bool CanSubmitQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const;

	/** 提交可提交状态的任务，并把任务设为已完成。 */
	UFUNCTION(BlueprintCallable, Category="角色|任务", DisplayName="提交任务")
	bool SubmitQuest(FGameplayTag QuestSeriesId, FGameplayTag QuestId);

	/** 判断指定任务是否已经完成。 */
	UFUNCTION(BlueprintPure, Category="角色|任务", DisplayName="任务是否已完成")
	bool IsQuestCompleted(FGameplayTag QuestSeriesId, FGameplayTag QuestId) const;

	/** 复制取得玩家当前保存的全部任务运行时记录。 */
	UFUNCTION(BlueprintPure, Category="角色|任务", DisplayName="获取全部任务记录")
	TArray<FLxQuestRuntimeRecord> GetAllQuestRecords() const { return QuestRecords; }

	/** 任务进度在本地或网络同步后发生变化时触发。 */
	UPROPERTY(BlueprintAssignable, Category="角色|任务", DisplayName="任务进度变化事件")
	FOnLxQuestProgressChanged OnQuestProgressChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/** 判断系列ID和任务ID是否组成有效的父子标签关系。 */
	static bool IsQuestIdentityValid(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId);

	/** 查找指定任务的可修改运行时记录。 */
	FLxQuestRuntimeRecord* FindQuestRecord(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId);

	/** 查找指定任务的只读运行时记录。 */
	const FLxQuestRuntimeRecord* FindQuestRecord(const FGameplayTag& QuestSeriesId, const FGameplayTag& QuestId) const;

	/** 判断当前实例是否允许修改服务器权威任务状态。 */
	bool CanModifyQuestState() const;

	/** 广播任务状态变化，并同步通知角色内容模块监听者。 */
	void BroadcastQuestProgressChanged();

	/** 客户端收到任务记录复制结果后广播刷新事件。 */
	UFUNCTION(Category="角色|任务", DisplayName="同步任务记录")
	void OnRep_QuestRecords();

	/** 玩家持有的任务状态记录；只向所属客户端同步。 */
	UPROPERTY(SaveGame, ReplicatedUsing=OnRep_QuestRecords, VisibleAnywhere, BlueprintReadOnly, Category="角色|任务",
		DisplayName="任务记录", meta=(AllowPrivateAccess="true"))
	TArray<FLxQuestRuntimeRecord> QuestRecords;
};
