// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemDataType.h"
#include "LxEquipmentData.generated.h"

/**
 * @struct FLxEquipmentPartInfo
 * @brief 表示装备槽位信息的结构体
 *
 * 继承自FLxStructData，此类型用于在数据表中配置装备栏类型及数量信息，不可直接用于数据存储。
 *
 * @note 可在蓝图中使用
 */
USTRUCT(BlueprintType, DisplayName="装备部位信息")
struct FLxEquipmentPartInfo : public FLxTableRowBase
{
	GENERATED_BODY()

	/**
	 * @var ELxEquipmentType EquipmentType
	 * @brief 装备类型
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备部位|装备类型", DisplayName="装备类型")
	ELxEquipmentType EquipmentType;

	/**
	 * @var TSoftObjectPtr<UTexture2D> EquipmentDefaultIcon
	 * @brief 默认图标
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备部位|默认图标", DisplayName="默认图标")
	TSoftObjectPtr<UTexture2D> EquipmentDefaultIcon;
};

/**
 * @struct FLxEquipmentData
 * @brief 表示装备属性的结构体
 *
 * 继承自FLxItemAttributes，用于定义装备类型的属性，并在数据表中使用该结构体来存储装备数据。
 *
 * @note
 * - 本结构体用于定义装备类型的属性，并在数据表中使用该结构体来存储装备数据
 * - 本类型不与其他类型进行关联
 * - 本类型ID为行名称
 */
USTRUCT(BlueprintType, DisplayName="装备属性")
struct FLxEquipmentData : public FLxItemBase
{
	GENERATED_BODY()


	// ==================== 装备基础属性，用于提前设计数据表  ====================

	/**
	 * @var ELxEquipmentType EquipmentType
	 * @brief 装备部位类型
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="装备部位")
	ELxEquipmentType EquipmentType;

	/**
	 * @var double StrengthenValue
	 * @brief 装备的强化强度
	 *
	 * 百分比值，表示装备中所有属性的提高比例。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="强化强度")
	double StrengthenValue = 0.0;

	/**
	 * @var double ForgingPotential
	 * @brief 锻造潜能
	 *
	 * 通过锻造可以改变装备的扩展词条的数量及强度，但是会随机消耗锻造潜能。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备基础属性", DisplayName="锻造潜能")
	double ForgingPotential = 0.0;

	/**
	 * @var TArray<FLxItemEntryName> EquipmentBaseEntries
	 * @brief 装备基础属性词条 每件装备上固定的一条词条，不可被更改种类，可以被调整数值
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备词条", DisplayName="基础词条")
	FLxItemEntryName EquipmentBaseEntrie;

	/**
	 * @var TArray<FLxItemEntryName> EquipmentExtendEntries
	 * @brief 装备扩展词条 每件装备上可以有无数条词条，可以被增删改，可以被调整数值
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备词条", DisplayName="扩展词条")
	TArray<FLxItemEntryName> EquipmentExtendEntrieList;

	// ==================== 装备数据实例化之后填充  ====================

	/**
	 * @var TArray<FLxItemEntry> BasicEntrys
	 * @brief 装备的基础词条
	 */
	FLxItemEntry BasicEntry;

	/**
	 * @var TArray<FLxItemEntry> ExtendEntrys
	 * @brief 装备的扩展词条
	 */
	TArray<FLxItemEntry> ExtendEntryList;


	/**
	 * @brief 装备属性默认构造函数
	 *
	 * 初始化装备的默认属性值，包括装备类型、物品类型、最大堆叠数量等。
	 */
	FLxEquipmentData()
	{
		EquipmentType = ELxEquipmentType::EquipmentSizeMax;
		// 设置装备的默认属性值
		ItemType = ELxItemType::Equipment;
		ItemMaxCount = 1;
		ItemCanStack = false;
	}

	/**
	 * @brief 获取装备类型的名称
	 *
	 * 根据当前装备的类型返回相应的中文名称。
	 *
	 * @return 返回装备类型的中文名称，如果类型未知则返回"未知"
	 */
	virtual FString GetItemTypeName() const override
	{
		
		switch (EquipmentType)
		{
		case ELxEquipmentType::Weapon:
			return TEXT("主手武器");
		case ELxEquipmentType::Deputy:
			return TEXT("副手");
		case ELxEquipmentType::Helmet:
			return TEXT("头盔");
		case ELxEquipmentType::Armor:
			return TEXT("胸甲");
		case ELxEquipmentType::Leggings:
			return TEXT("护腿");
		case ELxEquipmentType::Boots:
			return TEXT("鞋子");
		case ELxEquipmentType::Glove:
			return TEXT("手套");
		case ELxEquipmentType::Belt:
			return TEXT("腰带");
		default:
			return TEXT("未知");
		};
	}

	/**
	 * @brief 初始化装备数据
	 *
	 * 该方法根据基础词条和扩展词条列表，初始化装备的词条属性。对于每个词条，它会从数据表中获取对应的行数据，并根据装备的基础强度和强化值调整词条数值，然后将调整后的词条添加到基础词条或扩展词条列表中。
	 *
	 * @note 此方法重写了基类的方法，确保在创建装备实例时正确初始化其属性。
	 */
	virtual void InitData() override
	{
		// 根据词条列表，初始化词条属性列表
		if (const FLxItemEntry* RowData = EquipmentBaseEntrie.EntryRow.GetRow<FLxItemEntry>(TEXT("FLxEquipmentAttributes::InitData")))
		{
			FLxItemEntry NewEntry = *RowData;
			NewEntry = NewEntry * EquipmentBaseEntrie.EntryStrength;
			NewEntry = NewEntry * (1 + (StrengthenValue / 100));
			NewEntry.InitData();
			BasicEntry = NewEntry;
		}
		
		for (auto& Entrie : EquipmentExtendEntrieList)
		{
			if (const FLxItemEntry* RowData = Entrie.EntryRow.GetRow<FLxItemEntry>(TEXT("FLxEquipmentAttributes::InitData")))
			{
				FLxItemEntry NewEntry = *RowData;
				NewEntry = NewEntry * Entrie.EntryStrength;
				NewEntry = NewEntry * (1 + (StrengthenValue / 100));
				NewEntry.InitData();
				ExtendEntryList.Add(NewEntry);
			}
		}
	};

	
};
