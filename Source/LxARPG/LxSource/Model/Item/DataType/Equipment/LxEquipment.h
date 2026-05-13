// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxEquipment.generated.h"

/**
 * @brief 装备静态信息。
 *
 * 装备物品在新物品体系中的基础数据结构，继承通用物品静态信息，
 * 并额外记录装备部位、强化值和锻造潜能等装备专属属性。
 */
USTRUCT(BlueprintType, DisplayName="装备物品信息")
struct FLxEquipmentInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** @brief 装备部位类型，例如武器、饰品等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="装备部位")
	ELxEquipmentType EquipmentType = ELxEquipmentType::EquipmentSizeMax;
	/** @brief 装备强化值覆盖项。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="强化值覆盖")
	int32 EnhancementValueOverride = 0;
	/** @brief 装备锻造潜能。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="锻造潜力")
	int32 ForgePotential = 1;
	
	
	

	FLxEquipmentInformation()
	{
		ItemCountMax = 1;
		ItemType = ELxItemType::Equipment;
		ItemIDTag = LxTag_Item_Equipment;
	}

	/**
	 * @brief 构造函数，用于初始化装备静态信息。
	 *
	 * 该构造函数接收装备ID、稀有度类型、装备类型、强化值覆盖项、锻造潜能以及默认词条列表作为参数，
	 * 并使用这些参数来初始化一个`FLxEquipmentInformation`实例。此构造函数还处理了从给定的词条列表中
	 * 移除并设置第一个词条为默认词条的操作，并将剩余的词条存储在扩展词条列表中。
	 *
	 * @param ID 装备的唯一标识符。
	 * @param RarityType 物品的稀有度级别。
	 * @param EquipmentType 装备所属的具体部位类型。
	 * @param EnhancementValueOverride 强化值覆盖项，用于指定装备的特定强化等级。
	 * @param ForgePotential 装备的潜在锻造能力，影响装备可被锻造的程度或次数。
	 * @param DefaultEntryQuoteList 初始化时提供的词条列表，其中第一个元素将被设为默认词条，其余则添加至扩展词条列表。
	 * @return 无返回值。
	 */
	FLxEquipmentInformation(FGameplayTag InItemIDTag, ELxItemRarityType RarityType,
	                        ELxEquipmentType EquipmentType, int32 EnhancementValueOverride, int32 ForgePotential, const TArray<FLxEntryQuote>& DefaultEntryQuoteList = TArray<FLxEntryQuote>())
	{
		this->ItemIDTag = InItemIDTag;
		this->ItemRarity  = RarityType;
		this->EquipmentType = EquipmentType;
		this->EnhancementValueOverride = EnhancementValueOverride;
		this->ForgePotential = ForgePotential;
		this->ItemEntryQuotes = DefaultEntryQuoteList;

		ItemCountMax = 1;
		ItemType = ELxItemType::Equipment;
	}
};

/**
 * @brief 装备物品对象。
 *
 * UObject 化后的装备类型，负责提供装备物品的通用接口实现，
 * 并通过 FLxEquipmentInformation 保存当前装备的静态属性。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxEquipment : public ULxItemBase
{
	GENERATED_BODY()

public:
	ULxEquipment();
	virtual ~ULxEquipment() override;
	
	/** @brief 使用装备时返回安装装备的使用状态。 */
	virtual ELxItemUseState ItemUse() override;

	/** @brief 装备数量文本，装备默认不显示数量。 */
	virtual FLxString ItemCountText() override;

	/** @brief 获取装备部位类型。 */
	ELxEquipmentType EquipmentType() const;

	/** 获取当前装备信息副本，用于 UI 显示。 */
	FLxEquipmentInformation EquipmentInformation() const;

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;
private:
	/** @brief 当前装备的静态信息。 */
	FLxEquipmentInformation m_fEquipmentInformation;
};
