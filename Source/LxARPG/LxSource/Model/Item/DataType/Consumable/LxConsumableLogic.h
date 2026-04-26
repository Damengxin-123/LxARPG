// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxConsumableLogic.generated.h"

UCLASS()
class LXARPG_API ULxConsumableLogic : public ULxItemLogicBase
{
	GENERATED_BODY()

	/**
	 * @brief 初始化物品逻辑
	 *
	 * 该方法用于根据给定的物品定义信息初始化消耗品逻辑。如果提供的物品信息为空或不匹配消耗品类别，则初始化失败。
	 *
	 * @param pItemInfo 指向物品定义基础类型的指针，包含了需要初始化的物品的所有信息
	 * @return 如果成功初始化则返回true，否则返回false
	 */
public:
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	/**
	 * @brief 获取物品数据
	 *
	 * 该方法返回一个指向当前消耗品逻辑中存储的物品数据的指针。此数据包含了物品的基础信息、堆叠信息、数量、可视化信息和稀有度信息。
	 *
	 * @return 返回一个指向FLxItemDateBase类型的指针，包含当前消耗品的所有数据
	 */
	virtual FLxItemDateBase* GetItemDataBase() override;
	/**
	 * @brief 使用物品
	 *
	 * 该方法用于执行消耗品的使用逻辑。默认实现返回false，表示物品未被使用。
	 * 子类应根据具体的消耗品类别重写此方法以提供实际的使用逻辑。
	 *
	 * @return 如果物品成功被使用则返回true，否则返回false
	 */
	virtual bool UseItem() override;

	/** 获取消耗品缓存数据。 */
	FLxConsumableData* GetConsumableData() { return &m_ConsumableData; }

	const FLxConsumableData* GetConsumableData() const { return &m_ConsumableData; }

private:
	UPROPERTY()
	FLxConsumableData m_ConsumableData;
};
