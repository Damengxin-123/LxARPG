// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxMaterial.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxMaterialLogic.generated.h"

UCLASS()
class LXARPG_API ULxMaterialLogic : public ULxItemLogicBase
{
	GENERATED_BODY()

	/**
	 * @brief 初始化材料逻辑
	 *
	 * 该方法用于根据提供的物品定义信息初始化材料逻辑。只有当提供的`pItemInfo`指针非空且指向的物品类型为材料时，初始化才会成功。
	 *
	 * @param pItemInfo 指向物品定义基础类型的常量指针，包含了待初始化材料的所有必要信息。
	 * @return 如果初始化成功返回true，否则返回false。失败的情况包括但不限于`pItemInfo`为空或其表示的不是材料类型。
	 */
public:
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	/**
	 * @brief 获取物品数据基础
	 *
	 * 该方法返回一个指向内部存储的`FLxItemDateBase`结构体的指针，该结构体包含了物品的所有相关属性信息。
	 *
	 * @return 返回一个指向`FLxItemDateBase`的指针，通过该指针可以访问和修改物品的基础属性、堆叠信息、数量、可视化信息以及稀有度信息等。
	 */
	virtual FLxItemDateBase* GetItemDataBase() override;
	/**
	 * @brief 使用物品
	 *
	 * 该虚函数用于实现材料的使用逻辑。默认实现返回false，表示材料未被使用。
	 * 子类应根据具体的材料类型重写此方法以提供实际的使用逻辑。
	 *
	 * @return 如果材料成功被使用则返回true，否则返回false。
	 */
	virtual bool UseItem() override;

private:
	FLxMaterialData m_MaterialData;
};

