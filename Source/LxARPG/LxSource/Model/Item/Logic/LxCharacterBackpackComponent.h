#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "StructUtils/InstancedStruct.h"
#include "LxCharacterBackpackComponent.generated.h"

class ALxBaseCharacter;
class ULxItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterBackpackChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LXARPG_API ULxCharacterBackpackComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterBackpackComponent();

	/**
	 * @brief 初始化角色背包组件。
	 *
	 * 负责缓存所属角色并初始化背包格子数据。
	 */
	virtual void BaseComponentInitialize() override;

	/** 按物品类型和行 ID 创建物品并放入背包。 */
	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddItemByRowID(ELxItemType InItemType, FName InItemRowID, int32 InItemCount = 1);

	/** 按结构体数据创建物品并放入背包。 */
	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddItemByStruct(const FInstancedStruct& InItemData);

	/** 将现有物品对象放入背包空位或可堆叠位置。 */
	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddExistingItem(ULxItemData* InItemData);

	/** 将现有物品对象放入指定背包格。 */
	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddExistingItemAt(ULxItemData* InItemData, int32 InDestinationIndex);

	/** 将来自非背包来源的物品复制后放入指定背包格。 */
	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool AddItemAtFromExternal(ULxItemData* InItemData, int32 InDestinationIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 在背包内移动或交换物品。
	 *
	 * @param InSourceIndex 源格子索引。
	 * @param InDestinationIndex 目标格子索引。
	 * @return 操作成功返回 true，否则返回 false。
	 */
	bool MoveItem(int32 InSourceIndex, int32 InDestinationIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 消耗指定格子中的物品。
	 *
	 * @param InIndex 目标背包格子索引。
	 * @return 消耗成功返回 true，否则返回 false。
	 */
	bool ConsumeItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 删除指定格子中的物品。
	 *
	 * @param InIndex 目标背包格子索引。
	 * @return 删除成功返回 true，否则返回 false。
	 */
	bool RemoveItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 对背包内物品进行整理排序。
	 *
	 * 会把有效物品前移并清理空槽位。
	 */
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 获取指定格子的物品对象。
	 *
	 * @param InIndex 背包格子索引。
	 * @return 若索引有效则返回物品对象，否则返回 nullptr。
	 */
	ULxItemData* GetItemAt(int32 InIndex) const;
	
	/**
	 * @brief 获取背包内部物品数组的可写引用。
	 *
	 * @return 返回背包物品数组引用。
	 */
	TArray<TObjectPtr<ULxItemData>>& GetItems();
	
	/**
	 * @brief 查询指定类型的全部物品。
	 *
	 * @param InItemType 要筛选的物品类型。
	 * @return 返回匹配类型的物品数组副本。
	 */
	TArray<TObjectPtr<ULxItemData>> QueryTypeItem(ELxItemType InItemType) const;

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	/**
	 * @brief 获取背包格子总数。
	 *
	 * @return 当前背包可用格子数量。
	 */
	int32 GetBackpackSlotCount() const;

	/**
	 * @brief 取出指定格子的物品并清空该格子。
	 *
	 * @param InIndex 目标背包格子索引。
	 * @return 返回被取出的物品对象，失败时返回 nullptr。
	 */
	ULxItemData* TakeItemAt(int32 InIndex);

	UPROPERTY(BlueprintAssignable, Category="Character|Backpack")
	FOnCharacterBackpackChanged OnBackpackChanged;

private:
	/** 初始化背包槽位数组。 */
	void InitializeBackpackSlots();
	/** 判断索引是否在背包范围内。 */
	bool IsValidBackpackIndex(int32 InIndex) const;
	/** 查找一个空的背包格索引。 */
	int32 FindEmptySlotIndex() const;
	/** 尝试将物品堆叠到背包中的已有物品上。 */
	bool TryStackItemIntoInventory(ULxItemData* InItemData);
	/** 尝试将源物品堆叠到指定目标物品上。 */
	bool TryStackItemIntoSlot(ULxItemData* InTargetItem, ULxItemData* InSourceItem, bool& bOutSourceConsumed) const;
	/** 执行两个物品之间的堆叠。 */
	bool StackItem(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const;
	/** 判断两个物品是否允许堆叠。 */
	bool CanItemsStack(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const;
	/** 根据行 ID 从表中创建物品对象。 */
	ULxItemData* CreateItemByRowID(ELxItemType InItemType, FName InItemRowID);
	/** 广播背包内容发生变化。 */
	void BroadcastBackpackChanged();

private:
	/** 当前拥有此背包组件的角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 背包中的物品数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemData>> m_vCharacterItems;

	/** 背包是否已经完成初始化。 */
	bool m_bBackpackInitialized = false;
};
