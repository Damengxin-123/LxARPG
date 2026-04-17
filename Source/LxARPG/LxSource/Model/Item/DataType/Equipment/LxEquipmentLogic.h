// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxEquipmentLogic.generated.h"

/**
 * 
 */
UCLASS()
class LXARPG_API ULxEquipmentLogic : public ULxItemLogicBase
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化装备逻辑
	 *
	 * 该方法用于初始化装备的逻辑，包括设置装备的基础信息、稀有度信息等。
	 *
	 * @param pItemInfo 指向物品定义基础类型的指针，包含了物品的各种属性信息
	 * @return 如果成功初始化了装备逻辑，则返回true；否则返回false。失败的情况包括传入的pItemInfo为空或其ItemType不是装备类型等情况。
	 */
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;

	/**
	 * @brief 获取装备的数据基
	 *
	 * 该方法用于获取当前装备的完整数据基，包括基础信息、堆叠信息、数量、可视化信息和稀有度信息等。
	 *
	 * @return 返回一个指向FLxItemDateBase类型的指针，包含了装备的所有属性信息。如果装备逻辑未正确初始化或数据为空，则返回空指针。
	 */
	virtual FLxItemDateBase* GetItemDataBase() override;

	/**
	 * @brief 使用装备
	 *
	 * 该方法用于实现装备的使用逻辑。对于装备类型，通常不涉及消耗使用的逻辑，因此默认返回false。
	 *
	 * @return 如果装备成功被使用则返回true，否则返回false。对于装备，默认总是返回false。
	 */
	virtual bool UseItem() override;

	/**
	 * @brief 获取装备的数据
	 *
	 * 该方法用于获取当前装备的详细数据，包括基础信息、词条信息等。
	 *
	 * @return 返回一个指向FLxEquipmentData类型的指针，包含了装备的所有属性信息。如果装备逻辑未正确初始化或数据为空，则返回空指针。
	 */
	FLxEquipmentData* GetEquipmentData();

	const FLxEquipmentData* GetEquipmentData() const { return &m_EquipmentData; }
private:
	FLxEquipmentData m_EquipmentData;
};

