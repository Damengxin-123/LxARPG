#include "LxBackpackWidget.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxBackpackWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
	if (ShortcutSlotCount <= 0)
	{
		ShortcutSlotCount = QUICK_ACCESS_TOOLBAR_COUNT;
	}
	InitializeLocalSlots();
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
	ShowShortcutList();
	ShowWarehouseList();
	ShowSkillList();
}

void ULxBackpackWidget::HandleLocalSlotsDataUpdate()
{
	ShowShortcutList();
	ShowWarehouseList();
	ShowSkillList();
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
	TArray<ULxItemUIData*> ItemList;

	if (!m_pCharacterBackpackComponent)
	{
		OnInventoryItemListChanged.Broadcast(ItemList);
		return;
	}

	TArray<TObjectPtr<ULxItemSlotData>>& BackpackSlots = m_pCharacterBackpackComponent->GetAllItems();
	ItemList.Reserve(BackpackSlots.Num());

	for (int32 SlotIndex = 0; SlotIndex < BackpackSlots.Num(); ++SlotIndex)
	{
		ULxItemSlotData* SlotData = BackpackSlots[SlotIndex];
		ULxItemLogicBase* ItemData = SlotData ? SlotData->ItemDataPtr : nullptr;

		if (m_CurrentItemType != ELxItemType::None)
		{
			const FLxItemDateBase* ItemBaseData = ItemData ? ItemData->GetItemDataBase() : nullptr;
			if (!ItemBaseData || ItemBaseData->ItemInfo.ItemType != m_CurrentItemType)
			{
				continue;
			}
		}

		ULxItemUIData* BackpackData = NewObject<ULxItemUIData>(this);
		BackpackData->InitializeGridData(SlotIndex, SlotData, EItemSlotWidgetType::Inventory, INDEX_NONE);
		ItemList.Add(BackpackData);
	}

	OnInventoryItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::ShowEquipmentList()
{
	TArray<ULxItemUIData*> ItemList;

	if (!m_pCharacterEquipmentComponent)
	{
		OnEquipmentItemListChanged.Broadcast(ItemList);
		return;
	}

	TArray<TObjectPtr<ULxEquipmentSlotData>>& EquipmentSlots = m_pCharacterEquipmentComponent->GetEquipmentSlots();
	ItemList.Reserve(EquipmentSlots.Num());

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		ULxEquipmentSlotData* EquipmentSlotData = EquipmentSlots[SlotIndex];
		const int32 SlotSubType = EquipmentSlotData ? static_cast<int32>(EquipmentSlotData->EquipmentType) : INDEX_NONE;

		ULxItemUIData* BackpackData = NewObject<ULxItemUIData>(this);
		BackpackData->InitializeGridData(SlotIndex, EquipmentSlotData, EItemSlotWidgetType::Equipment, SlotSubType);
		ItemList.Add(BackpackData);
	}

	OnEquipmentItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::ShowShortcutList()
{
	TArray<ULxItemUIData*> ItemList;
	ItemList.Reserve(m_vShortcutSlots.Num());

	for (int32 SlotIndex = 0; SlotIndex < m_vShortcutSlots.Num(); ++SlotIndex)
	{
		ULxItemUIData* Data = NewObject<ULxItemUIData>(this);
		Data->InitializeGridData(SlotIndex, m_vShortcutSlots[SlotIndex], EItemSlotWidgetType::Shortcut, INDEX_NONE);
		ItemList.Add(Data);
	}

	OnShortcutItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::ShowWarehouseList()
{
	TArray<ULxItemUIData*> ItemList;
	ItemList.Reserve(m_vWarehouseSlots.Num());

	for (int32 SlotIndex = 0; SlotIndex < m_vWarehouseSlots.Num(); ++SlotIndex)
	{
		ULxItemUIData* Data = NewObject<ULxItemUIData>(this);
		Data->InitializeGridData(SlotIndex, m_vWarehouseSlots[SlotIndex], EItemSlotWidgetType::Warehouse, INDEX_NONE);
		ItemList.Add(Data);
	}

	OnWarehouseItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::ShowSkillList()
{
	TArray<ULxItemUIData*> ItemList;
	ItemList.Reserve(m_vSkillSlots.Num());

	for (int32 SlotIndex = 0; SlotIndex < m_vSkillSlots.Num(); ++SlotIndex)
	{
		ULxItemUIData* Data = NewObject<ULxItemUIData>(this);
		Data->InitializeGridData(SlotIndex, m_vSkillSlots[SlotIndex], EItemSlotWidgetType::Skill, INDEX_NONE);
		ItemList.Add(Data);
	}

	OnSkillItemListChanged.Broadcast(ItemList);
}

void ULxBackpackWidget::RebindCharacterComponents(ALxBaseCharacter* PlayerCharacter)
{
	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->OnDataChange.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->OnDataChange.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	m_pCharacterBackpackComponent = PlayerCharacter ? PlayerCharacter->GetCharacterBackpackComponent() : nullptr;
	m_pCharacterEquipmentComponent = PlayerCharacter ? PlayerCharacter->GetCharacterEquipmentComponent() : nullptr;

	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->OnDataChange.AddDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}

	if (m_pCharacterEquipmentComponent)
	{
		m_pCharacterEquipmentComponent->OnDataChange.AddDynamic(this, &ULxBackpackWidget::HandleBackpackDataUpdate);
	}
}

void ULxBackpackWidget::InitializeLocalSlots()
{
	UnbindLocalSlots();

	m_vShortcutSlots.Empty();
	m_vWarehouseSlots.Empty();
	m_vSkillSlots.Empty();

	for (int32 i = 0; i < ShortcutSlotCount; ++i)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(this);
		Slot->ItemSlotType = ELxItemSlotType::Shortcut;
		Slot->ID = i;
		m_vShortcutSlots.Add(Slot);
	}

	for (int32 i = 0; i < WarehouseSlotCount; ++i)
	{
		ULxItemSlotData* Slot = NewObject<ULxItemSlotData>(this);
		Slot->ItemSlotType = ELxItemSlotType::Warehouse;
		Slot->ID = i;
		m_vWarehouseSlots.Add(Slot);
	}

	for (int32 i = 0; i < SkillSlotCount; ++i)
	{
		ULxSkillSlotData* Slot = NewObject<ULxSkillSlotData>(this);
		Slot->ItemSlotType = ELxItemSlotType::Shortcut;
		Slot->ID = i;
		m_vSkillSlots.Add(Slot);
	}

	BindLocalSlots();
}

void ULxBackpackWidget::BindLocalSlots()
{
	for (ULxItemSlotData* Slot : m_vShortcutSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.AddDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}

	for (ULxItemSlotData* Slot : m_vWarehouseSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.AddDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}

	for (ULxSkillSlotData* Slot : m_vSkillSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.AddDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}
}

void ULxBackpackWidget::UnbindLocalSlots()
{
	for (ULxItemSlotData* Slot : m_vShortcutSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}

	for (ULxItemSlotData* Slot : m_vWarehouseSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}

	for (ULxSkillSlotData* Slot : m_vSkillSlots)
	{
		if (Slot)
		{
			Slot->OnSlotChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleLocalSlotsDataUpdate);
		}
	}
}
