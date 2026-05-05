#include "LxItemTooltipWidget.h"

#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

bool ULxItemTooltipWidget::SetDisplayItemLogic(ULxItemBase* InItem)
{
	if (!InItem || !InItem->ItemIsValid())
	{
		return false;
	}

	m_pCurrentItem = InItem;

	OnItemBaseInformationUpdated(m_pCurrentItem->ItemInformation());

	// 当前物品为装备时，额外把装备专属结构体交给蓝图显示。
	if (ULxEquipment* Equipment = Cast<ULxEquipment>(m_pCurrentItem))
	{
		OnEquipmentInformationUpdated(Equipment->EquipmentInformation());
	}

	TArray<ULxUITextData*> EntryUIDataList = BuildItemEntryUITextDataList();
	OnItemEntryDisplayUpdated(EntryUIDataList.Num() > 0, EntryUIDataList);

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

		// C++ 只整理词条文本数据，具体布局和样式交给蓝图实现。
		TextData->DisplayText = EntryObject->GetDisplayName();
		TextData->IsDarkColor = bIsDarkColor;
		Result.Add(TextData);
		bIsDarkColor = !bIsDarkColor;
	}

	return Result;
}
