#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Item/DataType/Buff/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Skill/LxSkillLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterBackpackComponent::ULxCharacterBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterBackpackComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	m_pOwnerCharacter = GetCharacterOwner();
	InitializeBackpack();
}

bool ULxCharacterBackpackComponent::AddItemByRowID(ELxItemType InItemType, FName InItemID, int32 InItemCount)
{
	// 此处需要去查询默认物品数据表，然后创建物品数据
	return false;
}

bool ULxCharacterBackpackComponent::RemoveItemAt(ELxItemType InItemType, FName InItemID, int32 InItemCount)
{
	for (auto& item : m_vItemList)
	{
		if (item->GetItemDataBase()->ItemInfo.ItemType == InItemType && item->GetItemDataBase()->ItemInfo.ItemID == InItemID)
		{
			if (item->GetItemDataBase()->ItemCount >= InItemCount)
			{
				// 此处需要进行物品数量的减少，和物品的移除
				return true;
			}
			else
			{
				// 此处需要进行物品数量的减少，和物品的移除
				return true;
			}
		}
	}
	return false;
}

bool ULxCharacterBackpackComponent::CheckHaveItem(ELxItemType InItemType, FName InItemID, int32 InItemCount) const
{
	for (auto& item : m_vItemList)
	{
		if (item->GetItemDataBase()->ItemInfo.ItemType == InItemType && item->GetItemDataBase()->ItemInfo.ItemID == InItemID)
		{
			if (item->GetItemDataBase()->ItemCount >= InItemCount)
			{
				// 物品数量满足
				return true;
			}
			else
			{
				// 当前不太满足，需要继续遍历剩余物品
				InItemCount -= item->GetItemDataBase()->ItemCount;
				continue;
			}
		}
	}
	return InItemCount <= 0;
}

void ULxCharacterBackpackComponent::SortingOfItems()
{
	// 使用一个简单的冒泡排序
	for (int i = 0;i < m_vItemList.Num() - 1; i++)
	{
		for (int j = 0; j < m_vItemList.Num() - 1; j ++)
		{
			if (m_vItemList[j] < m_vItemList[j + 1])
			{
				std::swap(m_vItemList[i], m_vItemList[j]);
			}
		}
	}
	// 先清空所有槽位
	for (auto& slot : m_vBackpackSlots)
	{
		slot->ClearItem();
	}
	// 将排序后的物品放入槽位中
	for (int i = 0;i < m_vItemList.Num() - 1; i++)
	{
		m_vBackpackSlots[i]->SetItem(m_vItemList[i]);
	}
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::GetAllItems()
{
	return m_vBackpackSlots;
}

TArray<TObjectPtr<ULxItemSlotData>>& ULxCharacterBackpackComponent::QueryItemsOnItemType(ELxItemType InItemType)
{
	m_vFilteringCache.Empty();
	// 遍历所有槽位，将符合条件的物品类型，放入缓存中，
	for (auto& slot : m_vFilteringCache)
	{
		if (slot->ItemDataPtr && slot->ItemDataPtr->GetItemDataBase()->ItemInfo.ItemType == InItemType)
		{
			m_vFilteringCache.Add(slot);
		}
	}
	return m_vFilteringCache;
}

void ULxCharacterBackpackComponent::InitializeBackpack()
{
	// 初始化变量
	
	m_vFilteringCache.Empty();
	m_vBackpackSlots.Empty();
	m_vItemList.Empty();

	// 此处应当查询存档系统，获取此角色的物品信息

	// 初始化物品栏槽位
	for (int32 i = 0; i < BackpackSlotCount; ++i)
	{
		ULxItemSlotData* NewSlot = NewObject<ULxItemSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Backpack;
		NewSlot->ID = i;
		m_vBackpackSlots.Add(NewSlot);
	}
	// 此处应当查询存档系统，获取此角色的背包物品存放情况，然后用于初始化所有的槽位
	
	
}

