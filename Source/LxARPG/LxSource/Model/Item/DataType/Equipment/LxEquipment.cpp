#include "LxEquipment.h"

ULxEquipment::ULxEquipment()
{
	m_fEquipmentInformation = FLxEquipmentInformation();
}

ULxEquipment::~ULxEquipment()
{
}

ELxItemUseState ULxEquipment::ItemUse()
{
	return ELxItemUseState::InstallEquipment;
}

FLxString ULxEquipment::ItemCountText()
{
	return m_fEquipmentInformation.ItemCount > 1 ? FLxString(m_fEquipmentInformation.ItemCount) : FLxString();
}

ELxEquipmentType ULxEquipment::EquipmentType() const
{
	return m_fEquipmentInformation.EquipmentType;
}

FLxEquipmentInformation ULxEquipment::EquipmentInformation() const
{
	return m_fEquipmentInformation;
}

void ULxEquipment::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (InItemData)
	{
		if (InItemData->ItemType == ELxItemType::Equipment)
		{
			m_fEquipmentInformation = *static_cast<const FLxEquipmentInformation*>(InItemData);
			m_fEquipmentInformation.ItemCount = InItemCount;
		}
	}
}

FLxItemInformationBase* ULxEquipment::ItemBase()
{
	return &m_fEquipmentInformation;
}
