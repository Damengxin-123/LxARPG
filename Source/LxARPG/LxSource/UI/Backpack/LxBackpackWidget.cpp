#include "LxBackpackWidget.h"

#include "LxARPG/LxSource/Model/Item/DataType/ItemData/LxItemData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentEnum.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxBackpackData.h"

void ULxBackpackWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
}

void ULxBackpackWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);
	RebindCharacterComponents(PlayerCharacter);
	HandleBackpackDataUpdate();
}

void ULxBackpackWidget::HandleBackpackDataUpdate()
{
	ShowItemList();
	ShowEquipmentList();
}

void ULxBackpackWidget::SortingOfItems()
{
	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->SortingOfItems();
	}
}

void ULxBackpackWidget::SwitchItemType(ELxItemType NewType)
{
	m_CurrentItemType = NewType;
	ShowItemList();
}

void ULxBackpackWidget::ShowItemList()
{
	TArray<ULxBackpackData*> ItemList;

	if (!m_pCharacterBackpackComponent)
	{
		OnInventoryItemListChanged.Broadcast(ItemList);
		return;
	}

	const int32 SlotCount = m_pCharacterBackpackComponent->GetBackpackSlotCount();
	ItemList.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		ULxItemData* ItemData = m_pCharacterBackpackComponent->GetItemAt(SlotIndex);
		if (m_CurrentItemType != ELxItemType::None)
		{
			if (!ItemData || ItemData->GetItemType() != m_CurrentItemType)
			{
				continue;
			}
		}

		ULxBackpackData* BackpackData = NewObject<ULxBackpackData>(this);
		BackpackData->InitializeGridData(SlotIndex, ItemData, EItemSlotWidgetType::Inventory, INDEX_NONE, m_pCharacterBackpackComponent);
		ItemList.Add(BackpackData);
	}

	OnInventoryItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::ShowEquipmentList()
{
	TArray<ULxBackpackData*> ItemList;

	if (!m_pCharacterEquipmentComponent)
	{
		OnEquipmentItemListChanged.Broadcast(ItemList);
		return;
	}

	const int32 SlotCount = m_pCharacterEquipmentComponent->GetEquipmentSlotCount();
	ItemList.Reserve(SlotCount);

	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const ELxEquipmentType EquipmentType = static_cast<ELxEquipmentType>(SlotIndex);
		ULxItemData* ItemData = m_pCharacterEquipmentComponent->GetEquipmentAt(EquipmentType);

		ULxBackpackData* BackpackData = NewObject<ULxBackpackData>(this);
		BackpackData->InitializeGridData(SlotIndex, ItemData, EItemSlotWidgetType::Equipment, SlotIndex, m_pCharacterBackpackComponent);
		ItemList.Add(BackpackData);
	}

	OnEquipmentItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::RebindCharacterComponents(ALxBaseCharacter* PlayerCharacter)
{
	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->OnBackpackChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	m_pCharacterBackpackComponent = PlayerCharacter ? PlayerCharacter->GetCharacterBackpackComponent() : nullptr;
	m_pCharacterEquipmentComponent = PlayerCharacter ? PlayerCharacter->GetCharacterEquipmentComponent() : nullptr;

	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->OnBackpackChanged.AddDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->OnEquipmentChanged.AddDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}
}
