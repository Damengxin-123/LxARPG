// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Item/ItemBase/LxItemEnmuType.h"
#include "UObject/Object.h"
#include "Runtime/CoreUObject/Public/StructUtils/InstancedStruct.h"
#include "LxItemData.generated.h"

/**
 * @brief 物品数量改变事件
 *
 * 当物品数量发生改变时触发的多播委托事件。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemQuantityChangeEvent, ULxItemData*, ItemDataPtr, bool, IsValid);

/**
 * 
 */
UCLASS()
class LXARPG_API ULxItemData : public UObject
{
	GENERATED_BODY()
	public:
	
	/**
	 * @brief 创建新的物品数据对象
	 *
	 * 该函数用于创建一个ULxItemData的新实例，并使用给定的物品数据进行初始化。
	 *
	 * @param pParent 父对象，指定新创建的ULxItemData实例的外部拥有者
	 * @param ItemData 物品数据指针，用于初始化新的ULxItemData实例
	 * @return 返回指向新创建的ULxItemData实例的指针
	 */
	static ULxItemData* CreateNewItemData(UObject* pParent, FLxItemBase* ItemData);

	/**
	 * @brief 创建一个新的ULxItemData实例
	 *
	 * 该函数用于创建一个ULxItemData的新实例，并使用给定的物品数据进行初始化。
	 *
	 * @param pParent 父对象，指定新创建的ULxItemData实例的外部拥有者
	 * @param ItemData 物品数据，类型为FInstancedStruct，用于初始化新的ULxItemData实例
	 * @return 返回指向新创建的ULxItemData实例的指针
	 */
	UFUNCTION(BlueprintCallable, DisplayName="创建新的物品信息数据对象")
	static ULxItemData* CreateNewItemData(UObject* pParent, const FInstancedStruct& ItemData);

	/**
	 * @brief 创建新的装备物品数据对象
	 *
	 * 该函数用于创建一个装备类型的ULxItemData实例。
	 *
	 * @param pParent 父对象，指定新创建的ULxItemData实例的外部拥有者
	 * @param ItemData 装备属性数据，用于初始化新的ULxItemData实例
	 * @return 返回指向新创建的ULxItemData实例的指针
	 */
	static ULxItemData* CreateNewEquipmentItemData(UObject* pParent, const FLxEquipmentData& ItemData);

	/**
	 * @brief 创建新的消耗品物品数据对象
	 *
	 * 该函数用于创建一个消耗品类型的ULxItemData实例。
	 *
	 * @param pParent 父对象，指定新创建的ULxItemData实例的外部拥有者
	 * @param ItemData 消耗品属性数据，用于初始化新的ULxItemData实例
	 * @return 返回指向新创建的ULxItemData实例的指针
	 */
	static ULxItemData* CreateNewConsumableItemData(UObject* pParent, const FLxConsumableData& ItemData);

	/**
	 * @brief 创建新的材料物品数据对象
	 *
	 * 该函数用于创建一个材料类型的ULxItemData实例。
	 *
	 * @param pParent 父对象，指定新创建的ULxItemData实例的外部拥有者
	 * @param ItemData 材料属性数据，用于初始化新的ULxItemData实例
	 * @return 返回指向新创建的ULxItemData实例的指针
	 */
	static ULxItemData* CreateNewMaterialItemData(UObject* pParent, const FLxMaterialData& ItemData);
	
	
	/**
	 * @brief 使用物品
	 *
	 * 该函数用于执行使用当前ULxItemData实例所代表的物品的操作。
	 *
	 * @return 返回一个布尔值，表示物品是否被成功使用
	 */
	UFUNCTION(BlueprintCallable, DisplayName="使用物品")
	bool UseItem();

	/**
	 * @brief 判断给定的物品ID是否与当前物品的ID相同
	 *
	 * 该函数用于比较给定的物品ID（FName类型）与当前ULxItemData实例所代表的物品ID是否一致。
	 *
	 * @param ItemID 要比较的物品ID，类型为FName
	 * @return 返回一个布尔值，如果给定的物品ID与当前物品ID相同则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, DisplayName="判断物品ID是否与此物品相同")
	bool ItemIDIsMe(FName ItemID);

	/**
	 * @brief 判断物品是否还有效
	 *
	 * 该函数用于检查当前ULxItemData实例所代表的物品是否还有效。
	 * 通常用于检查物品数量是否大于0或其他有效性条件。
	 *
	 * @return 返回一个布尔值，如果物品有效则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, DisplayName="物品是否还有效")
	bool IsValid();
	
	/**
	 * @brief 判断物品是否可以堆叠
	 *
	 * 该函数用于检查当前ULxItemData实例所代表的物品是否支持堆叠。
	 *
	 * @return 返回一个布尔值，如果物品支持堆叠则返回true，否则返回false
	 */
	UFUNCTION(BlueprintCallable, DisplayName="判断物品是否可以堆叠")
	bool ItemIsStack();

	/**
	 * @brief 将来源物品堆叠到当前物品
	 *
	 * 该函数尝试将给定的`SourceItemData`中的物品堆叠到调用此方法的`ULxItemData`实例所代表的物品上。只有当两个物品可以堆叠时，才会执行堆叠操作。
	 *
	 * @param SourceItemData 源物品数据，指向要堆叠到当前物品的`ULxItemData`实例
	 * @return 如果堆叠成功返回true；如果堆叠失败（例如，物品类型不匹配或已经达到最大堆叠数量）则返回false
	 */
	UFUNCTION(BlueprintCallable, DisplayName="堆叠物品")
	bool StackItemToMe(ULxItemData* SourceItemData);

	/**
	 * @brief 获取当前物品的类型
	 *
	 * 该函数用于获取ULxItemData实例所代表的物品的具体类型，返回值为ELxItemType枚举类型。
	 *
	 * @return 返回一个ELxItemType枚举值，表示当前物品的类型
	 */
	UFUNCTION(BlueprintCallable, DisplayName="获取物品类型")
	ELxItemType GetItemType();
	
	/**
	 * @brief 获取当前物品的具体信息
	 *
	 * 该函数用于获取ULxItemData实例所代表的物品的具体信息。返回值类型取决于物品的实际类型，例如装备、消耗品等。
	 *
	 * @tparam ItemType 物品信息的数据类型
	 * @return 返回一个指向ItemType类型的常量引用，表示当前物品的具体信息
	 */
	template<typename ItemType>
	const ItemType* GetConstItemInfo();

	/**
 	 * @brief 获取当前物品的基础属性
 	 *
 	 * 该函数用于获取ULxItemData实例所代表的物品的基础属性信息。如果存在基础属性数据，则返回指向FLxItemData结构体的指针；否则，返回nullptr。
 	 *
 	 * @return 返回一个指向FLxItemData类型的指针，表示当前物品的基础属性。如果未找到相关属性，则返回nullptr。
 	 */
	UFUNCTION(BlueprintCallable, DisplayName="获取基类物品类型")
	FLxItemBase& GetItemBase();

	/**
	 * @brief 获取当前装备的具体信息
	 *
	 * 该函数用于获取ULxItemData实例所代表的装备的具体信息。如果当前物品是装备类型，则返回其属性数据。
	 *
	 * @return 返回一个指向FLxEquipmentData类型的常量指针，表示当前装备的具体信息。如果当前物品不是装备，则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, DisplayName="获取装备信息")
	FLxEquipmentData& GetEquipmentItemData();

	/**
	 * @brief 获取消耗品信息
	 *
	 * 该函数用于获取当前ULxItemData实例所代表的物品是否为消耗品，并返回其具体信息。如果当前物品是消耗品类别，则返回其属性数据。
	 *
	 * @return 返回一个指向FLxConsumableData类型的常量指针，表示当前消耗品的具体信息。如果当前物品不是消耗品，则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, DisplayName="获取消耗品信息")
	FLxConsumableData& GetConsumableItemData();

	/**
	 * @brief 获取材料物品信息
	 *
	 * 该函数用于获取当前ULxItemData实例所代表的物品是否为材料类型，并返回其具体信息。如果当前物品是材料类别，则返回其属性数据。
	 *
	 * @return 返回一个指向FLxMaterialData类型的常量指针，表示当前材料的具体信息。如果当前物品不是材料，则返回nullptr。
	 */
	UFUNCTION(BlueprintCallable, DisplayName="获取消耗品信息")
	FLxMaterialData& GetMaterialItemData();

	/**
	 * @var FOnItemQuantityChangeEvent OnItemQuantityChange
	 * @brief 物品数量发生变化时触发的事件
	 *
	 * 该委托在物品的数量发生更改时被调用，可以用于监听并响应物品数量的变化。
	 * 通过绑定此事件，可以在游戏逻辑中实现对物品数量变化的响应，例如更新UI、记录日志等。
	 */
	UPROPERTY(BlueprintAssignable, DisplayName="物品属性发生更改")
	FOnItemQuantityChangeEvent OnItemQuantityChange;
private:

	/**
	 * @brief 通知物品数量发生变化
	 *
	 * 该函数用于在物品数量发生变化时进行通知。当调用此函数时，可以触发相关的逻辑处理，例如更新UI、记录日志等。
	 * 这是一个内部使用的函数，通常由系统自动调用，以响应物品数量的变化。
	 */
	inline void ItemCountChange();

	/**
	 * @var FInstancedStruct m_fItemInfo
	 * @brief 可以实现多态的结构体类型
	 *
	 * 用于存储物品的具体信息，支持多态。
	 */
	FInstancedStruct m_fItemInfo;

	/**
	 * @var ELxItemType m_eRuntimeItemType
	 * @brief 物品类型
	 *
	 * 运行时确定的物品类型。
	 */
	ELxItemType m_eRuntimeItemType = ELxItemType::None;
};
