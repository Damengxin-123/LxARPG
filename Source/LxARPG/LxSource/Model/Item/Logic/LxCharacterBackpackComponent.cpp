#include "LxCharacterBackpackComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemData/LxItemData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterialData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTable.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTableTypeEnum.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"

ULxCharacterBackpackComponent::ULxCharacterBackpackComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterBackpackComponent::BaseComponentInitialize()
{
	if (m_bBackpackInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	InitializeBackpackSlots();
	m_bBackpackInitialized = true;
}

bool ULxCharacterBackpackComponent::AddItemByRowID(ELxItemType InItemType, FName InItemRowID, int32 InItemCount)
{
	if (InItemCount <= 0)
	{
		return false;
	}

	ULxItemData* ItemData = CreateItemByRowID(InItemType, InItemRowID);
	if (!ItemData)
	{
		return false;
	}

	ItemData->GetItemBase().ItemCount = InItemCount;
	return AddExistingItem(ItemData);
}

bool ULxCharacterBackpackComponent::AddItemByStruct(const FInstancedStruct& InItemData)
{
	if (!InItemData.IsValid())
	{
		return false;
	}

	ULxItemData* ItemData = ULxItemData::CreateNewItemData(this, InItemData);
	return AddExistingItem(ItemData);
}

bool ULxCharacterBackpackComponent::AddExistingItem(ULxItemData* InItemData)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!InItemData || !InItemData->IsValid())
	{
		return false;
	}

	TryStackItemIntoInventory(InItemData);
	if (!InItemData->IsValid())
	{
		BroadcastBackpackChanged();
		return true;
	}

	const int32 EmptyIndex = FindEmptySlotIndex();
	if (EmptyIndex == INDEX_NONE)
	{
		return false;
	}

	m_vCharacterItems[EmptyIndex] = InItemData;
	BroadcastBackpackChanged();
	return true;
}

bool ULxCharacterBackpackComponent::AddExistingItemAt(ULxItemData* InItemData, int32 InDestinationIndex)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!InItemData || !InItemData->IsValid() || !IsValidBackpackIndex(InDestinationIndex))
	{
		return false;
	}

	ULxItemData* DestinationItem = m_vCharacterItems[InDestinationIndex];
	if (!DestinationItem)
	{
		m_vCharacterItems[InDestinationIndex] = InItemData;
		BroadcastBackpackChanged();
		return true;
	}

	bool bSourceConsumed = false;
	if (!TryStackItemIntoSlot(DestinationItem, InItemData, bSourceConsumed))
	{
		return false;
	}

	if (!bSourceConsumed)
	{
		return AddExistingItem(InItemData);
	}

	BroadcastBackpackChanged();
	return true;
}

bool ULxCharacterBackpackComponent::AddItemAtFromExternal(ULxItemData* InItemData, int32 InDestinationIndex)
{
	if (!InItemData || !InItemData->IsValid())
	{
		return false;
	}

	ULxItemData* ItemCopy = ULxItemData::CreateNewItemData(this, InItemData->GetItemDataCopy());
	return AddExistingItemAt(ItemCopy, InDestinationIndex);
}

bool ULxCharacterBackpackComponent::MoveItem(int32 InSourceIndex, int32 InDestinationIndex)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!IsValidBackpackIndex(InSourceIndex) || !IsValidBackpackIndex(InDestinationIndex) || InSourceIndex == InDestinationIndex)
	{
		return false;
	}

	ULxItemData* SourceItem = m_vCharacterItems[InSourceIndex];
	if (!SourceItem)
	{
		return false;
	}

	ULxItemData* DestinationItem = m_vCharacterItems[InDestinationIndex];
	if (!DestinationItem)
	{
		m_vCharacterItems[InDestinationIndex] = SourceItem;
		m_vCharacterItems[InSourceIndex] = nullptr;
		BroadcastBackpackChanged();
		return true;
	}

	if (!StackItem(DestinationItem, SourceItem))
	{
		Swap(m_vCharacterItems[InSourceIndex], m_vCharacterItems[InDestinationIndex]);
	}
	else if (!SourceItem->IsValid())
	{
		m_vCharacterItems[InSourceIndex] = nullptr;
	}

	BroadcastBackpackChanged();
	return true;
}

bool ULxCharacterBackpackComponent::ConsumeItemAt(int32 InIndex)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!IsValidBackpackIndex(InIndex))
	{
		return false;
	}

	ULxItemData* ItemData = m_vCharacterItems[InIndex];
	if (!ItemData || ItemData->GetItemType() != ELxItemType::Consumable)
	{
		return false;
	}

	FLxItemBase& ItemBase = ItemData->GetItemBase();
	if (ItemBase.ItemCount <= 0)
	{
		return false;
	}

	--ItemBase.ItemCount;
	if (ItemBase.ItemCount <= 0)
	{
		m_vCharacterItems[InIndex] = nullptr;
	}

	BroadcastBackpackChanged();
	return true;
}

bool ULxCharacterBackpackComponent::RemoveItemAt(int32 InIndex)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!IsValidBackpackIndex(InIndex) || !m_vCharacterItems[InIndex])
	{
		return false;
	}

	m_vCharacterItems[InIndex] = nullptr;
	BroadcastBackpackChanged();
	return true;
}

void ULxCharacterBackpackComponent::SortingOfItems()
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	// 通过数据结构体中自带的比较函数进行排序
	for (int i = 0; i < m_vCharacterItems.Num(); i ++)
	{
		for (int j = 0; j < m_vCharacterItems.Num() - i - 1; j ++)
		{
			if (m_vCharacterItems[j] == nullptr && m_vCharacterItems[j + 1] != nullptr)
			{
				m_vCharacterItems[j] = m_vCharacterItems[j + 1];
				m_vCharacterItems[j + 1] = nullptr;
				continue;
			}
			if (m_vCharacterItems[j] == nullptr || m_vCharacterItems[j + 1] == nullptr)
			{
				continue;
			}
			if (m_vCharacterItems[j]->GetItemBase() > m_vCharacterItems[j + 1]->GetItemBase())
			{
				ULxItemData* temp = m_vCharacterItems[j];
				m_vCharacterItems[j] = m_vCharacterItems[j + 1];
				m_vCharacterItems[j + 1] = temp;
				temp = nullptr;
			}
		}
	}
	
	BroadcastBackpackChanged();
}

ULxItemData* ULxCharacterBackpackComponent::GetItemAt(int32 InIndex) const
{
	if (!IsValidBackpackIndex(InIndex))
	{
		return nullptr;
	}

	return m_vCharacterItems[InIndex];
}

TArray<TObjectPtr<ULxItemData>>& ULxCharacterBackpackComponent::GetItems()
{
	return m_vCharacterItems;
}

TArray<TObjectPtr<ULxItemData>> ULxCharacterBackpackComponent::QueryTypeItem(ELxItemType InItemType) const
{
	TArray<TObjectPtr<ULxItemData>> Result;

	for (ULxItemData* ItemData : m_vCharacterItems)
	{
		if (!ItemData)
		{
			continue;
		}

		if (InItemType == ELxItemType::None || ItemData->GetItemType() == InItemType)
		{
			Result.Add(ItemData);
		}
	}

	return Result;
}

int32 ULxCharacterBackpackComponent::GetBackpackSlotCount() const
{
	return ITEM_SLOT_COUNT;
}

ULxItemData* ULxCharacterBackpackComponent::TakeItemAt(int32 InIndex)
{
	if (!m_bBackpackInitialized)
	{
		BaseComponentInitialize();
	}

	if (!IsValidBackpackIndex(InIndex))
	{
		return nullptr;
	}

	ULxItemData* ItemData = m_vCharacterItems[InIndex];
	m_vCharacterItems[InIndex] = nullptr;
	if (ItemData)
	{
		BroadcastBackpackChanged();
	}
	return ItemData;
}

void ULxCharacterBackpackComponent::InitializeBackpackSlots()
{
	m_vCharacterItems.SetNumZeroed(GetBackpackSlotCount());
}

bool ULxCharacterBackpackComponent::IsValidBackpackIndex(int32 InIndex) const
{
	return m_vCharacterItems.IsValidIndex(InIndex);
}

int32 ULxCharacterBackpackComponent::FindEmptySlotIndex() const
{
	return m_vCharacterItems.Find(nullptr);
}

bool ULxCharacterBackpackComponent::TryStackItemIntoInventory(ULxItemData* InItemData)
{
	for (ULxItemData* ExistingItem : m_vCharacterItems)
	{
		if (!ExistingItem)
		{
			continue;
		}

		if (StackItem(ExistingItem, InItemData) && !InItemData->IsValid())
		{
			return true;
		}
	}

	return !InItemData->IsValid();
}

bool ULxCharacterBackpackComponent::TryStackItemIntoSlot(ULxItemData* InTargetItem, ULxItemData* InSourceItem, bool& bOutSourceConsumed) const
{
	bOutSourceConsumed = false;
	if (!StackItem(InTargetItem, InSourceItem))
	{
		return false;
	}

	bOutSourceConsumed = !InSourceItem->IsValid();
	return true;
}

bool ULxCharacterBackpackComponent::StackItem(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const
{
	if (!CanItemsStack(InTargetItem, InSourceItem))
	{
		return false;
	}

	FLxItemBase& TargetBase = InTargetItem->GetItemBase();
	FLxItemBase& SourceBase = InSourceItem->GetItemBase();
	const int32 RemainingSpace = TargetBase.ItemMaxCount - TargetBase.ItemCount;
	if (RemainingSpace <= 0)
	{
		return false;
	}

	const int32 TransferCount = FMath::Min(RemainingSpace, SourceBase.ItemCount);
	TargetBase.ItemCount += TransferCount;
	SourceBase.ItemCount -= TransferCount;

	return TransferCount > 0;
}

bool ULxCharacterBackpackComponent::CanItemsStack(ULxItemData* InTargetItem, ULxItemData* InSourceItem) const
{
	if (!InTargetItem || !InSourceItem || InTargetItem == InSourceItem)
	{
		return false;
	}

	FLxItemBase& TargetBase = InTargetItem->GetItemBase();
	FLxItemBase& SourceBase = InSourceItem->GetItemBase();

	return TargetBase.IsValid()
		&& SourceBase.IsValid()
		&& TargetBase.ItemCanStack
		&& SourceBase.ItemCanStack
		&& TargetBase.RowID == SourceBase.RowID
		&& TargetBase.ItemCount < TargetBase.ItemMaxCount;
}

ULxItemData* ULxCharacterBackpackComponent::CreateItemByRowID(ELxItemType InItemType, FName InItemRowID)
{
	if (InItemRowID.IsNone())
	{
		return nullptr;
	}

	ULxGameInstanceSubsystem* GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
	if (!GameInstanceSubsystem)
	{
		return nullptr;
	}

	const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem->GetGameDataManager();
	if (!GameDataTablesManager)
	{
		return nullptr;
	}

	switch (InItemType)
	{
	case ELxItemType::Equipment:
		{
			const ULxDataTable* DataTable = GameDataTablesManager->GetDataTables(ELxDataTableTypeEnum::EquipmentData);
			const FLxEquipmentData* RowData = DataTable ? DataTable->GetData<FLxEquipmentData>(InItemRowID) : nullptr;
			return RowData ? ULxItemData::CreateNewEquipmentItemData(this, *RowData) : nullptr;
		}
	case ELxItemType::Consumable:
		{
			const ULxDataTable* DataTable = GameDataTablesManager->GetDataTables(ELxDataTableTypeEnum::ConsumableData);
			const FLxConsumableData* RowData = DataTable ? DataTable->GetData<FLxConsumableData>(InItemRowID) : nullptr;
			return RowData ? ULxItemData::CreateNewConsumableItemData(this, *RowData) : nullptr;
		}
	case ELxItemType::Material:
		{
			const ULxDataTable* DataTable = GameDataTablesManager->GetDataTables(ELxDataTableTypeEnum::MaterialData);
			const FLxMaterialData* RowData = DataTable ? DataTable->GetData<FLxMaterialData>(InItemRowID) : nullptr;
			return RowData ? ULxItemData::CreateNewMaterialItemData(this, *RowData) : nullptr;
		}
	default:
		return nullptr;
	}
}

void ULxCharacterBackpackComponent::BroadcastBackpackChanged()
{
	OnBackpackChanged.Broadcast();
}
