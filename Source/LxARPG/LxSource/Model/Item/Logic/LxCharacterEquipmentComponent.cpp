#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemData/LxItemData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	void AddAttributeValue(int32& InOutValue, int32 InValue)
	{
		if (InValue == ERR_ATTRIBUTE)
		{
			return;
		}

		if (InOutValue == ERR_ATTRIBUTE)
		{
			InOutValue = InValue;
			return;
		}

		InOutValue += InValue;
	}

	void MergeEntryValue(FLxAttributeValueSet& InOutValue, const FLxAttributeValueSet& InValue)
	{
		AddAttributeValue(InOutValue.m_nMaxValue, InValue.m_nMaxValue);
		AddAttributeValue(InOutValue.m_nMinValue, InValue.m_nMinValue);
		AddAttributeValue(InOutValue.m_nMaxFloatRatio, InValue.m_nMaxFloatRatio);
		AddAttributeValue(InOutValue.m_nMinFloatRatio, InValue.m_nMinFloatRatio);
	}

	FString BuildEntryMergeKey(const FLxItemEntry& InEntry)
	{
		return FString::Printf(TEXT("%s_%d"), *InEntry.AppliedToAttRow.RowName.ToString(), static_cast<int32>(InEntry.EntryType));
	}
}

ULxCharacterEquipmentComponent::ULxCharacterEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterEquipmentComponent::BaseComponentInitialize()
{
	if (m_bEquipmentInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	InitializeEquipmentSlots();
	m_bEquipmentInitialized = true;
}

bool ULxCharacterEquipmentComponent::EquipItemFromBackpack(int32 InBackpackIndex)
{
	if (!m_bEquipmentInitialized)
	{
		BaseComponentInitialize();
	}

	if (!m_pOwnerCharacter)
	{
		return false;
	}

	ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter->GetCharacterBackpackComponent();
	if (!BackpackComponent)
	{
		return false;
	}

	ULxItemData* BackpackItem = BackpackComponent->GetItemAt(InBackpackIndex);
	if (!BackpackItem || BackpackItem->GetItemType() != ELxItemType::Equipment)
	{
		return false;
	}

	return EquipItemFromBackpackToSlot(InBackpackIndex, BackpackItem->GetEquipmentItemData().EquipmentType);
}

bool ULxCharacterEquipmentComponent::EquipItemFromBackpackToSlot(int32 InBackpackIndex, ELxEquipmentType InEquipmentType)
{
	if (!m_bEquipmentInitialized)
	{
		BaseComponentInitialize();
	}

	if (!m_pOwnerCharacter || !IsValidEquipmentType(InEquipmentType))
	{
		return false;
	}

	ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter->GetCharacterBackpackComponent();
	if (!BackpackComponent)
	{
		return false;
	}

	ULxItemData* BackpackItem = BackpackComponent->GetItemAt(InBackpackIndex);
	if (!BackpackItem || BackpackItem->GetItemType() != ELxItemType::Equipment)
	{
		return false;
	}

	if (BackpackItem->GetEquipmentItemData().EquipmentType != InEquipmentType)
	{
		return false;
	}

	ULxItemData* TakenItem = BackpackComponent->TakeItemAt(InBackpackIndex);
	if (!TakenItem)
	{
		return false;
	}

	if (!EquipResolvedItem(TakenItem, InEquipmentType))
	{
		BackpackComponent->AddExistingItem(TakenItem);
		return false;
	}

	return true;
}

bool ULxCharacterEquipmentComponent::EquipItemFromExternal(ULxItemData* InItemData, ELxEquipmentType InEquipmentType)
{
	if (!InItemData || InItemData->GetItemType() != ELxItemType::Equipment || !IsValidEquipmentType(InEquipmentType))
	{
		return false;
	}

	if (InItemData->GetEquipmentItemData().EquipmentType != InEquipmentType)
	{
		return false;
	}

	ULxItemData* ItemCopy = ULxItemData::CreateNewItemData(this, InItemData->GetItemDataCopy());
	return EquipResolvedItem(ItemCopy, InEquipmentType);
}

bool ULxCharacterEquipmentComponent::UnequipItemToBackpack(ELxEquipmentType InEquipmentType)
{
	if (!m_bEquipmentInitialized)
	{
		BaseComponentInitialize();
	}

	if (!m_pOwnerCharacter || !IsValidEquipmentType(InEquipmentType))
	{
		return false;
	}

	ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter->GetCharacterBackpackComponent();
	if (!BackpackComponent)
	{
		return false;
	}

	const int32 EquipmentIndex = GetEquipmentIndex(InEquipmentType);
	ULxItemData* EquippedItem = m_vEquipmentItems[EquipmentIndex];
	if (!EquippedItem)
	{
		return false;
	}

	if (!BackpackComponent->AddExistingItem(EquippedItem))
	{
		return false;
	}

	m_vEquipmentItems[EquipmentIndex] = nullptr;
	BroadcastEquipmentChanged();
	return true;
}

bool ULxCharacterEquipmentComponent::UnequipItemToBackpackAt(ELxEquipmentType InEquipmentType, int32 InBackpackIndex)
{
	if (!m_bEquipmentInitialized)
	{
		BaseComponentInitialize();
	}

	if (!m_pOwnerCharacter || !IsValidEquipmentType(InEquipmentType))
	{
		return false;
	}

	ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter->GetCharacterBackpackComponent();
	if (!BackpackComponent)
	{
		return false;
	}

	const int32 EquipmentIndex = GetEquipmentIndex(InEquipmentType);
	ULxItemData* EquippedItem = m_vEquipmentItems[EquipmentIndex];
	if (!EquippedItem || !BackpackComponent->AddExistingItemAt(EquippedItem, InBackpackIndex))
	{
		return false;
	}

	m_vEquipmentItems[EquipmentIndex] = nullptr;
	BroadcastEquipmentChanged();
	return true;
}

ULxItemData* ULxCharacterEquipmentComponent::GetEquipmentAt(ELxEquipmentType InEquipmentType) const
{
	if (!IsValidEquipmentType(InEquipmentType))
	{
		return nullptr;
	}

	return m_vEquipmentItems[GetEquipmentIndex(InEquipmentType)];
}

TArray<TObjectPtr<ULxItemData>>& ULxCharacterEquipmentComponent::GetAllEquipment()
{
	return m_vEquipmentItems;
}

TArray<FLxItemEntry> ULxCharacterEquipmentComponent::GetTotalEquipmentEntryList() const
{
	TArray<FLxItemEntry> TotalEntryList;
	TMap<FString, int32> EntryIndexMap;

	for (ULxItemData* ItemData : m_vEquipmentItems)
	{
		if (!ItemData || ItemData->GetItemType() != ELxItemType::Equipment)
		{
			continue;
		}

		FLxEquipmentData& EquipmentData = ItemData->GetEquipmentItemData();
		TArray<const FLxItemEntry*> EntryList;

		if (EquipmentData.BasicEntry.m_fEntryValue.IsValid())
		{
			EntryList.Add(&EquipmentData.BasicEntry);
		}

		for (const FLxItemEntry& Entry : EquipmentData.ExtendEntryList)
		{
			if (Entry.m_fEntryValue.IsValid())
			{
				EntryList.Add(&Entry);
			}
		}

		for (const FLxItemEntry* Entry : EntryList)
		{
			if (!Entry)
			{
				continue;
			}

			const FString EntryKey = BuildEntryMergeKey(*Entry);
			if (const int32* ExistingIndex = EntryIndexMap.Find(EntryKey))
			{
				MergeEntryValue(TotalEntryList[*ExistingIndex].m_fEntryValue, Entry->m_fEntryValue);
			}
			else
			{
				const int32 NewIndex = TotalEntryList.Add(*Entry);
				EntryIndexMap.Add(EntryKey, NewIndex);
			}
		}
	}

	return TotalEntryList;
}

int32 ULxCharacterEquipmentComponent::GetEquipmentSlotCount() const
{
	return static_cast<int32>(ELxEquipmentType::EquipmentSizeMax);
}

void ULxCharacterEquipmentComponent::InitializeEquipmentSlots()
{
	m_vEquipmentItems.SetNumZeroed(GetEquipmentSlotCount());
}

bool ULxCharacterEquipmentComponent::IsValidEquipmentType(ELxEquipmentType InEquipmentType) const
{
	const int32 EquipmentIndex = static_cast<int32>(InEquipmentType);
	return EquipmentIndex >= 0 && EquipmentIndex < GetEquipmentSlotCount();
}

int32 ULxCharacterEquipmentComponent::GetEquipmentIndex(ELxEquipmentType InEquipmentType) const
{
	return static_cast<int32>(InEquipmentType);
}

bool ULxCharacterEquipmentComponent::EquipResolvedItem(ULxItemData* InItemData, ELxEquipmentType InEquipmentType)
{
	if (!InItemData || InItemData->GetItemType() != ELxItemType::Equipment || !m_pOwnerCharacter || !IsValidEquipmentType(InEquipmentType))
	{
		return false;
	}

	ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter->GetCharacterBackpackComponent();
	if (!BackpackComponent)
	{
		return false;
	}

	const int32 EquipmentIndex = GetEquipmentIndex(InEquipmentType);
	ULxItemData* ReplacedItem = m_vEquipmentItems[EquipmentIndex];
	m_vEquipmentItems[EquipmentIndex] = InItemData;

	if (ReplacedItem && !BackpackComponent->AddExistingItem(ReplacedItem))
	{
		m_vEquipmentItems[EquipmentIndex] = ReplacedItem;
		return false;
	}

	BroadcastEquipmentChanged();
	return true;
}

void ULxCharacterEquipmentComponent::BroadcastEquipmentChanged()
{
	OnEquipmentChanged.Broadcast();
}
