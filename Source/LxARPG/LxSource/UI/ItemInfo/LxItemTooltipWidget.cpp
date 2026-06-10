#include "LxItemTooltipWidget.h"

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

		TextData->DisplayText = EntryObject->GetDisplayName();
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

		TextData->DisplayText = EntryRuntimeInfo.EntryObject->GetDisplayName();
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
