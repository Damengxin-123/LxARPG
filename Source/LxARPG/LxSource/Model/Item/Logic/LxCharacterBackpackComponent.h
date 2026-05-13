#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
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
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="角色背包组件")
class LXARPG_API ULxCharacterBackpackComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	/** 创建背包组件，并关闭 Tick。 */
	ULxCharacterBackpackComponent();

	/** 背包物品使用事件。 */
	UPROPERTY(BlueprintAssignable, DisplayName="背包物品使用事件")
	FOnBackpackItemUsed OnItemUsed;

	/** 初始化背包槽位和组件缓存。 */
	virtual void BaseComponentInitialize() override;

	/**
	 * 按物品标签 ID 添加物品到背包。
	 *
	 * @param InItemIDTag 物品标签 ID。
	 * @param InItemCount 添加数量。
	 * @return 添加成功返回 true。
	 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="添加物品到背包-标签ID", meta=(Categories="物品"))
	bool AddItemByTagID(FGameplayTag InItemIDTag, int32 InItemCount = 1);

	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="检查能否添加物品列表")
	bool CanAddItemList(const TArray<FLxItemQuote>& InItemList) const;

	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="添加物品列表")
	bool AddItemList(const TArray<FLxItemQuote>& InItemList);

	/**
	 * 从背包中移除指定数量的物品。
	 *
	 * @param InItemIDTag 物品标签 ID。
	 * @param InItemCount 移除数量。
	 * @return 移除成功返回 true。
	 */
	bool RemoveItemAt(FGameplayTag InItemIDTag, int32 InItemCount);

	/**
	 * 检查背包中是否拥有指定数量的物品。
	 *
	 * @param InItemIDTag 物品标签 ID。
	 * @param InItemCount 需要检查的数量。
	 * @return 数量满足时返回 true。
	 */
	bool CheckHaveItem(FGameplayTag InItemIDTag, int32 InItemCount = 1) const;

	/** 按物品类型和稀有度对背包物品重新排序。 */
	void SortingOfItems();

	/** 获取背包全部槽位。 */
	TArray<TObjectPtr<ULxItemSlotData>>& GetAllItems();

	/** 按物品类型查询背包槽位，结果缓存在组件内部数组中。 */
	TArray<TObjectPtr<ULxItemSlotData>>& QueryItemsOnItemType(ELxItemType InItemType);

	/** 背包槽位使用物品成功后调用，由背包组件统一广播给数据中转组件。 */
	void NotifyItemUsedFromSlot(ULxItemBase* UsedItem);

protected:
	/** 默认背包槽位数量。 */
	UPROPERTY(Blueprintable, BlueprintReadWrite, DisplayName="背包槽位数量")
	int32 BackpackSlotCount = 100;

private:
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

	/** 背包过滤查询缓存数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vFilteringCache;

	/** 背包槽位数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vBackpackSlots;

	/** 背包内有效物品缓存。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemBase>> m_vItemList;

	/** 当前组件所属角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter = nullptr;
};
