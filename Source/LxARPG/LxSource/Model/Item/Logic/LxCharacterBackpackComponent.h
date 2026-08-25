#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemInformationBase.h"
#include "LxCharacterBackpackComponent.generated.h"

class ALxBaseCharacter;
class ULxItemBase;
class ULxItemSlotData;

/** 背包物品使用事件。背包只广播被使用的物品对象，具体词条效果由数据中转组件和词条模块处理。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackpackItemUsed, ULxItemBase*, UsedItem);

/**
 * 角色背包组件。
 *
 * 当前背包组件只使用新的 ULxItemBase 物品体系：物品通过 FLxItemQuote 创建，
 * 槽位保存 ULxItemBase 指针，词条通过物品对象自身的 GetItemEntryList 访问。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色背包模块")
class LXARPG_API ULxCharacterBackpackModule : public ULxCharacterContentModuleBase
{
	GENERATED_BODY()

public:
	/** 创建背包组件，并关闭 Tick。 */
	ULxCharacterBackpackModule();

	/** 背包物品使用事件。 */
	UPROPERTY(BlueprintAssignable, DisplayName="背包物品使用事件")
	FOnBackpackItemUsed OnItemUsed;

	/** 注册背包网络复制属性。 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 检查背包能否容纳指定的物品列表。 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="检查能否添加物品列表")
	bool CanAddItemList(const TArray<FLxItemQuote>& InItemList) const;

	/** 添加物品列表到背包。 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="添加物品列表")
	bool AddItemList(const TArray<FLxItemQuote>& InItemList);

	/** 检查背包是否拥有物品列表中的所有物品。 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="检查是否拥有物品列表")
	bool CheckHaveItemList(const TArray<FLxItemQuote>& InItemList) const;

	/** 从背包中移除指定的物品列表。 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="移除物品列表")
	bool RemoveItemList(const TArray<FLxItemQuote>& InItemList);

	/** 按物品类型和稀有度对背包物品重新排序。 */
	void SortingOfItems();

	/** 获取背包全部槽位。 */
	TArray<TObjectPtr<ULxItemSlotData>>& GetAllItems();

	/** 获取指定索引处的背包槽位。 */
	ULxItemSlotData* GetBackpackSlotAt(int32 SlotIndex) const;

	/** 将槽位从源索引移动到目标索引。 */
	bool MoveBackpackSlot(int32 SourceSlotIndex, int32 TargetSlotIndex);

	/** 将本地背包槽位数据同步到复制数组，用于网络同步。 */
	void SyncReplicatedBackpackSlots();

protected:
	/** 初始化背包槽位和角色缓存。 */
	virtual void OnModuleInitialize() override;

	/** 默认背包槽位数量。 */
	UPROPERTY(Blueprintable, BlueprintReadWrite, DisplayName="背包槽位数量")
	int32 BackpackSlotCount = 100;

private:
	/** 向背包添加一类物品，仅作为批量添加接口的内部步骤。 */
	bool AddItemByTagID(FGameplayTag InItemIDTag, int32 InItemCount);

	/** 从背包移除一类物品，仅作为批量移除接口的内部步骤。 */
	bool RemoveItemAt(FGameplayTag InItemIDTag, int32 InItemCount);

	/** 检查背包中是否拥有指定数量的一类物品。 */
	bool CheckHaveItem(FGameplayTag InItemIDTag, int32 InItemCount) const;

	/** 背包槽位使用物品成功后统一广播使用事件。 */
	void NotifyItemUsedFromSlot(ULxItemBase* UsedItem);

	/** 物品数量变化时刷新背包状态。 */
	UFUNCTION()
	void HandleTrackedItemCountChanged(ULxItemBase* Item);

	/** 槽位内容变化时刷新背包状态。 */
	UFUNCTION()
	void HandleBackpackSlotChanged(ULxItemBase* InItemData);

	/** 重新绑定当前背包内物品和槽位事件。 */
	void RefreshTrackedBindings();

	/** 清理无效物品和空引用。 */
	bool CleanupInvalidItems();

	/** 初始化背包槽位。 */
	void InitializeBackpack();

	/** 将服务器复制来的槽位数据应用到本地背包。 */
	void ApplyReplicatedBackpackSlots();

	/** 从槽位数据构建物品引用结构体。 */
	FLxItemQuote BuildItemQuoteFromSlot(ULxItemSlotData* SlotData) const;

	/** 背包槽位数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vBackpackSlots;

	/** 背包内有效物品缓存。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemBase>> m_vItemList;

	/** 用于网络复制的背包槽位引用数组。 */
	UPROPERTY(ReplicatedUsing=OnRep_BackpackSlots)
	TArray<FLxItemQuote> ReplicatedBackpackSlots;

	/** 当前组件所属角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter = nullptr;

	/** 复制回调，当服务器同步背包槽位数据到客户端时调用。 */
	UFUNCTION()
	void OnRep_BackpackSlots();
};
