// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuffEnum.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxBuff.generated.h"

class ULxItemEntryLogic;

/**
 * @brief Buff 静态信息。
 *
 * Buff 作为物品类型时使用的基础数据结构，继承通用物品静态信息，
 * 并额外记录 Buff 子类型。
 */
USTRUCT(BlueprintType, DisplayName="Buff物品信息")
struct FLxBuffInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** @brief Buff 类型，例如常驻型、时效型、属性增益型等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|Buff", DisplayName="Buff类型")
	ELxBuffType BuffType = ELxBuffType::None;
	// Buff 词条等待词条系统重新设计后再接入。

	FLxBuffInformation()
	{
		ItemCount = 1;
		ItemCountMax = 1;
		ItemType = ELxItemType::Buff;
		ItemIDTag = LxTag_Item_Buff;
	}
};

/**
 * @brief Buff 物品对象。
 *
 * UObject 化后的 Buff 物品类型，负责提供 Buff 作为物品时的通用接口实现。
 * 当前 Buff 不支持堆叠，数量通常固定为 1。
 */
UCLASS(BlueprintType)
class LXARPG_API ULxBuff : public ULxItemBase
{
	GENERATED_BODY()

public:
	ULxBuff();
	virtual ~ULxBuff() override;
	
	/** @brief 使用 Buff 时触发词条激活流程。 */
	virtual ELxItemUseState ItemUse() override;
	/** @brief Buff 默认不显示数量文本。 */
	virtual FLxString ItemCountText() override;

	/** Buff物品是否仍然有效。 */
	bool IsBuffValid() const { return m_fBuffInformation.ItemCount > 0 && m_fBuffInformation.ItemIDTag.IsValid(); }

	/** 设置运行时剩余时间，供 Buff UI 通过 ItemCountText 显示。小于 0 表示永久 Buff。 */
	void SetRemainingDuration(float InRemainingDuration);

	/** 获取运行时剩余时间。小于 0 表示永久 Buff。 */
	float GetRemainingDuration() const { return RemainingDuration; }

protected:
	virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;
private:
	/** @brief 当前 Buff 物品的静态信息。 */
	FLxBuffInformation m_fBuffInformation;

	/** 运行时剩余时间，由 Buff 组件负责同步。 */
	float RemainingDuration = -1.f;
};
