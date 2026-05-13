// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxMaterialEnum.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxMaterial.generated.h"

/**
 * @brief 材料静态信息。
 *
 * 材料物品在新物品体系中的基础数据结构，继承通用物品静态信息，
 * 并额外记录材料子类型。
 */
USTRUCT(BlueprintType, DisplayName="材料物品信息")
struct FLxMaterialInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** @brief 材料类型，例如普通材料、任务物品等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|材料", DisplayName="材料类型")
	ELxMaterialType MaterialType = ELxMaterialType::None;

	FLxMaterialInformation()
	{
		ItemType = ELxItemType::Material;
		ItemIDTag = LxTag_Item_Material;
	}
	FLxMaterialInformation(FGameplayTag InItemIDTag, ELxItemRarityType RarityType,
		ELxMaterialType MaterialType, FLxItemCount CountMax)
	{
		this->ItemIDTag = InItemIDTag;
		this->ItemRarity  = RarityType;
		this->ItemCountMax  = CountMax;
		this->MaterialType  = MaterialType;
		
		ItemType = ELxItemType::Material;
	}
};

/**
 * @brief 材料物品对象。
 *
 * UObject 化后的材料类型，负责提供材料物品的通用接口实现。
 * 支持同 ID 物品堆叠，堆叠成功后会广播数量变化事件。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxMaterial : public ULxItemBase
{
	GENERATED_BODY()

public:
	ULxMaterial();
	virtual ~ULxMaterial() override;
	
	/** @brief 材料默认不可直接使用。 */
	virtual ELxItemUseState ItemUse() override;

	/** @brief 获取材料数量文本。 */
	virtual FLxString ItemCountText() override;

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;
private:
	/** @brief 当前材料的静态信息。 */
	FLxMaterialInformation m_fMaterialInformation;
};
