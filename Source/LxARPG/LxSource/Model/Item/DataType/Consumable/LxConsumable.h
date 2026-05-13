// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxConsumableEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxConsumable.generated.h"

/**
 * @brief 消耗品静态信息。
 *
 * 消耗品物品在新物品体系中的基础数据结构，继承通用物品静态信息，
 * 并额外记录消耗品子类型。
 */
USTRUCT(BlueprintType, DisplayName="消耗品物品信息")
struct FLxConsumableInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** @brief 消耗品类型，例如回复类、功能类等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|消耗品", DisplayName="消耗品类型")
	ELxConsumableType ConsumableType = ELxConsumableType::None;

	FLxConsumableInformation()
	{
		ItemType = ELxItemType::Consumable;
		ItemIDTag = LxTag_Item_Consumable;
	}
	FLxConsumableInformation(FGameplayTag InItemIDTag, ELxItemRarityType RarityType,
		ELxConsumableType ConsumableType, FLxItemCount CountMax, const TArray<FLxEntryQuote>& EntryQuoteList = TArray<FLxEntryQuote>())
	{
		this->ItemIDTag = InItemIDTag;
		this->ItemRarity  = RarityType;
		this->ItemCountMax  = CountMax;
		this->ConsumableType  = ConsumableType;
		this->ItemEntryQuotes = EntryQuoteList;
		
		ItemType = ELxItemType::Consumable;
	}
};

/**
 * @brief 消耗品物品对象。
 *
 * UObject 化后的消耗品类型，负责提供消耗品的通用物品接口实现。
 * 支持同 ID 物品堆叠，堆叠成功后会广播数量变化事件。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxConsumable : public ULxItemBase
{
	GENERATED_BODY()

public:
	ULxConsumable();
	virtual ~ULxConsumable() override;


	virtual ELxItemUseState ItemUse() override;

	/** @brief 获取消耗品数量文本。 */
	virtual FLxString ItemCountText() override;

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;
private:
	/** @brief 当前消耗品的静态信息。 */
	FLxConsumableInformation m_fConsumableInformation;
};
