#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEquipmentModule::ULxCharacterEquipmentModule()
{
}

void ULxCharacterEquipmentModule::OnModuleInitialize()
{
	if (m_pOwnerCharacter == nullptr)
	{
		m_pOwnerCharacter = GetCharacterOwner();
	}

	InitializeEquipmentSlots();
	RefreshEquipmentList();
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterEquipmentModule::GetEquipmentSlots()
{
	return m_vEquipmentSlots;
}

void ULxCharacterEquipmentModule::InitializeEquipmentSlots()
{
	if (!m_vEquipmentSlots.IsEmpty())
	{
		return;
	}

	if (EquipmentSlotsConfig.IsEmpty())
	{
		SetDefauitEquipmentSlotsConfig();
	}

	for (int32 Index = 0; Index < EquipmentSlotsConfig.Num(); ++Index)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->InitItemSlot(ELxItemSlotType::Equipment, EquipmentSlotsConfig[Index], nullptr);
		NewSlot->OnItemDataChanged.AddDynamic(this, &ULxCharacterEquipmentModule::HandleEquipmentSlotChanged);
		m_vEquipmentSlots.Add(NewSlot);
	}
}

void ULxCharacterEquipmentModule::SetDefauitEquipmentSlotsConfig()
{
	EquipmentSlotsConfig.Empty();
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Weapon);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Deputy);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Helmet);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Armor);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Leggings);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Boots);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Glove);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Belt);
	EquipmentSlotsConfig.Add(LxTag_Item_Equipment_Jewelry);
	
	// for (int32 Index = 0; Index < static_cast<int32>(ELxEquipmentType::EquipmentSizeMax); ++Index)
	// {
	// 	EquipmentSlotsConfig.Add(static_cast<ELxEquipmentType>(Index));
	// }
}

void ULxCharacterEquipmentModule::BroadcastEquipmentChanged()
{
	OnDataChange.Broadcast();
}

void ULxCharacterEquipmentModule::HandleEquipmentSlotChanged(ULxItemBase*)
{
	RefreshEquipmentList();
}

void ULxCharacterEquipmentModule::RefreshEquipmentList()
{
	m_vEquipmentList.Empty();

	for (ULxItemSlotData* EquipmentSlot : m_vEquipmentSlots)
	{
		if (EquipmentSlot == nullptr || !EquipmentSlot->IsValid())
		{
			continue;
		}

		ULxEquipment* Equipment = Cast<ULxEquipment>(EquipmentSlot->GetItem());
		if (Equipment == nullptr || !Equipment->ItemIsValid())
		{
			continue;
		}

		m_vEquipmentList.Add(Equipment);
	}

	BroadcastEquipmentChanged();
}
