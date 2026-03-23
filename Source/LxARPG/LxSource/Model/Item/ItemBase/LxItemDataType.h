#pragma once
#include "CoreMinimal.h"
#include "LxItemEnmuType.h"
#include "LxRarityInfoData.h"
#include "Engine/Texture2D.h"
#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Core/Database/LxTableRowBase.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxItemDataType.generated.h"




/**
 * @brief 物品基础属性结构体
 * 该结构体继承自FLxTableRowBase，用于定义物品的基本属性。
 */
USTRUCT(BlueprintType, DisplayName="物品基础属性")
struct FLxItemBase : public FLxTableRowBase
{
	GENERATED_BODY()

	/**
	 * @brief 物品类型
	 * 该枚举用于定义物品的类型，包括装备、消耗品、材料、技能和状态等。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品类型")
	ELxItemType ItemType = ELxItemType::None;

	/**
	 * @brief 物品稀有度数据表行句柄
	 * 该变量用于引用定义在LxRarityInfo数据表中的特定稀有度信息。通过此句柄，可以访问与物品稀有度相关的各种属性，如名称、颜色或掉落概率等。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品稀有度",  meta=(RowType="LxRarityInfo"))
	FDataTableRowHandle ItemRarityRow;

	/**
	 * @brief 物品数量
	 * 该变量表示物品的数量，用于记录特定物品的当前数量。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 * 默认值为ERR_ATTRIBUTE，表示未定义或错误的状态。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品数量")
	int32 ItemCount = ERR_ATTRIBUTE;

	/**
	 * @brief 物品最大堆叠数量
	 * 该变量定义了单个物品类型在库存中可以堆叠的最大数量。默认值为99，表示每种物品最多可以堆叠99个。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品最大堆叠数量")
	int32 ItemMaxCount = 99;

	/**
	 * @brief 物品是否可以堆叠
	 * 该布尔变量用于指示物品是否可以在库存中堆叠。如果设置为`true`，则相同类型的物品可以堆叠在一起以节省空间；如果设置为`false`，则每个物品都必须单独占用一个槽位。
	 * 可以在编辑器中设置，并且可以通过蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品基础属性", DisplayName="物品是否可以堆叠")
	bool ItemCanStack = true;

	/**
	 * @brief 物品图标
	 * 该变量用于存储物品图标的引用，可以在编辑器中设置，并且支持蓝图读写。
	 * 通过此变量可以访问和显示物品的图标，以便在游戏中可视化地表示物品。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品图标")
	TSoftObjectPtr<UTexture2D> ItemIconPath;

	/**
	 * @brief 物品可视化名称
	 * 该变量用于存储物品的显示名称，以便在游戏中显示给玩家。可以通过编辑器设置，并且支持蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化名称")
	FText ItemText;

	/**
	 * @brief 物品可视化描述
	 * 该变量用于存储物品的详细描述文本，以便在游戏中显示给玩家。可以通过编辑器设置，并且支持蓝图读写。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "物品可视化信息", DisplayName="物品可视化描述")
	FText ItemDescription;
	/**
	 * @brief 物品稀有度信息
	 * 该变量存储了与物品稀有度相关的信息，包括稀有度的名称、颜色、掉落概率等属性。
	 * 通过RarityInfo可以访问和设置这些稀有度相关的属性，用于在游戏内标识和处理不同稀有度的物品。
	 */
	FLxRarityInfo RarityInfo;

	virtual void InitData() override
	{
		if (FLxRarityInfo* RarityInfoData = ItemRarityRow.GetRow<FLxRarityInfo>(TEXT("FLxItemAttributes::InitData")))
		{
			RarityInfo = *RarityInfoData;
		}
	}

	/**
	 * @brief重载操作符函数
	 *
	 * @param lhs 左操作数
	 * @param rhs 右操作数
	 * @return 返回操作结果
	 */
	bool operator<(const FLxItemBase& Other) const
	{
		// 先比较物品类型
		if (ItemType == Other.ItemType)
		{
			if (RarityInfo.RarityValue == Other.RarityInfo.RarityValue)
			{
				// 比较物品ID，ID更小的更靠前
				return FLxString(RowID).ToInt() < FLxString(Other.RowID).ToInt();
			}
			// 比较稀有度，稀有度更小的更靠前
			return RarityInfo.RarityValue < Other.RarityInfo.RarityValue;
		}
		// 比较物品类型，类型更小的更靠前
		return ItemType < Other.ItemType;
	}

	/**
	 * @brief重载运算符
	 *
	 * @param lhs 左操作数
	 * @param rhs 右操作数
	 * @return 返回运算结果
	 */
	bool operator>(const FLxItemBase& Other) const
	{
		return !(*this < Other);
	}

	/**
	 * @brief 检查当前对象是否有效
	 * 该方法用于验证当前对象的RowID是否不为"null"，从而判断对象是否有效。
	 * @return 如果RowID不是"null"，则返回true，表示对象有效；否则返回false。
	 */
	virtual bool IsValid()
	{
		return RowID != TEXT("null");
	};

	/**
	 * @brief 获取物品稀有度名称
	 * 该方法返回当前物品的稀有度名称。
	 *
	 * @return 返回表示物品稀有度的文本
	 */
	FText GetRarityName () const
	{
		return RarityInfo.RarityText;
	}

	/**
	 * @brief 获取物品类型名称
	 * 根据物品的类型枚举值返回对应的中文名称。
	 * @return 返回表示物品类型的字符串，如果类型未知，则返回"未知"。
	 */
	virtual  FString GetItemTypeName() const
	{
		switch (ItemType)
		{
			case ELxItemType::Equipment:
				return TEXT("装备");
			case ELxItemType::Consumable:
				return TEXT("消耗品");
			case ELxItemType::Material:
				return TEXT("材料");
			case ELxItemType::Skill:
				return TEXT("技能");
			case ELxItemType::Buff:
				return TEXT("状态");
			default:
			return TEXT("未知");
			
		}
	}

	/**
	 * @brief 浣跨敤鐗╁搧
	 *
	 * 鐗╁搧浣跨敤鐨勮櫄鍑芥暟锛屽瓙绫诲彲閲嶅啓姝ゅ嚱鏁板疄鐜板叿浣撶殑浣跨敤閫昏緫銆?	 *
	 * @return 濡傛灉浣跨敤鎴愬姛鍒欒繑鍥瀟rue锛屽惁鍒欒繑鍥瀎alse
	 */
	virtual bool UseItem(){ return false;};
};
