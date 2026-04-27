// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxBuff.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxBuffLogic.generated.h"

UCLASS()
class LXARPG_API ULxBuffLogic : public ULxItemLogicBase
{
	GENERATED_BODY()

public:
	static ULxBuffLogic* CreateBuffLogicObject(const FLxBuffDefine& InBuffDefine, UObject* InOuter);

	/**
	 * @brief 初始化物品逻辑
	 *
	 * 该方法用于初始化一个物品的逻辑，特别是当物品类型为Buff时。它会根据提供的物品信息来设置内部数据结构。
	 *
	 * @param pItemInfo 指向包含物品定义基础信息的指针。这些信息包括但不限于物品ID、类型、堆叠信息等。
	 * @return 如果成功初始化了物品逻辑，则返回true；否则返回false。失败的情况包括传入的pItemInfo为空或其描述的物品不是Buff类型。
	 */
	virtual bool InitItemLogic(const FLxItemDefineBase* pItemInfo) override;
	/**
	 * @brief 获取物品数据基础
	 *
	 * 该方法用于获取当前Buff逻辑实例的物品数据基础。这些数据包括了物品的基础信息、堆叠信息、数量、可视化信息和稀有度信息。
	 *
	 * @return 返回一个指向FLxItemDateBase结构体的指针，包含了物品的各种属性信息。如果未初始化或内部数据为空，则返回的指针可能为nullptr。
	 */
	virtual FLxItemDateBase* GetItemDataBase() override;
	/**
	 * @brief 使用物品
	 *
	 * 该方法用于实现Buff类型物品的使用逻辑。默认实现返回false，表示物品未被使用。
	 * 具体的使用逻辑需要在子类中重写此方法来提供。
	 *
	 * @return 如果物品成功被使用则返回true，否则返回false。
	 */
	virtual bool UseItem() override;

	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="Buff是否有效")
	bool IsBuffValid() const;

	virtual FLxString GetItemCount();

	UFUNCTION(BlueprintPure, Category="Buff", DisplayName="获取Buff数据")
	const FLxBuffData& GetBuffData() const { return m_BuffData; }

	FLxBuffData& GetMutableBuffData() { return m_BuffData; }

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="设置Buff生效比例")
	void SetEffectProportion(float InEffectProportion);

	UFUNCTION(BlueprintCallable, Category="Buff", DisplayName="设置Buff持续时间")
	void SetDuration(float InDuration);

private:
	UPROPERTY()
	FLxBuffData m_BuffData;
};
