#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxItemSaveData.h"
#include "LxCharacterEquipmentComponent.generated.h"

class ULxItemBase;
class ULxItemSlotData;

/**
 * 角色装备组件。
 *
 * 该组件只关心新的装备物品对象 ULxEquipment：装备槽位负责接收和保存物品，
 * 组件通过 OnDataChange 通知数据中转组件，已装备物品统一从槽位读取。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色装备模块")
class LXARPG_API ULxCharacterEquipmentModule : public ULxCharacterContentModuleBase
{
	GENERATED_BODY()

public:
	/** 获取全部装备槽位。 */
	TArray<TObjectPtr<ULxItemSlotData>>& GetEquipmentSlots();

	/** 整体恢复装备槽位后只广播一次变化；关闭应用时只验证，由中转组件替换现有效果。 */
	bool RestoreEquipmentSaveData(const TArray<FLxItemSlotSaveRecord>& InSlots, bool bApply = true);

	/** 装备槽位配置，每个元素表示一个装备槽可接受的装备部位类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备槽位配置", meta=(Categories="物品.装备"))
	TArray<FGameplayTag> EquipmentSlotsConfig;

private:
	/** 初始化装备槽位并通知数据中转组件。 */
	virtual void OnModuleInitialize() override;

	/** 初始化装备槽位。 */
	void InitializeEquipmentSlots();

	/** 在没有手动配置时填充默认装备槽位。 */
	void SetDefauitEquipmentSlotsConfig();

	/** 响应任意装备槽位内容变化。 */
	UFUNCTION()
	void HandleEquipmentSlotChanged(ULxItemBase* InItemData);

	/** 装备槽位数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vEquipmentSlots;
};
