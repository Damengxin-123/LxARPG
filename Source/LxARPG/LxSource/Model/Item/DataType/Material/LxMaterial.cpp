#include "LxMaterial.h"

ULxMaterial::ULxMaterial()
{
	m_fMaterialInformation = FLxMaterialInformation();
}

ULxMaterial::~ULxMaterial()
{
}

ELxItemUseState ULxMaterial::ItemUse()
{
	return ELxItemUseState::Failed;
}


FLxString ULxMaterial::ItemCountText()
{
	return m_fMaterialInformation.ItemCount > 1 ? FLxString(m_fMaterialInformation.ItemCount) : FLxString();
}

void ULxMaterial::SetItemData(const FLxItemInformationBase* InItemData, FLxItemCount InItemCount)
{
	if (InItemData)
	{
		if (InItemData->ItemType == ELxItemType::Material)
		{
			m_fMaterialInformation = *static_cast<const FLxMaterialInformation*>(InItemData);
			m_fMaterialInformation.ItemCount = InItemCount;
		}
	}
}

FLxItemInformationBase* ULxMaterial::ItemBase()
{
	return &m_fMaterialInformation;
}
