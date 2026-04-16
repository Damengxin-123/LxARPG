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
	// 暂时没有啥要更新的
}

void ULxBackpackWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);
	UpdatedBackpack();
}
void ULxBackpackWidget::UpdatedBackpack()
{
	// 获取到当前控制器持有的角色的背包组件和装备组件
	if (m_pPlayerCharacter)
	{
		m_pCharacterBackpackComponent = m_pPlayerCharacter->GetCharacterBackpackComponent();
		m_pCharacterEquipmentComponent = m_pPlayerCharacter->GetCharacterEquipmentComponent();
	}
	// 暂时不用绑定组件的事件，而是由物品格子自己更新物品槽位内的物品信息
	// 获取装备槽位和物品槽位 进行缓存
	if (m_pCharacterBackpackComponent)
	{
		m_vItemSlotList = m_pCharacterBackpackComponent->GetAllItems();
	}
	if (m_pCharacterEquipmentComponent)
	{
		m_vEquipmentSlotList = m_pCharacterEquipmentComponent->GetEquipmentSlots();
	}
	// 判断，如果获取到了物品和装备，则发出更新事件
	if (!m_vItemSlotList.IsEmpty())
	{
		OnItemListUpdated.Broadcast();
	}
	if (!m_vEquipmentSlotList.IsEmpty())
	{
		OnEquipmentListUpdated.Broadcast();
	}
}
void ULxBackpackWidget::SortingOfItems()
{
	// 先调用排序函数
	if (m_pCharacterBackpackComponent)
	{
		m_pCharacterBackpackComponent->SortingOfItems();
	}
	// 然后再获取物品列表缓存
	if (m_pCharacterEquipmentComponent)
	{
		m_vItemSlotList = m_pCharacterBackpackComponent->GetAllItems();
	}
	// 判断，如果获取到了物品和装备，则发出更新事件
	if (!m_vItemSlotList.IsEmpty())
	{
		OnItemListUpdated.Broadcast();
	}
}

TArray<UObject*> ULxBackpackWidget::GetItemUIDataList()
{
	TArray<UObject*> ItemUIDataList;
	for (auto& item :  m_vItemSlotList)
	{
		ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(this);
		ItemUIData->m_pSlotData = item;
		ItemUIDataList.Add(ItemUIData);
	}
	return ItemUIDataList;
}

TArray<UObject*> ULxBackpackWidget::GetEquipmentUIDataList()
{
	TArray<UObject*> EquipmentUIDataList;
	for (auto& equ :  m_vEquipmentSlotList)
	{
		ULxItemUIData* EquipmentUIData = NewObject<ULxItemUIData>(this);
		EquipmentUIData->m_pSlotData = equ;
		EquipmentUIDataList.Add(EquipmentUIData);
	}
	return EquipmentUIDataList;
}

void ULxBackpackWidget::SwitchItemType(ELxItemType NewType)
{
	if (m_pCharacterBackpackComponent)
	{
		m_vItemSlotList = m_pCharacterBackpackComponent->QueryItemsOnItemType(NewType);
	}
	// 判断，如果获取到了物品和装备，则发出更新事件
	if (!m_vItemSlotList.IsEmpty())
	{
		OnItemListUpdated.Broadcast();
	}
}
