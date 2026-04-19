#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxCharacterComponentBase.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxCharacterBackpackComponent.generated.h"

class ULxItemLogicBase;
class ULxItemSlotData;
class ALxBaseCharacter;
struct FLxItemDefineBase;
struct FLxItemDateBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBackpackInstantRestore, ELxCharacterAttributeID, AttributeID, float, RestoreValue);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LXARPG_API ULxCharacterBackpackComponent : public ULxCharacterComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterBackpackComponent();

	UPROPERTY(BlueprintAssignable, DisplayName="背包即时恢复事件")
	FOnBackpackInstantRestore OnInstantRestore;

	/**
	 * @brief 初始化背包组件。
	 *
	 * 该方法用于初始化ULxCharacterBackpackComponent。它首先调用基类的BaseComponentInitialize()方法，然后调用InitializeBackpack()来完成背包的初始化工作。
	 *
	 * @note 此方法应在组件设置阶段被调用，并且根据组件的生命周期可能被多次调用。
	 */
	virtual void BaseComponentInitialize() override;

	/**
	 * @brief 向背包中添加指定ID的物品。
	 *
	 * 该函数尝试将指定数量的特定类型和ID的物品添加到角色的背包中。如果背包空间不足或参数无效，则添加操作可能失败。
	 *
	 * @param InItemType 物品类型，例如装备、消耗品等。
	 * @param InItemID 物品唯一标识符。
	 * @param InItemCount 要添加的物品数量，默认为1。
	 * @return 如果成功添加物品返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="Backpack", DisplayName="添加物品到背包-ID")
	bool AddItemByRowID(ELxItemType InItemType, FName InItemID, int32 InItemCount = 1);

	/**
	 * @brief从背包中移除指定ID的物品。
	 *
	 * 该方法尝试从角色的背包中移除特定类型和ID的指定数量的物品。如果背包中没有足够的该物品或参数无效，则移除操作可能失败。
	 *
	 * @param InItemType 物品类型，例如装备、消耗品等。
	 * @param InItemID 物品唯一标识符。
	 * @param InItemCount 要移除的物品数量。
	 * @return 如果成功移除物品返回true，否则返回false。
	 */
	bool RemoveItemAt(ELxItemType InItemType, FName InItemID, int32 InItemCount);

	/**
	 * @brief 检查背包中是否有指定数量的特定物品。
	 *
	 * 该方法用于检查角色背包中是否包含指定类型和ID的物品，并且数量至少为指定的数量。如果满足条件，则返回true，否则返回false。
	 *
	 * @param InItemType 物品类型，例如装备、消耗品等。
	 * @param InItemID 物品唯一标识符。
	 * @param InItemCount 要检查的物品数量，默认为1。
	 * @return 如果背包中存在至少指定数量的指定物品返回true，否则返回false。
	 */
	bool CheckHaveItem(ELxItemType InItemType, FName InItemID, int32 InItemCount = 1) const;

	/**
	 * @brief 对背包中的物品进行排序。
	 *
	 * 该方法用于对角色背包中的所有物品按照某种规则进行排序。具体的排序逻辑由实现者定义，可能基于物品类型、ID或其他属性。
	 *
	 * @note 此方法可以被外部调用以重新组织背包内物品的顺序，例如在用户界面中手动触发排序操作后。
	 */
	void SortingOfItems();

	/**
	 * @brief 获取背包中所有的物品。
	 *
	 * 该方法返回一个包含背包中所有物品的数组。每个元素都是指向ULxItemSlotData对象的指针，表示背包中的一个槽位及其所包含的物品数据。
	 *
	 * @return 返回一个const引用，指向存储背包中所有物品槽位数据的TArray。
	 */
	TArray<TObjectPtr<ULxItemSlotData>>& GetAllItems();

	/**
	 * @brief 查询指定类型的物品。
	 *
	 * 该方法用于从背包中查询所有属于特定类型的物品。返回的数组包含了所有符合条件的ULxItemSlotData对象的引用，这些对象代表了背包中的槽位及其所包含的物品数据。
	 *
	 * @param InItemType 要查询的物品类型，例如装备、消耗品等。
	 * @return 返回一个const引用，指向存储符合查询条件的所有物品槽位数据的TArray。
	 */
	TArray<TObjectPtr<ULxItemSlotData>>& QueryItemsOnItemType(ELxItemType InItemType);
	
protected:
	// 默认的背包槽位数量
	UPROPERTY(Blueprintable, BlueprintReadWrite, DisplayName="背包槽位数量")
	int32 BackpackSlotCount = 100;
	
private:
	UFUNCTION()
	void HandleTrackedItemChanged();

	UFUNCTION()
	void HandleTrackedItemUsed(ULxItemLogicBase* UsedItem);

	void RefreshTrackedItemBindings();

	void HandleConsumableUsed(ULxItemLogicBase* UsedItem);

	bool CleanupInvalidItems();

	/**
	 * @brief 完成背包组件的初始化工作。
	 *
	 * 该方法负责设置ULxCharacterBackpackComponent所需的所有初始配置，包括但不限于创建背包槽位、初始化物品数组等。它通常在BaseComponentInitialize()方法中被调用以确保背包组件正确初始化。
	 *
	 * @note 此方法属于内部实现细节，不应由外部代码直接调用。
	 */
	void InitializeBackpack();

	// 背包过滤、查询缓存数组
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vFilteringCache;
	
	// 背包槽位数组
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vBackpackSlots;

	// 背包内物品数组
	UPROPERTY()
	TArray<TObjectPtr<ULxItemLogicBase>> m_vItemList;

	// 当前组件依附的角色对象
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter = nullptr;
	
};
