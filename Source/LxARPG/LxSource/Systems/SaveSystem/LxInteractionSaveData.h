#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxItemSaveData.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxInteractionSaveData.generated.h"

/** 单个交互功能节点的持久状态，不保存资产配置、界面或运行时对象引用。 */
USTRUCT(BlueprintType, meta=(DisplayName="交互功能存档"))
struct LXARPG_API FLxInteractionFeatureSaveRecord
{
	GENERATED_BODY()

	/** 静态交互树中的节点标识，节点显示顺序改变后仍可定位原功能。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|交互", DisplayName="节点ID")
	FGuid NodeID;

	/** 防止修改节点功能类型后把旧数据应用到其他功能。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|交互", DisplayName="交互类型")
	ELxInteractionActionType InteractionType = ELxInteractionActionType::Dialogue;

	/** 交互可用状态；会话中的交互和占用状态在保存时恢复为可交互。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|交互", DisplayName="交互状态")
	ELxInteractionDataState InteractionState = ELxInteractionDataState::Interactable;

	/** 机关当前的开关状态。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|机关", DisplayName="机关状态")
	ELxMechanismState MechanismState = ELxMechanismState::Closed;

	/** 宝箱、商店或仓库的槽位内容，保留空槽位以维持原排列。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|物品", DisplayName="物品槽位")
	TArray<FLxItemSlotSaveRecord> Slots;

	/** 宝箱是否已经触发获取完成，读档时不重新发放完成事件。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|宝箱", DisplayName="已通知获取完成")
	bool bCompletionBroadcasted = false;

	/** 商店运行时的出售价格倍率。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|商店", DisplayName="商品价值倍率")
	float TradeItemValueRate = 1.0f;

	/** 商店运行时的收购价格倍率。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|商店", DisplayName="收购价值比例")
	float PurchaseValueRate = 1.0f;
};

/** 一个可交互对象的存档，以对象标签和节点标识分两层索引。 */
USTRUCT(BlueprintType, meta=(DisplayName="可交互对象存档"))
struct LXARPG_API FLxInteractionSaveRecord
{
	GENERATED_BODY()

	/** 关卡中该对象独占的持久标签，与其他对象不可重复。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|交互", DisplayName="交互对象ID")
	FGameplayTag InteractionIDTag;

	/** 该对象的所有已保存功能，节点顺序变化不会改变索引。 */
	UPROPERTY(SaveGame, BlueprintReadOnly, Category="存档|交互", DisplayName="功能节点存档")
	TMap<FGuid, FLxInteractionFeatureSaveRecord> Features;
};
