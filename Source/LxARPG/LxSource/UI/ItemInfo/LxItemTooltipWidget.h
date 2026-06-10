#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxItemTooltipWidget.generated.h"

class ULxItemBase;
class ULxUITextData;

/** 按词条逻辑类型拆分后的词条显示数据。 */
struct FLxItemEntryDisplayDataByLogicType
{
	TArray<ULxUITextData*> NormalEntryDataList;
	TArray<ULxUITextData*> BaseEntryDataList;
	TArray<ULxUITextData*> LockedEntryDataList;
	TArray<ULxUITextData*> SpecialEntryDataList;
};

UCLASS(BlueprintType, Blueprintable, DisplayName="物品悬浮信息控件")
class LXARPG_API ULxItemTooltipWidget : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置显示物品信息")
	bool SetDisplayItemLogic(ULxItemBase* InItem);

	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置显示物品信息和价值")
	bool SetDisplayItemLogicWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue);

	UFUNCTION(BlueprintCallable, Category="ItemTooltip", DisplayName="设置悬浮窗口屏幕位置")
	void SetTooltipScreenPosition(FVector2D InScreenPosition);

	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示词条信息")
	void OnItemEntryDisplayUpdated(bool bHasEntry, const TArray<ULxUITextData*>& ItemEntryDataList);

	/** 按普通、基础、锁定、特殊四种词条逻辑类型显示词条信息。 */
	UFUNCTION(BlueprintImplementableEvent, Category="物品悬浮信息", DisplayName="按词条逻辑类型显示词条信息")
	void OnItemEntryDisplayUpdatedByLogicType(
		bool bHasEntry,
		const TArray<ULxUITextData*>& NormalEntryDataList,
		const TArray<ULxUITextData*>& BaseEntryDataList,
		const TArray<ULxUITextData*>& LockedEntryDataList,
		const TArray<ULxUITextData*>& SpecialEntryDataList);

	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示物品基础信息")
	void OnItemBaseInformationUpdated(const FLxItemInformationBase& ItemInformation);

	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="更新物品价值")
	void OnItemValueUpdated(int32 ItemValue, bool bShowItemValue);

	UFUNCTION(BlueprintImplementableEvent, Category="ItemTooltip", DisplayName="显示装备信息")
	void OnEquipmentInformationUpdated(const FLxEquipmentInformation& EquipmentInformation);

protected:
	UPROPERTY(Transient, BlueprintReadOnly, Category="ItemTooltip", DisplayName="当前物品", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxItemBase> m_pCurrentItem = nullptr;

private:
	/** 构建兼容旧显示事件的完整词条文本列表。 */
	TArray<ULxUITextData*> BuildItemEntryUITextDataList();

	/** 按词条逻辑类型构建词条文本列表。 */
	FLxItemEntryDisplayDataByLogicType BuildItemEntryUITextDataByLogicType();
};
