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
	bool MoveItem(int32 InSourceIndex, int32 InDestinationIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool ConsumeItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	bool RemoveItemAt(int32 InIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	ULxItemData* GetItemAt(int32 InIndex) const;
	
	TArray<TObjectPtr<ULxItemData>>& GetItems();
	
	TArray<TObjectPtr<ULxItemData>> QueryTypeItem(ELxItemType InItemType) const;

	UFUNCTION(BlueprintCallable, Category="Character|Backpack")
	int32 GetBackpackSlotCount() const;

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
