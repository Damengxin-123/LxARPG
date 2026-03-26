#include "LxBackpackData.h"

void ULxBackpackData::InitializeGridData(int32 InIndex, ULxItemData* InItemData, EItemSlotWidgetType InSlotType, int32 InItemSubType, ULxCharacterBackpackComponent* InBackpackComponent)
{
	m_nIndex = InIndex;
	m_pItemData = InItemData;
	m_nItemType = InSlotType;
	m_nItemSubType = InItemSubType;
	m_pBackpackComponent = InBackpackComponent;
}
