#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEquipmentComponent::ULxCharacterEquipmentComponent()
{
}

void ULxCharacterEquipmentComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	InitializeEquipmentSlots();
}

TArray<TObjectPtr<ULxEquipmentSlotData>>& ULxCharacterEquipmentComponent::GetEquipmentSlots()
{
	return m_vEquipmentSlots;
}

inline void ULxCharacterEquipmentComponent::InitializeEquipmentSlots()
{
	if (EquipmentSlotsConfig.IsEmpty())
	{
		SetDefauitEquipmentSlotsConfig();
	}
	for (int i = 0; i < EquipmentSlotsConfig.Num(); ++i)
	{
		TObjectPtr<ULxEquipmentSlotData> NewSlot = NewObject<ULxEquipmentSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Equipment;
		NewSlot->EquipmentType = EquipmentSlotsConfig[i];
		NewSlot->ID = i;
		m_vEquipmentSlots.Add(NewSlot);
	}
}

void ULxCharacterEquipmentComponent::SetDefauitEquipmentSlotsConfig()
{
	for (int i = 0; i < static_cast<int32>(ELxEquipmentType::EquipmentSizeMax); ++i)
	{
		EquipmentSlotsConfig.Add(static_cast<ELxEquipmentType>(i));
	}
}

void ULxCharacterEquipmentComponent::BroadcastEquipmentChanged()
{
	OnDataChange.Broadcast();
}
