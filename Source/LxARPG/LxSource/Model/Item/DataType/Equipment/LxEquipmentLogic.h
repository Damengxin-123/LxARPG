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
	// 使用通用物品定义初始化装备逻辑（内部按 FLxEquipmentDefine 解析）
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	// 获取装备数据的基类视图
	virtual const FLxItemDateBase* GetItemDataBase() const override;
	// 装备道具当前不支持“直接使用”
	virtual bool UseItem() override;
	// 是否可堆叠
	virtual bool ItemIsStack() override;
	// 与另一个物品逻辑对象尝试堆叠
	virtual bool StackItem(ULxItemLogicBase* SourceItemLogic) override;
	// 当前装备数据是否有效
	virtual bool ItemIsValid() override;
	// 排序比较（用于背包排序等）
	virtual bool operator<(const ULxItemLogicBase* Other) const override;
	// 排序比较（用于背包排序等）
	virtual bool operator>(const ULxItemLogicBase* Other) const override;

private:
	FLxEquipmentData m_EquipmentData;
};
