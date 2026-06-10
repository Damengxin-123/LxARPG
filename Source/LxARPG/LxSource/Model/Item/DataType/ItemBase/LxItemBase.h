// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxItemEnmuType.h"
#include "LxItemInformationBase.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "UObject/Object.h"
#include "LxItemBase.generated.h"

class UTexture2D;

/**
 * 物品数量变化事件。
 *
 * 当物品内部数量发生变化时广播，背包格子、快捷栏和数量文本可以监听它刷新显示。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemCountChanged, ULxItemBase*, Item);

/**
 * 运行时物品词条信息。
 *
 * 用于在运行时同时保存词条对象和该词条在物品修改逻辑中的分类。
 */
USTRUCT(BlueprintType, DisplayName="运行时物品词条信息")
struct FLxItemEntryRuntimeInfo
{
	GENERATED_BODY()

	/** 运行时词条对象。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="词条", DisplayName="运行时词条对象")
	TObjectPtr<ULxEntryObjectBase> EntryObject = nullptr;

	/** 词条逻辑类型。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="词条", DisplayName="词条逻辑类型")
	ELxEntryLogicType EntryLogicType = ELxEntryLogicType::Normal;
};

/**
 * UObject 化后的运行时物品基类。
 *
 * 运行时物品对象只保存当前数量、词条对象和一份从数据表拷贝来的静态配置；
 * 真正的物品定义由 LxItemConfig 在数据表加载后统一提供。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxItemBase : public UObject
{
	GENERATED_BODY()

public:
	/** 物品数量改变时触发。 */
	UPROPERTY(BlueprintAssignable, Category="物品", DisplayName="物品数量改变事件")
	FOnItemCountChanged OnItemCountChanged;

	static ULxItemBase* CreateItemObject(UObject* InParent, FLxItemQuote InItemQuote);

	/** 使用物品引用初始化运行时物品对象。 */
	void InitItemObject(const FLxItemQuote& InItemQuote);

	/** 当前物品是否还可以和同ID物品堆叠。 */
	bool ItemIsStackable();

	/** 获取物品标签 ID。 */
	FGameplayTag ItemIDTag();

	/** 获取物品大类，例如装备、消耗品、材料等。 */
	ELxItemType ItemType();

	/** 获取物品稀有度。 */
	ELxItemRarityType ItemRarity();

	/** 获取物品 UI 显示名称，数据来源为物品基础结构体。 */
	FText ItemDisplayName();

	/** 获取物品 UI 显示描述，数据来源为物品基础结构体。 */
	FText ItemDisplayDescription();

	/** 获取物品图标软引用，UI 可按需同步加载或异步加载。 */
	TSoftObjectPtr<UTexture2D> ItemIcon();

	/** 获取当前物品的基础信息副本，用于 UI 显示。 */
	FLxItemInformationBase ItemInformation();

	/** 获取当前物品数量。 */
	FLxItemCount ItemCount();

	/** 尝试把传入物品堆叠到当前物品上。 */
	bool ItemStack(ULxItemBase* InItem);

	/** 当前物品数据是否有效。 */
	bool ItemIsValid();

	/** 按物品类型和稀有度排序。 */
	bool operator<(ULxItemBase& InItem);

	/** 按物品类型和稀有度排序。 */
	bool operator>(ULxItemBase& InItem);

	/** 按物品标签 ID 判断是否为同一种物品。 */
	bool operator==(ULxItemBase& InItem);

	/** 初始化物品词条对象，需在静态物品数据设置完成后调用。 */
	void InitItemEntry();

	/** 获取物品词条对象列表。 */
	TArray<TObjectPtr<ULxEntryObjectBase>>& GetItemEntryList();

	/** 获取带有逻辑类型的运行时物品词条信息列表。 */
	const TArray<FLxItemEntryRuntimeInfo>& GetItemEntryRuntimeInfoList() const;

	void BroadcastItemCountChanged();

	/** 获取用于 UI 显示的数量文本。 */
	virtual FLxString ItemCountText() PURE_VIRTUAL(ULxItemBase::ItemCountText, return FLxString(););

	/** 使用物品后返回对应的使用结果。 */
	virtual ELxItemUseState ItemUse() PURE_VIRTUAL(ULxItemBase::ItemUse, return ELxItemUseState::Failed;);

	/** 开始使用物品。用于按住输入、持续使用、技能蓄力等需要按下边沿的物品。 */
	virtual ELxItemUseState ItemUseStart();

	/** 结束使用物品。普通物品默认在抬起时执行一次性使用。 */
	virtual ELxItemUseState ItemUseEnd();

protected:
	/** 广播物品数量改变事件，只有新旧数量不一致时才会真正通知。 */
	/** 设置物品静态数据和运行时数量。 */
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount);

	/** 获取当前运行时物品保存的静态配置副本。 */
	virtual FLxItemInformationBase* ItemBase(){ return nullptr; };

private:
	/**
	 * 物品词条对象缓存。
	 *
	 * 数据来源于物品静态配置中的 ItemEntryConfigs，创建后由 UI、属性组件或效果组件读取。
	 */
	UPROPERTY()
	TArray<TObjectPtr<ULxEntryObjectBase>> ItemEntryArray;

	/** 带有逻辑类型的物品词条对象缓存，供 UI 和后续物品修改功能按类型读取。 */
	UPROPERTY()
	TArray<FLxItemEntryRuntimeInfo> ItemEntryRuntimeInfoArray;
};
