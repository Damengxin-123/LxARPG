#include "LxItemTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"

#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

bool ULxItemTooltipWidget::SetDisplayItemLogic(ULxItemBase* InItem)
{
	return SetDisplayItemLogicWithValue(InItem, 0, false);
}

bool ULxItemTooltipWidget::SetDisplayItemLogicWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue)
{
	if (!InItem || !InItem->ItemIsValid())
	{
		return false;
	}

	m_pCurrentItem = InItem;

	OnItemBaseInformationUpdated(m_pCurrentItem->ItemInformation());
	const bool bShouldShowItemValue = bInShowItemValue
		&& m_pCurrentItem->ItemType() != ELxItemType::Buff
		&& m_pCurrentItem->ItemType() != ELxItemType::Skill;
	OnItemValueUpdated(InItemValue, bShouldShowItemValue);

	if (ULxEquipment* Equipment = Cast<ULxEquipment>(m_pCurrentItem))
	{
		OnEquipmentInformationUpdated(Equipment->EquipmentInformation());
        RefreshCraftingInformation();
	}

	TArray<ULxUITextData*> EntryUIDataList = BuildItemEntryUITextDataList();
	OnItemEntryDisplayUpdated(EntryUIDataList.Num() > 0, EntryUIDataList);

	FLxItemEntryDisplayDataByLogicType EntryUIDataByLogicType = BuildItemEntryUITextDataByLogicType();
	const bool bHasTypedEntry = EntryUIDataByLogicType.NormalEntryDataList.Num() > 0
		|| EntryUIDataByLogicType.BaseEntryDataList.Num() > 0
		|| EntryUIDataByLogicType.LockedEntryDataList.Num() > 0
		|| EntryUIDataByLogicType.SpecialEntryDataList.Num() > 0;
	OnItemEntryDisplayUpdatedByLogicType(
		bHasTypedEntry,
		EntryUIDataByLogicType.NormalEntryDataList,
		EntryUIDataByLogicType.BaseEntryDataList,
		EntryUIDataByLogicType.LockedEntryDataList,
		EntryUIDataByLogicType.SpecialEntryDataList);

	return true;
}

void ULxItemTooltipWidget::RefreshCraftingInformation()
{
    const auto* Equipment = Cast<ULxEquipment>(m_pCurrentItem);
    if (!Equipment) return;
    const FText Value = FText::FromString(FString::Printf(TEXT("强化度 %.0f%%"), Equipment->GetEnhancement() * 100.f));
    UWidget* Strength = GetWidgetFromName(TEXT("强化强度"));
    if (auto* Text = Cast<UTextBlock>(Strength)) Text->SetText(Value);
    else if (auto* RichText = Cast<URichTextBlock>(Strength)) RichText->SetText(Value);
    if (UWidget* Potential = GetWidgetFromName(TEXT("锻造潜能"))) Potential->SetVisibility(ESlateVisibility::Collapsed);
}

void ULxItemTooltipWidget::SetTooltipScreenPosition(FVector2D InScreenPosition)
{
	SetPositionInViewport(InScreenPosition, false);
}

TArray<ULxUITextData*> ULxItemTooltipWidget::BuildItemEntryUITextDataList()
{
	TArray<ULxUITextData*> Result;
	if (!m_pCurrentItem || !m_pCurrentItem->ItemIsValid())
	{
		return Result;
	}

	// 装备强化只参与本次文本计算；其他物品的倍率为1，不改变后端词条对象。
	const float DisplayScale = m_pCurrentItem->GetItemEntryEffectMultiplier();
	bool bIsDarkColor = true;
	for (ULxEntryObjectBase* EntryObject : m_pCurrentItem->GetItemEntryList())
	{
		if (!EntryObject)
		{
			continue;
		}

		ULxUITextData* TextData = NewObject<ULxUITextData>(this);
		if (!TextData)
		{
			continue;
		}

		TextData->DisplayText = EntryObject->GetDisplayNameWithScale(DisplayScale);
		TextData->IsDarkColor = bIsDarkColor;
		Result.Add(TextData);
		bIsDarkColor = !bIsDarkColor;
	}

	return Result;
}

FLxItemEntryDisplayDataByLogicType ULxItemTooltipWidget::BuildItemEntryUITextDataByLogicType()
{
	FLxItemEntryDisplayDataByLogicType Result;
	if (!m_pCurrentItem || !m_pCurrentItem->ItemIsValid())
	{
		return Result;
	}

	// 分类显示与旧版完整列表使用同一倍率，避免两种蓝图事件显示不同数值。
	const float DisplayScale = m_pCurrentItem->GetItemEntryEffectMultiplier();
	bool bIsDarkColor = true;
	for (const FLxItemEntryRuntimeInfo& EntryRuntimeInfo : m_pCurrentItem->GetItemEntryRuntimeInfoList())
	{
		if (!EntryRuntimeInfo.EntryObject)
		{
			continue;
		}

		ULxUITextData* TextData = NewObject<ULxUITextData>(this);
		if (!TextData)
		{
			continue;
		}

		TextData->DisplayText = EntryRuntimeInfo.EntryObject->GetDisplayNameWithScale(DisplayScale);
		TextData->IsDarkColor = bIsDarkColor;

		switch (EntryRuntimeInfo.EntryLogicType)
		{
		case ELxEntryLogicType::Base:
			Result.BaseEntryDataList.Add(TextData);
			break;
		case ELxEntryLogicType::Locked:
			Result.LockedEntryDataList.Add(TextData);
			break;
		case ELxEntryLogicType::Special:
			Result.SpecialEntryDataList.Add(TextData);
			break;
		case ELxEntryLogicType::Normal:
		default:
			Result.NormalEntryDataList.Add(TextData);
			break;
		}

		bIsDarkColor = !bIsDarkColor;
	}

	return Result;
}
