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
class ULxEquipmentSlotData;

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
	/**
 	 * @brief 初始化背包UI组件。
 	 *
 	 * 该方法负责初始化背包界面中的所有UI元素。它首先调用基类的InitializeUIComponents方法，然后执行特定于背包UI的初始化逻辑。
 	 * 当前实现中，除了调用基类的方法外，没有额外的初始化操作。
 	 *
 	 * @note 重写自ULxUIBaseObject::InitializeUIComponents()，确保在创建背包UI时调用此方法以完成必要的初始化设置。
 	 */
	virtual void InitializeUIComponents() override;
	/**
	 * @brief 根据当前玩家角色刷新背包UI组件。
	 *
	 * 该方法负责根据传入的玩家角色对象更新背包UI中的所有相关组件。它首先调用基类的UpdateUIComponents方法，然后执行特定于背包UI的更新逻辑。
	 *
	 * @param PlayerCharacter 当前控制的角色对象，用于获取和更新背包相关的数据。
	 */
	virtual void UpdateUIComponents(ALxBaseCharacter* PlayerCharacter) override;

	/**
	 * @brief 当物品列表更新时触发的事件。
	 *
	 * 该委托用于通知监听者背包内的物品列表发生了变化。可以用于在UI或其他系统中响应这些变化，例如刷新显示或更新逻辑状态。
	 *
	 * @note 该属性为蓝图可分配，并归类于"物品栏内容更新事件"类别下。
	 */
	UPROPERTY(BlueprintAssignable, Category="物品栏内容更新事件")
	FOnUiBaseUpdateEvent OnItemListUpdated;
	/**
	 * @brief 当装备栏内容更新时触发的事件。
	 *
	 * 该委托用于通知监听者装备栏内的物品列表发生了变化。可以用于在UI或其他系统中响应这些变化，例如刷新显示或更新逻辑状态。
	 *
	 * @note 该属性为蓝图可分配，并归类于"装备栏内容更新事件"类别下。
	 */
	UPROPERTY(BlueprintAssignable, Category="装备栏内容更新事件")
	FOnUiBaseUpdateEvent OnEquipmentListUpdated;

	/**
	 * @brief 对背包中的物品进行排序。
	 *
	 * 该方法调用背包组件（m_pCharacterBackpackComponent）的SortingOfItems方法，以对背包中的物品进行排序。如果背包组件不为空，则执行排序操作。
	 *
	 * @note 此方法在蓝图中可调用，并显示为"物品排序"。
	 */	
	UFUNCTION(BlueprintCallable, DisplayName="物品排序")
	void SortingOfItems();

	UFUNCTION(BlueprintCallable, DisplayName="获取物品栏数据列表")
	TArray<UObject*> GetItemUIDataList();

	UFUNCTION(BlueprintCallable, DisplayName="获取装备栏数据列表")
	TArray<UObject*> GetEquipmentUIDataList();
	/**
	 * @brief 切换当前显示的物品类型。
	 *
	 * 该方法用于切换背包中显示的物品类型。根据传入的物品类型参数，更新当前显示的物品列表，并调用ShowItemList()方法来刷新UI。
	 *
	 * @param NewType 新的物品类型，用于过滤和显示背包中的相应物品。
	 */
	UFUNCTION(BlueprintCallable, DisplayName="按照类型过滤显示物品")
	void SwitchItemType(ELxItemType NewType);

private:

	// 更新UI界面显示，当玩家角色更新之后
	void UpdatedBackpack();

	
	/**
	 * @brief 指向角色背包组件的指针，用于访问和操作玩家角色的背包数据。
	 *
	 * 该变量存储了ULxCharacterBackpackComponent类型的对象指针，通过它可以调用背包相关的功能，例如对物品进行排序或获取背包中的所有物品。如果此指针为空，则表示当前没有关联的角色背包组件。
	 *
	 * @note 该属性为UPROPERTY，支持反射和序列化。
	 */
	UPROPERTY()
	TObjectPtr<ULxCharacterBackpackComponent> m_pCharacterBackpackComponent = nullptr;
	/**
	 * @brief 指向角色装备组件的指针，用于访问和操作玩家角色的装备数据。
	 *
	 * 该变量存储了ULxCharacterEquipmentComponent类型的对象指针，通过它可以调用装备相关的功能，例如获取装备槽位信息或更新装备状态。如果此指针为空，则表示当前没有关联的角色装备组件。
	 *
	 * @note 该属性为UPROPERTY，支持反射和序列化。
	 */
	UPROPERTY()
	TObjectPtr<ULxCharacterEquipmentComponent> m_pCharacterEquipmentComponent = nullptr;
	/**
	 * @brief 背包中的物品槽位列表。
	 *
	 * 该数组存储了背包中所有物品槽位的数据指针，每个元素都是一个ULxItemSlotData对象。这些数据用于在UI中显示和管理背包内的物品。
	 *
	 * @note 该属性为UPROPERTY，支持反射和序列化。
	 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vItemSlotList;
	/**
	 * @brief 装备槽位列表。
	 *
	 * 该数组存储了装备栏中所有装备槽位的数据指针，每个元素都是一个ULxItemSlotData对象。这些数据用于在UI中显示和管理角色的装备信息。
	 *
	 * @note 该属性为UPROPERTY，支持反射和序列化。
	 */
	UPROPERTY()
	TArray<TObjectPtr<ULxEquipmentSlotData>> m_vEquipmentSlotList;

};
