#pragma once

#include "CoreMinimal.h"
#include "LxItemSlotDataType.h"
#include "LxItemSlotEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxItemSlotData.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxItemSlotChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDataChanged, ULxItemBase*, InItemData);

UCLASS(BlueprintType, DisplayName="物品槽位类型")
class LXARPG_API ULxItemSlotData : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * @brief 检查当前槽位是否包含一个有效的物品。
	 *
	 * 该方法用于验证当前槽位内是否有有效的物品数据引用。有效的定义是槽位内的物品数据引用非空且物品本身也是有效的。
	 *
	 * @return 如果槽位中的物品有效则返回true，否则返回false。
	 */
	bool IsValid();

	/**
	 * @brief 执行当前槽位内物品的使用逻辑。
	 *
	 * 该方法尝试使用当前槽位内的物品。如果槽位中的物品无效（即 `IsValid` 返回 false），则不会执行任何操作。
	 * 如果物品有效，将调用物品自身的 `ItemUse` 方法，并根据其返回的状态决定后续处理方式。
	 * 如果是激活词条类物品且使用成功，会通知背包组件进行统一的使用后处理。
	 * 使用成功后，会广播当前物品引用，以便监听该槽位的 UI 进行刷新。
	 */
	void ItemUse();

	/** 按下输入时开始使用当前槽位物品。 */
	UFUNCTION(BlueprintCallable, Category="物品槽位", DisplayName="开始使用槽位物品")
	void StartUseItem();

	/** 抬起输入时结束使用当前槽位物品。 */
	UFUNCTION(BlueprintCallable, Category="物品槽位", DisplayName="结束使用槽位物品")
	void EndUseItem();

	/**
	 * @brief 初始化物品槽位。
	 *
	 * 该方法用于初始化一个物品槽位，设置其类型、物品类型标签以及可选的物品数据引用。如果传入的槽位类型为 `None`，则不会进行任何操作。
	 *
	 * @param InItemSlotType 槽位的类型，定义了槽位的功能和逻辑。
	 * @param InItemType 物品类型标签，默认为 `LxTag_Item`。
	 * @param InItemData 可选的物品数据引用，用于初始化槽位中的物品，默认为 `nullptr`。
	 */
	void InitItemSlot(ELxItemSlotType InItemSlotType, FGameplayTag InItemType = LxTag_Item, ULxItemBase* InItemData = nullptr);

	/**
	 * @brief 设置当前槽位中的物品。
	 *
	 * 该方法尝试将传入的物品数据设置到当前槽位中。只有当传入的物品数据非空且有效时，才会被设置到槽位中，并广播物品数据已更改的事件。
	 *
	 * @param InItemData 要设置到槽位中的物品数据引用。
	 * @return 如果成功设置了有效的物品数据，则返回true；否则返回false。
	 */
	bool SetItem(ULxItemBase* InItemData);

	ULxItemBase* GetItem() const { return m_pItemData; }

	ELxItemSlotType GetSlotType() const { return m_eSlotType; }

	int32 GetSlotIndex() const { return SlotIndex; }

	void SetSlotIndex(int32 InSlotIndex) { SlotIndex = InSlotIndex; }

	FGameplayTag GetItemTypeTag() const { return m_fItemTypeTag; }

	UFUNCTION(BlueprintPure, Category="物品槽位", DisplayName="获取物品价值")
	int32 GetItemValue() const;

	UFUNCTION(BlueprintCallable, Category="物品槽位", DisplayName="设置物品价值倍率")
	void SetItemValueRate(float InItemValueRate);

	UFUNCTION(BlueprintCallable, Category="物品槽位", DisplayName="设置是否可以交易")
	void SetCanTrade(bool bInCanTrade);

	UFUNCTION(BlueprintPure, Category="物品槽位", DisplayName="是否可以交易")
	bool CanTrade() const { return bCanTrade; }

	/**
	 * @brief 清空当前槽位中的物品。
	 *
	 * 该方法用于移除当前槽位中的物品数据引用，并将其设置为 `nullptr`。当槽位中的物品被清空后，会广播一个事件通知相关的监听器槽位中的物品已发生变化。
	 */
	void ClearItem();

	/**
	 * @brief 检查当前槽位是否允许物品进入。
	 *
	 * 该方法用于判断当前槽位是否配置为允许物品进入。如果槽位逻辑设置中的 `ItemEnter` 为 true，则表示允许物品进入该槽位。
	 *
	 * @return 如果槽位允许物品进入则返回true，否则返回false。
	 */
	bool ItemIsEnter() const;

	/**
	 * @brief 检查当前槽位是否允许物品离开。
	 *
	 * 该方法用于判断当前槽位是否配置为允许物品离开。如果槽位逻辑设置中的 `ItemLeave` 为 true，则表示允许物品从该槽位离开。
	 *
	 * @return 如果槽位允许物品离开则返回true，否则返回false。
	 */
	bool ItemIsLeave() const;

	/**
	 * @brief 尝试将指定槽位中的物品移动到当前槽位。
	 *
	 * 该方法尝试将`InItemSlot`槽位中的物品移动到调用该方法的槽位中。根据不同的条件，可能的结果包括成功交换、堆叠、直接进入目标槽位等。
	 *
	 * @param InItemSlot 指向要从中移动物品的ULxItemSlotData对象指针。
	 * @return 返回一个ELxItemSlotDropResult枚举值，表示操作的结果。可能的返回值包括Swapped, StackedAll, StackedPartial, FailedInvalidSource, CannotEnter, TypeError, EnterSuccess。
	 */
	ELxItemSlotDropResult ItemEnterToThis(ULxItemSlotData* InItemSlot);

	/**
	 * @brief代表物品数据更改时触发的事件。
	 *
	 * 当槽位中的物品数据发生改变时，会通过该委托广播一个事件。这可以用于通知监听者槽位内的物品已被更新、移除或堆叠等。
	 * 该事件在 `ItemUse`、`SetItem`、`ClearItem` 和 `ItemEnterToThis` 方法中被调用，以确保任何对槽位内物品数据的操作都能及时通知到相关的监听器。
	 */
	FOnItemDataChanged OnItemDataChanged;
private:
	UFUNCTION()
	void HandleItemCountChanged(ULxItemBase* ChangedItem);

	/**
	 * @brief 定义当前槽位的类型。
	 *
	 * 该变量表示槽位的功能和逻辑。不同的槽位类型可以有不同的行为，例如背包格子、装备格子等。
	 * 初始值为 `ELxItemSlotType::None`，表示没有特定归属的槽位。
	 */
	ELxItemSlotType m_eSlotType = ELxItemSlotType::None;

	/**
	 * @brief 定义当前槽位内物品的类型标签。
	 *
	 * 该变量用于标识当前槽位可以容纳的物品类型。默认值为 `LxTag_Item`，表示通用的物品类型。
	 * 在初始化槽位时，可以通过 `InitItemSlot` 方法来设置不同的物品类型标签。
	 */
	FGameplayTag m_fItemTypeTag = LxTag_Item;

	/**
	 * @brief 槽位逻辑设置集合，定义了槽位的交互行为。
	 *
	 * 该结构体包含了控制槽位内物品如何与外部环境交互的逻辑。具体来说，它定义了是否允许物品进入或离开槽位，以及物品进入后是否作为快捷方式处理。
	 * - `ItemEnter` 控制是否允许物品被拖动进入当前槽位。
	 * - `ItemLeave` 控制是否允许物品从当前槽位被拖动出去。
	 * - `IsShortcut` 确定了当物品进入槽位时，是否只引用物品而不清空原有槽位中的物品数据（即作为快捷方式）。
	 */
	FLxSlotLogicSet m_fSlotLogicSet;

	/**
	 * @brief 槽位中当前存储的物品数据引用。
	 *
	 * 该变量保存了槽位内当前持有的物品数据。如果槽位为空，则该引用为 `nullptr`。
	 * 通过这个引用，可以访问和操作槽位中的物品数据。
	 */
	UPROPERTY(DisplayName="物品数据引用")
	TObjectPtr<ULxItemBase> m_pItemData = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品槽位", DisplayName="是否可以交易", meta=(AllowPrivateAccess="true"))
	bool bCanTrade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品槽位", DisplayName="物品价值倍率", meta=(AllowPrivateAccess="true", ClampMin="0.0", UIMin="0.0"))
	float ItemValueRate = 1.0f;

	int32 SlotIndex = INDEX_NONE;
	
	
};
