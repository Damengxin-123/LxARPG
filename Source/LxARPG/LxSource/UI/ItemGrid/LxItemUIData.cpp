#include "LxItemUIData.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

void ULxItemUIData::InitializeGridData(int32 InIndex, ULxItemSlotData* InSlotData, EItemSlotWidgetType InSlotType, int32 InItemSubType)
{
	m_nIndex = InIndex;
	m_pSlotData = InSlotData;
	m_pItemData = InSlotData ? InSlotData->ItemDataPtr : nullptr;
	m_nItemType = InSlotType;
	m_nItemSubType = InItemSubType;
}
