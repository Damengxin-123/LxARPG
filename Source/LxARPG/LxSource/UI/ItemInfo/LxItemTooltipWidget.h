#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemEnmuType.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"
#include "LxItemTooltipWidget.generated.h"

class ULxItemLogicBase;
class ULxUITextData;

/**
 * @brief 物品悬浮信息控件。
 *
 * C++层仅负责缓存当前物品逻辑、整理供蓝图读取的数据，
 * 具体界面布局与显示效果由继承它的蓝图类型实现。
 */
UCLASS(BlueprintType, Blueprintable, DisplayName="物品悬浮信息控件")
class LXARPG_API ULxItemTooltipWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/**
	 * @brief 设置当前需要显示的物品信息。
	 * @param InItemLogic 待显示的物品逻辑对象。
	 * @return 当物品对象有效时返回true，否则返回false。
	 */
	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置显示物品信息")
	bool SetDisplayItemLogic(ULxItemLogicBase* InItemLogic);

	/** 获取当前显示物品的类型。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取当前物品类型")
	ELxItemType GetCurrentItemType() const;

	/** 获取当前物品的词条显示数据列表。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取物品词条显示数据")
	TArray<ULxUITextData*> GetItemEntryUITextDataList();

	/** 获取当前物品基础信息。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取物品基础信息")
	const FLxItemDateBase& GetItemBaseInfo() const;

	/** 获取当前装备物品信息。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取装备信息")
	const FLxEquipmentData& GetEquipmentInfo() const;

	/** 获取当前消耗品物品信息。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取消耗品信息")
	const FLxConsumableData& GetConsumableInfo() const;

	/** 获取当前材料物品信息。 */
	UFUNCTION(BlueprintPure, Category="ItemTooltip", DisplayName="获取材料信息")
	const FLxMaterialData& GetMaterialInfo() const;

	/** 设置悬浮信息控件在视口中的屏幕位置。 */
	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置悬浮窗屏幕位置")
	void SetTooltipScreenPosition(FVector2D InScreenPosition);

public:
	/** 当前物品信息更新事件，供蓝图绑定刷新。 */
	UPROPERTY(BlueprintAssignable, Category="ItemTooltip", DisplayName="物品信息更新事件")
	FOnUiBaseUpdateEvent OnItemTooltipUpdate;

protected:
	/** 当前缓存的物品逻辑对象。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="ItemTooltip", DisplayName="当前物品逻辑", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxItemLogicBase> m_pCurrentItemLogic = nullptr;
};
