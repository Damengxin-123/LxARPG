#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxItemTooltipWidget.generated.h"

class ULxItemBase;
class ULxUITextData;

UCLASS(BlueprintType, Blueprintable, DisplayName="物品悬浮信息控件")
class LXARPG_API ULxItemTooltipWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置显示物品信息")
	bool SetDisplayItemLogic(ULxItemBase* InItem);

	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置悬浮窗口屏幕位置")
	void SetTooltipScreenPosition(FVector2D InScreenPosition);

	/** 词条显示刷新。没有词条时 bHasEntry 为 false，列表为空。 */
	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示词条信息")
	void OnItemEntryDisplayUpdated(bool bHasEntry, const TArray<ULxUITextData*>& ItemEntryDataList);

	/** 基础物品信息显示刷新，名称、描述、图标等可视化信息都在该结构体内。 */
	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示物品基础信息")
	void OnItemBaseInformationUpdated(const FLxItemInformationBase& ItemInformation);

	/** 装备物品显示刷新。当前物品为装备时调用，蓝图中显示装备专属信息。 */
	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示装备信息")
	void OnEquipmentInformationUpdated(const FLxEquipmentInformation& EquipmentInformation);

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="ItemTooltip", DisplayName="当前物品", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxItemBase> m_pCurrentItem = nullptr;

private:
	TArray<ULxUITextData*> BuildItemEntryUITextDataList();
};
