#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxBackpackWidget.generated.h"

class ALxBaseCharacter;
class ULxBackpackData;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackpackItemListChanged, const TArray<ULxBackpackData*>&, ItemList);

/**
 * @brief 背包界面逻辑对象
 *
 * C++ 层负责从角色的背包组件和装备组件生成 ULxBackpackData 列表，
 * 并通过事件委托把列表广播给蓝图。蓝图子类可将该列表直接传给 UTileView，
 * 以动态控制物品格子数量和显示内容。
 */
UCLASS()
class LXARPG_API ULxBackpackWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void InitializeUIComponents() override;
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/** 响应背包或装备数据变化并重建列表。 */
	UFUNCTION()
	void HandleBackpackDataUpdate();

	/** 整理背包中的物品。 */
	UFUNCTION(BlueprintCallable, DisplayName="整理背包")
	void SortingOfItems();

	/** 切换当前显示的物品类型筛选。 */
	UFUNCTION(BlueprintCallable, DisplayName="切换物品分类")
	void SwitchItemType(ELxItemType NewType);

	/** 广播背包格子数据列表，供蓝图传递给 TileView。 */
	UPROPERTY(BlueprintAssignable, Category="背包|格子数据广播")
	FOnBackpackItemListChanged OnInventoryItemListChanged;

	/** 广播装备格子数据列表，供蓝图传递给 TileView。 */
	UPROPERTY(BlueprintAssignable, Category="背包|格子数据广播")
	FOnBackpackItemListChanged OnEquipmentItemListChanged;

private:
	/** 构建并广播背包格子数据。 */
	void ShowItemList();

	/** 构建并广播装备格子数据。 */
	void ShowEquipmentList();

	/** 重新绑定角色上的背包和装备组件事件。 */
	void RebindCharacterComponents(ALxBaseCharacter* PlayerCharacter);

private:
	/** 当前显示的物品类型筛选，None 表示不过滤。 */
	ELxItemType m_CurrentItemType = ELxItemType::None;

	/** 当前角色的背包组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent = nullptr;

	/** 当前角色的装备组件。 */
	UPROPERTY()
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent = nullptr;
};
