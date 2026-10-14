#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxItemSaveData.h"
#include "LxSafeItemOperations.generated.h"

class ULxItemBase;
class ULxItemSlotData;

/** 安全提取的待提交凭证；来源仍保留完整物品，取消不会改变任何容器。 */
UCLASS(BlueprintType, DisplayName="物品安全转移凭证")
class LXARPG_API ULxItemTransferTicket : public UObject
{
	GENERATED_BODY()

public:
	/** 提取时的真实来源槽位，用于最终一致性检查。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="物品|安全转移", DisplayName="来源槽位")
	TObjectPtr<ULxItemSlotData> SourceSlot = nullptr;

	/** 保留完整实例及其生命周期，避免按物品ID重新生成装备。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="物品|安全转移", DisplayName="待转移物品")
	TObjectPtr<ULxItemBase> Item = nullptr;

	/** 提取时的实例快照，接收前必须与来源当前数据完全一致。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="物品|安全转移", DisplayName="期望物品记录")
	FLxItemSaveRecord ExpectedItem;

	/** 本次快速发送的整堆数量。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="物品|安全转移", DisplayName="转移数量")
	int32 Count = 0;

	/** 提交或取消后不可再次接收同一凭证。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="物品|安全转移", DisplayName="是否已完成")
	bool bCompleted = false;

	/** 终结未提交凭证；来源物品始终保留，因此无需执行补偿添加。 */
	UFUNCTION(BlueprintCallable, Category="物品|安全转移", DisplayName="取消物品转移")
	void Cancel();

	/** 查询凭证是否已经提交或取消。 */
	UFUNCTION(BlueprintPure, Category="物品|安全转移", DisplayName="物品转移是否已完成")
	bool IsCompleted() const { return bCompleted; }
};

/** 背包和仓库共享的数据层安全操作；界面只传递凭证，不修改槽位内容。 */
namespace LxSafeItemOperations
{
	/** 查询容器是否正在提交或发布安全操作结果，写接口据此拒绝通知回调中的重入。 */
	LXARPG_API bool IsContainerBusy(const UObject* Container);

	/** 一次锁住全部参与容器；调用方仅在成功后释放，锁应覆盖物品与金币提交和最终通知。 */
	LXARPG_API bool AcquireContainerOperationLocks(const TArray<UObject*>& Containers);

	/** 释放已经成功获取的容器锁，交易模块通过作用域保证失败退出也执行释放。 */
	LXARPG_API void ReleaseContainerOperationLocks(const TArray<UObject*>& Containers);

	/** 查询槽位所属容器的操作锁，避免通知期间拖放、使用或再次拆分。 */
	LXARPG_API bool IsSlotBusy(const ULxItemSlotData* Slot);

	/** 只允许普通背包和仓库槽位，排除技能、Buff、快捷引用及交易槽。 */
	LXARPG_API bool IsSupportedSlot(const ULxItemSlotData* Slot);

	/** 快捷发送来源还允许只可取出的宝箱和执行购买的交易格；此检查不授予放置或拆分权限。 */
	LXARPG_API bool IsQuickTransferSourceSlot(const ULxItemSlotData* Slot);

	/** 只允许完整装备实例及普通消耗品、材料，技能和Buff不参与。 */
	LXARPG_API bool IsSupportedItem(ULxItemBase* Item);

	/** 比较完整实例记录，拒绝界面显示之后数量、词条或装备状态已经变化的请求。 */
	LXARPG_API bool MatchesExpectedRecord(ULxItemSlotData* SourceSlot, const FLxItemSaveRecord& ExpectedRecord);

	/** 建立不修改来源的数据层提取凭证，可用于客户端只读预检查。 */
	LXARPG_API ULxItemTransferTicket* BeginSafeExtract(ULxItemSlotData* SourceSlot, FText& OutReason);

	/** 只读检查整堆能否安全进入目标；不交换其他物品。 */
	LXARPG_API bool CanAccept(ULxItemTransferTicket* Ticket, const TArray<TObjectPtr<ULxItemSlotData>>& TargetSlots, FText& OutReason);

	/** 权威数据层全量接收，先合并同属性堆叠，再把余量原实例放到空格。 */
	LXARPG_API bool Accept(ULxItemTransferTicket* Ticket, const TArray<TObjectPtr<ULxItemSlotData>>& TargetSlots, FText& OutReason);

	/** 只读查询拆分资格，不预先占用空格，适用于打开数量窗口之前。 */
	LXARPG_API bool CanSplitItem(ULxItemSlotData* SourceSlot, FText& OutReason);

	/** 只向独立合法空格拆分，当前操作跳过合并，后续仍使用普通堆叠规则。 */
	LXARPG_API bool Split(ULxItemSlotData* SourceSlot, const TArray<TObjectPtr<ULxItemSlotData>>& Slots, int32 Count, FText& OutReason);

	/** 取消未提交的转移，保留来源全部数据。 */
	LXARPG_API void Cancel(ULxItemTransferTicket* Ticket);
}
