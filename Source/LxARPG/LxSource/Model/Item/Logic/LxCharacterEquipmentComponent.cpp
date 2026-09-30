#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "GameFramework/Actor.h"

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
		NewSlot->SetSlotIndex(Index);
		NewSlot->InitItemSlot(ELxItemSlotType::Equipment, EquipmentSlotsConfig[Index], nullptr);
		NewSlot->OnItemDataChanged.AddUObject(this, &ULxCharacterEquipmentModule::HandleEquipmentSlotChanged);
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

bool ULxCharacterEquipmentModule::RestoreEquipmentSaveData(const TArray<FLxItemSlotSaveRecord>& InSlots, bool bApply)
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		return false;
	}
	InitializeEquipmentSlots();
	TArray<TObjectPtr<ULxItemSlotData>> RestoredSlots;
	for (int32 Index = 0; Index < EquipmentSlotsConfig.Num(); ++Index)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(this);
		Slot->SetSlotIndex(Index);
		Slot->InitItemSlot(ELxItemSlotType::Equipment, EquipmentSlotsConfig[Index], nullptr);
		RestoredSlots.Add(Slot);
	}
	if (!LxItemSaveData::RestoreSlots(this, InSlots, RestoredSlots))
	{
		return false;
	}
	if (!bApply)
	{
		return true;
	}
	for (ULxItemSlotData* OldSlot : m_vEquipmentSlots)
	{
		if (OldSlot)
		{
			OldSlot->OnItemDataChanged.RemoveAll(this);
		}
	}
	m_vEquipmentSlots = MoveTemp(RestoredSlots);
	for (ULxItemSlotData* Slot : m_vEquipmentSlots)
	{
		Slot->OnItemDataChanged.AddUObject(this, &ULxCharacterEquipmentModule::HandleEquipmentSlotChanged);
	}
	OnDataChange.Broadcast();
	return true;
}
