#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"

void ULxCharacterEquipmentModule::OnModuleInitialize()
{
	InitializeEquipmentSlots();
	OnDataChange.Broadcast();
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
}

void ULxCharacterEquipmentModule::HandleEquipmentSlotChanged(ULxItemBase*)
{
	OnDataChange.Broadcast();
}
