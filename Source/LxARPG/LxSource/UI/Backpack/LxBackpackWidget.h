#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxBackpackWidget.generated.h"

class ALxBaseCharacter;
class ULxItemUIData;
class ULxCharacterBackpackComponent;
class ULxCharacterEquipmentComponent;
class ULxItemSlotData;
class ULxSkillSlotData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackpackItemListChanged, const TArray<ULxItemUIData*>&, ItemList);

/**
 * @class ULxBackpackWidget
 * @brief 背包UI组件，用于显示和管理玩家角色的背包、装备、快捷栏等信息。
 *
 * 该类扩展自ULxUIBaseObject，并提供了与背包相关的功能，包括物品排序、切换物品类型、更新UI组件等。
 * 它还定义了多个事件委托，当相应列表发生变化时可以触发这些事件。
 *
 * @note 继承自ULxUIBaseObject，实现了初始化和更新UI组件的方法。
 */
UCLASS()
class LXARPG_API ULxBackpackWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	virtual void InitializeUIComponents() override;
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	UFUNCTION()
	void HandleBackpackDataUpdate();

	UFUNCTION()
	void HandleLocalSlotsDataUpdate();

	UFUNCTION(BlueprintCallable, DisplayName="������")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, DisplayName="�л���Ʒ����")
	void SwitchItemType(ELxItemType NewType);

	UPROPERTY(BlueprintAssignable, Category="����|�������ݹ㲥")
	FOnBackpackItemListChanged OnInventoryItemListChanged;

	UPROPERTY(BlueprintAssignable, Category="����|�������ݹ㲥")
	FOnBackpackItemListChanged OnEquipmentItemListChanged;

	UPROPERTY(BlueprintAssignable, Category="����|�������ݹ㲥")
	FOnBackpackItemListChanged OnShortcutItemListChanged;

	UPROPERTY(BlueprintAssignable, Category="����|�������ݹ㲥")
	FOnBackpackItemListChanged OnWarehouseItemListChanged;

	UPROPERTY(BlueprintAssignable, Category="����|�������ݹ㲥")
	FOnBackpackItemListChanged OnSkillItemListChanged;

private:
	void ShowItemList();
	void ShowEquipmentList();
	void ShowShortcutList();
	void ShowWarehouseList();
	void ShowSkillList();
	void RebindCharacterComponents(ALxBaseCharacter* PlayerCharacter);
	void InitializeLocalSlots();
	void BindLocalSlots();
	void UnbindLocalSlots();

private:
	ELxItemType m_CurrentItemType = ELxItemType::None;

	UPROPERTY()
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent = nullptr;

	UPROPERTY()
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Backpack|Local Slots")
	int32 ShortcutSlotCount = 10;

	UPROPERTY(EditDefaultsOnly, Category="Backpack|Local Slots")
	int32 WarehouseSlotCount = 30;

	UPROPERTY(EditDefaultsOnly, Category="Backpack|Local Slots")
	int32 SkillSlotCount = 10;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vShortcutSlots;

	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vWarehouseSlots;

};
