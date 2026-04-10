#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxCharacterEquipmentComponent.generated.h"

class ALxBaseCharacter;
class ULxEquipmentSlotData;
class ULxEquipmentLogic;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class LXARPG_API ULxCharacterEquipmentComponent : public ULxComponentBase
{
	GENERATED_BODY()

	/**
	 * @brief 构造函数，用于创建角色装备组件实例。
	 *
	 * 初始化角色装备组件的基本设置。此构造函数主要用于设置默认属性和初始化必要的变量。
	 *
	 * @return 无返回值
	 */
public:
	ULxCharacterEquipmentComponent();

	/**
	 * @brief 初始化角色装备组件。
	 *
	 * 负责缓存所属角色并初始化装备槽数据。
	 */
	virtual void BaseComponentInitialize() override;

	TArray<TObjectPtr<ULxEquipmentSlotData>>& GetEquipmentSlots();

protected:
	UPROPERTY(Blueprintable, BlueprintReadWrite, DisplayName="装备槽位配置")
	TArray<ELxEquipmentType> EquipmentSlotsConfig;

private:
	/**
	 * @brief 初始化装备槽位。
	 *
	 * 该方法用于初始化角色的装备槽位。如果当前没有定义任何装备槽位配置，则会调用 `SetDefauitEquipmentSlotsConfig` 方法来设置默认的装备槽位配置，确保每个角色都有一个基本的装备槽位布局。
	 *
	 * @note 此方法通常在组件初始化过程中被自动调用，以保证所有角色至少拥有基础的装备槽位配置。
	 */
	inline void InitializeEquipmentSlots();

	/**
	 * @brief 设置默认的装备槽位配置。
	 *
	 * 该方法用于为角色装备组件设置一套默认的装备槽位配置。如果当前没有定义装备槽位配置，则调用此方法来填充默认值，确保每个角色都有一个基本的装备槽位布局。
	 *
	 * @note 此方法通常在初始化过程中自动调用，以保证所有角色至少拥有基础的装备槽位配置。
	 */
	inline void SetDefauitEquipmentSlotsConfig();
	
	/** 广播装备数据发生变化。 */
	void BroadcastEquipmentChanged();


	
	/** 当前拥有此装备组件的角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter;

	// 背包槽位数组
	UPROPERTY()
	TArray<TObjectPtr<ULxEquipmentSlotData>> m_vEquipmentSlots;
	
	/** 当前角色各个装备位上的装备数据。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxEquipmentLogic>> m_vEquipmentList;
};
