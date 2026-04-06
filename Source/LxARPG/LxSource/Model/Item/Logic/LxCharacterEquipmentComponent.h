#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxCharacterEquipmentComponent.generated.h"

class ALxBaseCharacter;
class ULxItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCharacterEquipmentChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LXARPG_API ULxCharacterEquipmentComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxCharacterEquipmentComponent();

	/**
	 * @brief 初始化角色装备组件。
	 *
	 * 负责缓存所属角色并初始化装备槽数据。
	 */
	virtual void BaseComponentInitialize() override;

	/** 将背包中的装备放入其对应的装备位。 */
	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	bool EquipItemFromBackpack(int32 InBackpackIndex);

	/** 将背包中的装备放入指定装备位。 */
	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	bool EquipItemFromBackpackToSlot(int32 InBackpackIndex, ELxEquipmentType InEquipmentType);

	/** 将来自非背包来源的装备复制后放入指定装备位。 */
	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	bool EquipItemFromExternal(ULxItemData* InItemData, ELxEquipmentType InEquipmentType);

	/** 将指定装备位上的装备卸下到背包。 */
	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	bool UnequipItemToBackpack(ELxEquipmentType InEquipmentType);

	/** 将指定装备位上的装备卸下到背包指定格。 */
	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	bool UnequipItemToBackpackAt(ELxEquipmentType InEquipmentType, int32 InBackpackIndex);

	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	/**
	 * @brief 获取指定装备位上的物品对象。
	 *
	 * @param InEquipmentType 要查询的装备位类型。
	 * @return 若装备位有效则返回其上的物品对象，否则返回 nullptr。
	 */
	ULxItemData* GetEquipmentAt(ELxEquipmentType InEquipmentType) const;
	
	/**
	 * @brief 获取全部装备数组的可写引用。
	 *
	 * @return 返回当前全部装备数组引用。
	 */
	TArray<TObjectPtr<ULxItemData>>& GetAllEquipment();

	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	/**
	 * @brief 汇总当前已装备物品提供的词条列表。
	 *
	 * @return 返回全部装备词条组成的数组副本。
	 */
	TArray<FLxItemEntry> GetTotalEquipmentEntryList() const;

	UFUNCTION(BlueprintCallable, Category="Character|Equipment")
	/**
	 * @brief 获取装备槽总数。
	 *
	 * @return 当前角色支持的装备槽数量。
	 */
	int32 GetEquipmentSlotCount() const;

	UPROPERTY(BlueprintAssignable, Category="Character|Equipment")
	FOnCharacterEquipmentChanged OnEquipmentChanged;

private:
	/** 初始化装备槽位数组。 */
	void InitializeEquipmentSlots();
	/** 判断装备位类型是否合法。 */
	bool IsValidEquipmentType(ELxEquipmentType InEquipmentType) const;
	/** 将装备位类型转换为数组索引。 */
	int32 GetEquipmentIndex(ELxEquipmentType InEquipmentType) const;
	/** 将已解析好的装备物品放入指定装备位。 */
	bool EquipResolvedItem(ULxItemData* InItemData, ELxEquipmentType InEquipmentType);
	/** 广播装备数据发生变化。 */
	void BroadcastEquipmentChanged();

private:
	/** 当前拥有此装备组件的角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	/** 当前角色各个装备位上的装备数据。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemData>> m_vEquipmentItems;

	/** 装备组件是否已经完成初始化。 */
	bool m_bEquipmentInitialized = false;
};
