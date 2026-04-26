#include "LxItemTooltipWidget.h"

#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxItemEntryLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableLogic.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxItemEntryData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialLogic.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

namespace
{
	void AppendEntryUITextData(TArray<ULxUITextData*>& OutUIDataList, UObject* InOuter,
		ULxItemEntryLogic* InEntryLogic, bool& bInOutIsDarkColor)
	{
		if (!IsValid(InEntryLogic) || !InEntryLogic->IsEntryValid())
		{
			return;
		}

		ULxUITextData* NewTextData = NewObject<ULxUITextData>(InOuter);
		if (!NewTextData)
		{
			return;
		}

		NewTextData->DisplayText = InEntryLogic->BuildEntryDisplayText();
		NewTextData->IsDarkColor = bInOutIsDarkColor;
		OutUIDataList.Add(NewTextData);
		bInOutIsDarkColor = !bInOutIsDarkColor;
	}
}

bool ULxItemTooltipWidget::SetDisplayItemLogic(ULxItemLogicBase* InItemLogic)
{
	if (!InItemLogic || !InItemLogic->ItemIsValid())
	{
		return false;
	}

	m_pCurrentItemLogic = InItemLogic;
	OnItemTooltipUpdate.Broadcast();
	return true;
}

ELxItemType ULxItemTooltipWidget::GetCurrentItemType() const
{
	if (!m_pCurrentItemLogic || !m_pCurrentItemLogic->ItemIsValid() || !m_pCurrentItemLogic->GetItemDataBase())
	{
		return ELxItemType::None;
	}

	return m_pCurrentItemLogic->GetItemDataBase()->ItemInfo.ItemType;
}

TArray<ULxUITextData*> ULxItemTooltipWidget::GetItemEntryUITextDataList()
{
	TArray<ULxUITextData*> Result;
	bool bIsDarkColor = true;

	if (const ULxEquipmentLogic* EquipmentLogic = Cast<ULxEquipmentLogic>(m_pCurrentItemLogic))
	{
		if (const FLxEquipmentData* EquipmentData = EquipmentLogic->GetEquipmentData())
		{
			AppendEntryUITextData(Result, this, EquipmentData->EquipmentEntyInfo.EquipmentBasicEntry, bIsDarkColor);
			for (ULxItemEntryLogic* EntryLogic : EquipmentData->EquipmentEntyInfo.EquipmentExtendEntryList)
			{
				AppendEntryUITextData(Result, this, EntryLogic, bIsDarkColor);
			}
		}
		return Result;
	}

	if (const ULxConsumableLogic* ConsumableLogic = Cast<ULxConsumableLogic>(m_pCurrentItemLogic))
	{
		if (const FLxConsumableData* ConsumableData = ConsumableLogic->GetConsumableData())
		{
			for (ULxItemEntryLogic* EntryLogic : ConsumableData->ConsumableEntryInfo.ConsumableEntryList)
			{
				AppendEntryUITextData(Result, this, EntryLogic, bIsDarkColor);
			}
		}
		return Result;
	}

	if (const ULxBuffLogic* BuffLogic = Cast<ULxBuffLogic>(m_pCurrentItemLogic))
	{
		const auto& BuffData = BuffLogic->GetBuffData();
		for (ULxItemEntryLogic* EntryLogic : BuffData.BuffEntryList)
		{
			AppendEntryUITextData(Result, this, EntryLogic, bIsDarkColor);
		}
	}

	return Result;
}

const FLxItemDateBase& ULxItemTooltipWidget::GetItemBaseInfo() const
{
	static FLxItemDateBase EmptyItemData;

	if (!m_pCurrentItemLogic || !m_pCurrentItemLogic->ItemIsValid() || !m_pCurrentItemLogic->GetItemDataBase())
	{
		return EmptyItemData;
	}

	return *m_pCurrentItemLogic->GetItemDataBase();
}

const FLxEquipmentData& ULxItemTooltipWidget::GetEquipmentInfo() const
{
	static FLxEquipmentData EmptyEquipmentData;

	if (const ULxEquipmentLogic* EquipmentLogic = Cast<ULxEquipmentLogic>(m_pCurrentItemLogic))
	{
		if (const FLxEquipmentData* EquipmentData = EquipmentLogic->GetEquipmentData())
		{
			return *EquipmentData;
		}
	}

	return EmptyEquipmentData;
}

const FLxConsumableData& ULxItemTooltipWidget::GetConsumableInfo() const
{
	static FLxConsumableData EmptyConsumableData;

	if (const ULxConsumableLogic* ConsumableLogic = Cast<ULxConsumableLogic>(m_pCurrentItemLogic))
	{
		if (const FLxConsumableData* ConsumableData = ConsumableLogic->GetConsumableData())
		{
			return *ConsumableData;
		}
	}

	return EmptyConsumableData;
}

const FLxMaterialData& ULxItemTooltipWidget::GetMaterialInfo() const
{
	static FLxMaterialData EmptyMaterialData;

	if (const ULxMaterialLogic* MaterialLogic = Cast<ULxMaterialLogic>(m_pCurrentItemLogic))
	{
		if (const FLxMaterialData* MaterialData = MaterialLogic->GetMaterialData())
		{
			return *MaterialData;
		}
	}

	return EmptyMaterialData;
}

void ULxItemTooltipWidget::SetTooltipScreenPosition(FVector2D InScreenPosition)
{
	SetPositionInViewport(InScreenPosition, false);
}
