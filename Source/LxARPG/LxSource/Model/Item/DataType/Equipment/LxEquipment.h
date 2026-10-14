// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipmentEnum.h"
#include "LxEquipmentState.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxEquipment.generated.h"

/**
 * @brief 装备静态信息。
 *
 * 装备物品在新物品体系中的基础数据结构，继承通用物品静态信息，
 * 并额外记录装备部位、固定词条开关、锻造候选池及运行时强化度。
 */
USTRUCT(BlueprintType, DisplayName="装备物品信息")
struct FLxEquipmentInformation : public FLxItemInformationBase
{
	GENERATED_BODY()

	/** @brief 装备部位类型，例如武器、饰品等。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="装备部位")
	ELxEquipmentType EquipmentType = ELxEquipmentType::EquipmentSizeMax;
    /** 固定装备的普通词条来自静态配置，只允许强化。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="固定普通词条")
    bool bFixedNormalEntries = false;

    /** 新增词条的静态候选池，剔除装备已经拥有的词条后等概率抽取。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品|装备", DisplayName="锻造词条池", meta=(Categories="词条"))
    TArray<FGameplayTag> ForgeEntryPool;

    /** 当前强化度仅用于运行时显示，不写入静态词条。 */
    UPROPERTY(Transient, BlueprintReadOnly, Category="物品|装备", DisplayName="强化度")
    float Enhancement = 0.f;

    /** 旧资产的强化初值（百分数），仅保留反序列化兼容。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="请使用装备实例强化度"))
	int32 EnhancementValueOverride = 0;
	/** 旧资产的锻造潜能，仅保留反序列化兼容，不参与打造。 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="锻造不再消耗潜力"))
	int32 ForgePotential = 1;
	
	
	

	FLxEquipmentInformation()
	{
		ItemCount = 1;
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
	 * @param DefaultEntryConfigList 初始化时提供的词条配置列表，每条词条都带有后续物品修改系统使用的逻辑类型。
	 * @return 无返回值。
	 */
	FLxEquipmentInformation(FGameplayTag InItemIDTag, ELxItemRarityType RarityType,
	                        ELxEquipmentType EquipmentType, int32 EnhancementValueOverride, int32 ForgePotential, const TArray<FLxItemEntryConfig>& DefaultEntryConfigList = TArray<FLxItemEntryConfig>())
	{
		this->ItemIDTag = InItemIDTag;
		this->ItemRarity  = RarityType;
		this->EquipmentType = EquipmentType;
		this->EnhancementValueOverride = EnhancementValueOverride;
		this->ForgePotential = ForgePotential;
		this->ItemEntryConfigs = DefaultEntryConfigList;

		ItemCount = 1;
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
UCLASS(BlueprintType, DisplayName="装备实例")
class LXARPG_API ULxEquipment : public ULxItemBase
{
	GENERATED_BODY()

public:
    /** 普通词条数量上限，稀有度阈值统一按此值计算。 */
    static constexpr int32 MaxNormalEntries = 20;

    /** 创建装备的静态信息容器。 */
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

    /** 获取装备实例标识，同一物品ID的多件装备各自独立。 */
    UFUNCTION(BlueprintPure, Category="装备", DisplayName="获取装备实例ID")
    FGuid GetInstanceID() const { return InstanceID; }

    /** 获取当前强化度，0.3表示总增30%。 */
    UFUNCTION(BlueprintPure, Category="装备", DisplayName="获取强化度")
    float GetEnhancement() const { return m_fEquipmentInformation.Enhancement; }

    /** 查询装备是否锁定全部普通词条。 */
    UFUNCTION(BlueprintPure, Category="装备", DisplayName="是否固定词条装备")
    bool HasFixedEntries() const { return m_fEquipmentInformation.bFixedNormalEntries; }

    /** 获取未经强化缩放的普通词条。 */
    UFUNCTION(BlueprintPure, Category="装备", DisplayName="获取普通词条")
    TArray<FLxEquipmentEntry> GetNormalEntries() const;

    /** 提取最小装备状态，不包含静态基础词条。 */
    FLxEquipmentSaveRecord CaptureEquipmentState() const;
    /** 验证并恢复装备状态；失败不会修改当前装备。 */
    bool RestoreEquipmentState(const FLxEquipmentSaveRecord& State, const FGuid& InInstanceID);
    /** 迁移旧物品存档，只接受普通词条，基础词条始终读取静态配置。 */
    virtual bool RestoreItemEntries(const TArray<FLxItemEntryConfig>& InEntries) override;
    /** 原子更新强化和普通词条，固定装备拒绝词条变化。 */
    bool ApplyCraftingState(float Enhancement, const TArray<FLxEquipmentEntry>& Entries);
    /** 按普通词条数量计算装备稀有度，支持边界测试与界面预览。 */
    static ELxItemRarityType CalculateRarity(bool bFixed, int32 Count);

protected:
    /** 生成显示文本与实际效果包时乘以强化倍率，词条配置和对象缓存保留原值。 */
    virtual float GetEntryEffectMultiplier() const override { return 1.f + GetEnhancement(); }
    /** 初始化装备模板及其独立实例标识。 */
    virtual void SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount) override;

	virtual FLxItemInformationBase* ItemBase() override;
private:
    /** 按静态基础词条和未经缩放的普通词条重建运行时效果。 */
    bool RebuildEquipment(float Enhancement, const TArray<FLxEquipmentEntry>& Entries);
    /** 每件装备的稳定身份，跨容器移动时不变 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="每件装备的稳定身份，跨容器移动时不变")
    FGuid InstanceID;
    /** @brief 当前装备的静态信息及未经缩放的词条。 */
	FLxEquipmentInformation m_fEquipmentInformation;
};
