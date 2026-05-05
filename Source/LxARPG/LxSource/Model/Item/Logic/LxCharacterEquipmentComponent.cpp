#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEquipmentComponent::ULxCharacterEquipmentComponent()
{
}

void ULxCharacterEquipmentComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();

	if (m_pOwnerCharacter == nullptr)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	InitializeEquipmentSlots();
	RefreshEquipmentList();
}

TArray<TObjectPtr<ULxEquipmentSlotData>>& ULxCharacterEquipmentComponent::GetEquipmentSlots()
{
	return m_vEquipmentSlots;
}

const TArray<TObjectPtr<ULxEquipmentSlotData>>& ULxCharacterEquipmentComponent::GetEquipmentSlots() const
{
	return m_vEquipmentSlots;
}

void ULxCharacterEquipmentComponent::InitializeEquipmentSlots()
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
		ULxEquipmentSlotData* NewSlot = NewObject<ULxEquipmentSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Equipment;
		NewSlot->EquipmentType = EquipmentSlotsConfig[Index];
		NewSlot->ID = Index;
		NewSlot->OnSlotChanged.AddDynamic(this, &ULxCharacterEquipmentComponent::HandleEquipmentSlotChanged);
		m_vEquipmentSlots.Add(NewSlot);
	}
}

void ULxCharacterEquipmentComponent::SetDefauitEquipmentSlotsConfig()
{
	EquipmentSlotsConfig.Empty();
	for (int32 Index = 0; Index < static_cast<int32>(ELxEquipmentType::EquipmentSizeMax); ++Index)
	{
		EquipmentSlotsConfig.Add(static_cast<ELxEquipmentType>(Index));
	}
}

void ULxCharacterEquipmentComponent::BroadcastEquipmentChanged()
{
	OnDataChange.Broadcast();
}

void ULxCharacterEquipmentComponent::HandleEquipmentSlotChanged()
{
	RefreshEquipmentList();
}

void ULxCharacterEquipmentComponent::RefreshEquipmentList()
{
	m_vEquipmentList.Empty();

	for (ULxEquipmentSlotData* EquipmentSlot : m_vEquipmentSlots)
	{
		if (EquipmentSlot == nullptr || !EquipmentSlot->IsValid())
		{
			continue;
		}

		ULxEquipment* Equipment = Cast<ULxEquipment>(EquipmentSlot->ItemDataPtr);
		if (Equipment == nullptr || !Equipment->ItemIsValid())
		{
			continue;
		}

		m_vEquipmentList.Add(Equipment);
	}

	BroadcastEquipmentChanged();
}
