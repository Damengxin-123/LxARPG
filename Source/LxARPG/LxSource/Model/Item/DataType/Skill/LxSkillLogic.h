// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxSkill.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxSkillLogic.generated.h"

UCLASS()
class LXARPG_API ULxSkillLogic : public ULxItemLogicBase
{
	GENERATED_BODY()


public:
	/**
 	 * @brief 初始化技能逻辑
 	 *
 	 * 该方法用于根据给定的物品定义信息初始化技能逻辑。如果提供的物品信息是有效的，并且其类型为技能，
 	 * 则会使用这些信息来设置内部的技能数据结构，并返回成功状态。
 	 *
 	 * @param pItemInfo 指向物品定义基础类型的常量指针，包含了需要初始化的技能的所有相关信息。
 	 *
 	 * @return 如果初始化成功则返回true；否则返回false，表示初始化失败（例如，提供的物品信息为空或不是技能类型）。
 	 */
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	/**
	 * @brief 获取物品数据基类
	 *
	 * 该方法用于获取当前技能逻辑实例的内部物品数据基类。返回的数据包含了物品的基础信息、堆叠信息、数量、可视化信息以及稀有度信息。
	 *
	 * @return 返回指向内部物品数据基类的指针，该数据结构包含了物品的所有相关信息。
	 */
	virtual FLxItemDateBase* GetItemDataBase() override;
	/**
	 * @brief 使用技能
	 *
	 * 该方法用于执行与当前技能逻辑实例相关的使用操作。具体实现由子类提供，以支持特定技能的使用逻辑。
	 * 默认实现返回false，表示未实现具体的技能使用逻辑或技能无法被使用。
	 *
	 * @return 如果技能成功被使用则返回true；否则返回false，表示技能使用失败或未实现。
	 */
	virtual bool UseItem() override;

private:
	FLxSkilllData m_SkillData;
};

