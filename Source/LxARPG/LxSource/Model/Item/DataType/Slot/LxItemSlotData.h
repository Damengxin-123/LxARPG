#pragma once

#include "CoreMinimal.h"
#include "LxItemSlotEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxItemSlotData.generated.h"

UENUM(BlueprintType)
enum class ELxItemSlotDropResult : uint8
{
	// 交换成功 指双方进行了直接交换
	Swapped					UMETA(DisplayName="交换成功"),
	// 完全堆叠	值发起方物品完全堆叠到目标方
	StackedAll				UMETA(DisplayName="完全堆叠"),
	// 部分堆叠  指发起方物品堆叠到目标方之后，还剩了一些
	StackedPartial			UMETA(DisplayName="部分堆叠"),
	// 来源无效  指发起方没有物品
	FailedInvalidSource		UMETA(DisplayName="来源无效"), 
	// 不可进入  指目标方不能放置发起方物品
	CannotEnter				UMETA(DisplayName="不可进入"),
	// 类型错误 指发起方与目标方槽位类型不一致 特指具体的物品类型，如装备部位
	TypeError				UMETA(DisplayName="不可进入"),
	// 进入成功  指发起方物品直接进入目标方空槽位
	EnterSuccess			UMETA(DisplayName="进入成功"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLxItemSlotChanged);

/* 物品槽位类型
 * 用于在提供一个在物品数据和UI物品格子之间提供中介交互的功能，
 * 应当包含以下功能：
 *		1、设置物品、清空物品
 *		2、提供外来物品是否能进入此槽位的判断接口
 *		3、提供外来物品进入此槽位的接口：在内部判断是拒绝、堆叠、交换位置
 */
UCLASS(BlueprintType, DisplayName="物品槽位数据类型")
class LXARPG_API ULxItemSlotData : public UObject
{
	GENERATED_BODY()
public:
	/**
 	 * @brief 槽位内容发生变化时触发的事件。
 	 *
 	 * 当槽位中的物品被添加、移除或替换时，会广播此委托。这允许外部组件（如UI元素）监听槽位的变化，并根据需要更新其状态或显示。
 	 *
 	 * @note 此委托在 `NotifySlotChanged` 函数中被调用，该函数通常在槽位数据发生变更后自动执行。
 	 *
 	 * @see ULxItemSlotData::NotifySlotChanged
 	 */
	UPROPERTY(BlueprintAssignable, DisplayName="槽位变化事件")
	FOnLxItemSlotChanged OnSlotChanged;

	
	/**
	 * @brief 检查当前槽位数据是否有效。
	 *
	 * 该方法用于验证存储在槽位中的物品数据是否有效。一个有效的槽位数据意味着它关联的物品数据指针非空且该物品本身是有效的。
	 *
	 * @return 如果槽位中的物品数据有效，则返回 `true`；否则返回 `false`。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="当前槽位内物品是否有效")
	bool IsValid() const;

	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="使用物品")
	virtual void UseItem();

	/**
 	 * @brief 设置当前槽位的物品数据。
 	 *
 	 * 该方法用于将指定的物品逻辑基础数据设置到当前槽位中。如果传入的`InItemData`为nullptr，表示清空该槽位；如果`InItemData`不为空但其有效性检查失败（即`ItemIsValid()`返回false），则不会设置此物品并返回false。
 	 *
 	 * @param InItemData 指向要设置到槽位中的物品逻辑基础数据的指针。可以为nullptr，表示清空槽位。
 	 * @return 如果成功设置了物品或清空了槽位，则返回true；如果尝试设置无效的物品数据，则返回false。
 	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="初始化槽位内物品")
	bool SetItem(ULxItemBase* InItemData);

	/**
 	 * @brief 清除当前槽位中的物品数据。
 	 *
 	 * 调用此方法后，槽位将不再持有任何物品引用，其内部的物品指针会被设置为 `nullptr`。这通常用于在移除或替换物品时清理槽位状态。
 	 *
 	 * @note 此操作不会影响实际的物品对象，仅解除槽位与物品之间的关联。
 	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="清空槽位")
	void ClearItem();

	/**
	 * @brief 检查给定的槽位是否可以堆叠。
	 *
	 * 该方法用于确定传入的 `ULxItemBase` 槽位中的项目是否能够与当前槽位中的项目进行堆叠。如果两个项目相同且支持堆叠，则返回 `true`，否则返回 `false`。
	 *
	 * @param InItemSlot 指向要检查的 `ULxItemBase` 槽位的指针。
	 * @return 如果 `InItemSlot` 中的项目可以与当前槽位中的项目堆叠，则返回 `true`；否则返回 `false`。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="判断是否可以与此物品槽内的物品进行堆叠")
	virtual bool IsStack(ULxItemBase* InItemSlot);

	/**
 	 * @brief 检查给定的物品是否可以被当前槽位接受。
 	 *
 	 * 该函数用于验证传入的 `InItemData` 是否可以放置在当前槽位中。首先检查 `InItemData` 是否为空或无效，如果满足这些条件则直接返回 `false`。接着，通过调用 `IsItemTypeCompatible` 方法进一步判断物品类型是否与当前槽位兼容。
 	 *
 	 * @param InItemData 指向待检查的物品逻辑对象的指针。
 	 * @return 如果 `InItemData` 可以被当前槽位接受，则返回 `true`；否则返回 `false`。
 	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="检查物品是否可以进入此槽位")
	virtual bool CanAcceptItem(ULxItemBase* InItemData) const;
	
	/**
	 * @brief 检查当前槽位是否可以获取其包含的物品数据。
	 *
	 * 该方法通过验证槽位内是否有有效的物品数据来判断是否可以从该槽位获取物品信息。如果槽位类型为 `None` 或 `Shortcut`，则不允许获取物品数据；对于其他类型的槽位（如背包、装备栏等），则允许获取。
	 *
	 * @return 如果槽位中存在有效物品数据且槽位类型允许，则返回 `true`；否则返回 `false`。
	 *
	 * @see ELxItemSlotType
	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="检查物品是否可以从槽位中提取")
	virtual bool CanGetItemData() const;

	/**
	 * @brief 尝试将另一个槽位中的物品移动到当前槽位。
	 *
	 * 该函数用于管理不同槽位之间的物品转移。它会检查指定槽位 (`InItemSlot`) 中的物品是否可以移动到当前槽位，并执行必要的操作以完成转移。
	 *
	 * @param InItemSlot 源槽位，从中移动物品。
	 * @return 如果物品成功移动到当前槽位，则返回 `true`；否则返回 `false`。
	 */
	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="让其他槽位中的物品进入此槽位")
	virtual ELxItemSlotDropResult ItemEnterToThis(ULxItemSlotData* InItemSlot);
public:
	/**
	 * @brief 指定该槽位的类型，用于定义物品槽位的行为和属性。
	 *
	 * 此枚举值决定了物品槽位的具体用途，例如背包、装备栏或仓库等。不同的槽位类型可能具有不同的限制和功能，比如是否允许交换位置或者是否可以放入物品。
	 *
	 * @note 默认设置为 `None`，表示一个未指定类型的槽位。
	 *
	 * @see ELxItemSlotType
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="槽位类型")
	ELxItemSlotType ItemSlotType = ELxItemSlotType::None;

	/**
	 * @brief 指定槽位的唯一标识符，用于区分不同的物品槽位。
	 *
	 * 此属性为每个槽位提供了一个唯一的ID，便于在代码中进行识别和操作。例如，在背包、装备栏或仓库等不同类型的槽位中，可以通过ID来定位具体的槽位并执行相关操作。
	 *
	 * @note 默认设置为 `INDEX_NONE`，表示一个未指定ID的槽位。
	 *
	 * @see ULxItemSlotData
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="槽位ID")
	int32 ID = INDEX_NONE;

	/**
	 * @brief 指向当前槽位中存储的物品逻辑对象。
	 *
	 * 该属性用于存储和访问当前槽位中的物品数据。`ItemDataPtr` 是一个指向 `ULxItemBase` 类型的指针，可以用来表示任何类型的物品逻辑。
	 * 通过这个指针，可以对槽位中的物品进行操作，例如获取物品信息、使用物品或检查物品的有效性。
	 *
	 * @note 默认设置为 `nullptr`，表示当前槽位为空，没有存储任何物品。
	 *
	 * @see ULxItemBase
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="物品数据")
	TObjectPtr<ULxItemBase> ItemDataPtr = nullptr;
};

UCLASS(BlueprintType, DisplayName="装备槽位数据")
class LXARPG_API ULxEquipmentSlotData : public ULxItemSlotData
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item Slot", DisplayName="装备类型")
	ELxEquipmentType EquipmentType = ELxEquipmentType::Weapon;

	virtual bool CanAcceptItem(ULxItemBase* InItemData) const override;
	
};

UCLASS(BlueprintType, DisplayName="快捷栏槽位数据")
class LXARPG_API ULxShortcutItemSlotData : public ULxItemSlotData
{
	GENERATED_BODY()

public:
	ULxShortcutItemSlotData();

	UFUNCTION(BlueprintCallable, Category="Item Slot", DisplayName="绑定快捷栏来源槽位")
	bool BindSourceSlot(ULxItemSlotData* InSourceSlot);

	/** @brief 获取快捷栏当前绑定的来源槽位。 */
	UFUNCTION(BlueprintPure, Category="Item Slot", DisplayName="获取快捷栏来源槽位")
	ULxItemSlotData* GetSourceSlot() const { return SourceSlot; }

	virtual void UseItem() override;

	virtual bool CanAcceptItem(ULxItemBase* InItemData) const override;

	virtual ELxItemSlotDropResult ItemEnterToThis(ULxItemSlotData* InItemSlot) override;

protected:
	/** @brief 来源槽位内容变化时刷新快捷栏自身显示。 */
	UFUNCTION()
	void HandleSourceSlotChanged();

	/** @brief 已绑定物品数量变化时刷新快捷栏自身显示。 */
	UFUNCTION()
	void HandleBoundItemChanged(ULxItemBase* Item, int32 OldCount, int32 NewCount);

	/** @brief 快捷栏引用的真实物品来源槽位。 */
	UPROPERTY()
	TObjectPtr<ULxItemSlotData> SourceSlot = nullptr;

	/** @brief 当前监听数量变化事件的物品对象。 */
	UPROPERTY()
	TObjectPtr<ULxItemBase> BoundItem = nullptr;
};

// UCLASS(BlueprintType, DisplayName="技能槽位数据")
// class LXARPG_API ULxSkillSlotData : public ULxItemSlotData
// {
// 	GENERATED_BODY()
// public:
// };
