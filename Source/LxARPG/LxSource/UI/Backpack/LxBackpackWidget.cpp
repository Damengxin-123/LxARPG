#include "LxBackpackWidget.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxBackpackWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
}

void ULxBackpackWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);
	UpdatedBackpack();
}

void ULxBackpackWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxBackpackWidget::UpdatedBackpack()
{
	if (m_pPlayerCharacter)
	{
		BindDataTransferComponent(m_pPlayerCharacter->GetCharacterDataTransferComponent());
	}

	if (m_pCharacterDataTransferComponent == nullptr)
	{
		m_vItemSlotList.Reset();
		m_vEquipmentSlotList.Reset();
		OnItemListUpdated(GetItemUIDataList());
		OnEquipmentListUpdated(GetEquipmentUIDataList());
		return;
	}

	TArray<ULxItemSlotData*> BackpackItems;
	m_pCharacterDataTransferComponent->GetAllBackpackItems(BackpackItems);
	HandleBackpackItemsChanged(BackpackItems);

	TArray<ULxEquipmentSlotData*> EquipmentSlots;
	m_pCharacterDataTransferComponent->GetAllEquipment(EquipmentSlots);
	HandleEquipmentSlotsChanged(EquipmentSlots);
}

void ULxBackpackWidget::SortingOfItems()
{
	if (m_pCharacterDataTransferComponent)
	{
		// 操作入口也走数据中转组件，排序后的刷新由中转事件回流。
		m_pCharacterDataTransferComponent->SortBackpackItems();
	}
}

TArray<UObject*> ULxBackpackWidget::GetItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (ULxItemSlotData* ItemSlot : m_vItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = ItemSlot;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

TArray<UObject*> ULxBackpackWidget::GetEquipmentUIDataList()
{
	TArray<UObject*> EquipmentUIDataList;
	for (ULxEquipmentSlotData* EquipmentSlot : m_vEquipmentSlotList)
	{
		ULxItemUIData* EquipmentUIData = NewObject<ULxItemUIData>(this);
		EquipmentUIData->m_pSlotData = EquipmentSlot;
		EquipmentUIDataList.Add(EquipmentUIData);
	}
	return EquipmentUIDataList;
}

void ULxBackpackWidget::SwitchItemType(ELxItemType NewType)
{
	m_vItemSlotList.Reset();

	if (m_pCharacterDataTransferComponent)
	{
		TArray<ULxItemSlotData*> FilteredItems;
		m_pCharacterDataTransferComponent->QueryBackpackItemsByFilter(NewType, ELxItemRarityType::None, FilteredItems);
		HandleBackpackItemsChanged(FilteredItems);
		return;
	}

	OnItemListUpdated(GetItemUIDataList());
}

void ULxBackpackWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
{
	if (m_pCharacterDataTransferComponent == InDataTransferComponent)
	{
		return;
	}

	UnbindDataTransferComponent();
	m_pCharacterDataTransferComponent = InDataTransferComponent;

	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnBackpackItemChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackItemsChanged);
	m_pCharacterDataTransferComponent->OnBackpackItemChanged.AddDynamic(this, &ULxBackpackWidget::HandleBackpackItemsChanged);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleEquipmentSlotsChanged);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.AddDynamic(this, &ULxBackpackWidget::HandleEquipmentSlotsChanged);
}

void ULxBackpackWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnBackpackItemChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleBackpackItemsChanged);
	m_pCharacterDataTransferComponent->OnEquipmentChanged.RemoveDynamic(this, &ULxBackpackWidget::HandleEquipmentSlotsChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxBackpackWidget::HandleBackpackItemsChanged(const TArray<ULxItemSlotData*>& BackpackItems)
{
	m_vItemSlotList.Reset();
	for (ULxItemSlotData* SlotData : BackpackItems)
	{
		m_vItemSlotList.Add(SlotData);
	}
	OnItemListUpdated(GetItemUIDataList());
}

void ULxBackpackWidget::HandleEquipmentSlotsChanged(const TArray<ULxEquipmentSlotData*>& EquipmentSlots)
{
	m_vEquipmentSlotList.Reset();
	for (ULxEquipmentSlotData* SlotData : EquipmentSlots)
	{
		m_vEquipmentSlotList.Add(SlotData);
	}
	OnEquipmentListUpdated(GetEquipmentUIDataList());
}
