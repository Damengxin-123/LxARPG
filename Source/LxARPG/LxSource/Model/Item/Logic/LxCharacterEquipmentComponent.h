#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentModuleBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxCharacterEquipmentComponent.generated.h"

class ALxBaseCharacter;
class ULxEquipment;
class ULxItemSlotData;

/**
 * 角色装备组件。
 *
 * 该组件只关心新的装备物品对象 ULxEquipment：装备槽位负责接收和保存物品，
 * 组件负责根据槽位内容维护当前已装备列表，并通过 OnDataChange 通知数据中转组件。
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced, DisplayName="角色装备模块")
class LXARPG_API ULxCharacterEquipmentModule : public ULxCharacterContentModuleBase
{
	GENERATED_BODY()

public:
	/** 创建角色装备组件。 */
	ULxCharacterEquipmentModule();

	/** 获取全部装备槽位。 */
	TArray<TObjectPtr<ULxItemSlotData>>& GetEquipmentSlots();

	/** 装备槽位配置，每个元素表示一个装备槽可接受的装备部位类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName="装备槽位配置", meta=(Categories="物品.装备"))
	TArray<FGameplayTag> EquipmentSlotsConfig;

private:
	/** 初始化装备槽位并同步当前已装备缓存。 */
	virtual void OnModuleInitialize() override;

	/** 初始化装备槽位。 */
	void InitializeEquipmentSlots();

	/** 在没有手动配置时填充默认装备槽位。 */
	void SetDefauitEquipmentSlotsConfig();

	/** 广播装备数据变化。 */
	void BroadcastEquipmentChanged();

	/** 响应任意装备槽位内容变化。 */
	UFUNCTION()
	void HandleEquipmentSlotChanged(ULxItemBase* InItemData);

	/** 从装备槽位刷新当前已装备物品缓存。 */
	void RefreshEquipmentList();

	/** 当前拥有此装备组件的角色。 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pOwnerCharacter = nullptr;

	/** 装备槽位数组。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxItemSlotData>> m_vEquipmentSlots;

	/** 当前角色各个装备位上的装备对象。 */
	UPROPERTY()
	TArray<TObjectPtr<ULxEquipment>> m_vEquipmentList;
};
