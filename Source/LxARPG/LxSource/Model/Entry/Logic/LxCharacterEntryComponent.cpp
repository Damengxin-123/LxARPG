#include "LxCharacterEntryComponent.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Entry/Logic/LxItemEntryLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	bool IsValidEntry(const ULxItemEntryLogic* InEntryLogic)
	{
		return InEntryLogic != nullptr && InEntryLogic->IsEntryValid();
	}

	void AppendEntryIfValid(ULxItemEntryLogic* InEntryLogic, TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList)
	{
		if (IsValidEntry(InEntryLogic))
		{
			OutEntryList.Add(InEntryLogic);
		}
	}

	void AppendConsumableEntries(const ULxItemLogicBase* InUsedItem, TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList)
	{
		const ULxConsumableLogic* ConsumableLogic = Cast<ULxConsumableLogic>(InUsedItem);
		if (ConsumableLogic == nullptr)
		{
			return;
		}

		const FLxConsumableData* ConsumableData = ConsumableLogic->GetConsumableData();
		if (ConsumableData == nullptr)
		{
			return;
		}

		for (ULxItemEntryLogic* EntryLogic : ConsumableData->ConsumableEntryInfo.ConsumableEntryList)
		{
			AppendEntryIfValid(EntryLogic, OutEntryList);
		}
	}

	void AppendEntryByLogicType(ULxItemEntryLogic* InEntryLogic, FLxCharacterEntryPackage& OutEntryPackage)
	{
		if (!IsValidEntry(InEntryLogic))
		{
			return;
		}

		const FLxItemEntryData& EntryData = InEntryLogic->GetItemEntryData();
		switch (EntryData.EnteryBaseInfo.EntryLogicType)
		{
		case ELxEntryLogicType::ChangeAttributeValue:
			OutEntryPackage.CharacterAttributeEntryList.Add(InEntryLogic);
			if (EntryData.ChangeAttributeValue.EntryValueInfo.EntryTarget == ELxItemEntryTarget::ToValue)
			{
				OutEntryPackage.AttributeRecoveryEntryList.Add(InEntryLogic);
			}
			break;
		case ELxEntryLogicType::CreaterBuffer:
			OutEntryPackage.BuffEntryList.Add(InEntryLogic);
			break;
		case ELxEntryLogicType::BufferEnter:
			OutEntryPackage.BuffEntryList.Add(InEntryLogic);
			break;
		case ELxEntryLogicType::ChangeCharacterState:
			OutEntryPackage.StatusEntryList.Add(InEntryLogic);
			break;
		case ELxEntryLogicType::TextEnter:
			OutEntryPackage.UtilityEntryList.Add(InEntryLogic);
			break;
		default:
			break;
		}
	}
}

ULxCharacterEntryComponent::ULxCharacterEntryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterEntryComponent::BaseComponentInitialize()
{
	if (m_bEntryInitialized)
	{
		return;
	}

	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = GetCharacterOwner();
	}

	if (ULxCharacterEquipmentComponent* EquipmentComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEquipmentComponent() : nullptr)
	{
		EquipmentComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterEntryComponent::HandleEquipmentDataChange);
		EquipmentComponent->OnDataChange.AddDynamic(this, &ULxCharacterEntryComponent::HandleEquipmentDataChange);
	}

	if (ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterBackpackComponent() : nullptr)
	{
		BackpackComponent->OnItemUsed.RemoveDynamic(this, &ULxCharacterEntryComponent::HandleBackpackItemUsed);
		BackpackComponent->OnItemUsed.AddDynamic(this, &ULxCharacterEntryComponent::HandleBackpackItemUsed);
	}

	if (ULxCharacterBuffComponent* BuffComponent = GetOwnerBuffComponent())
	{
		BuffComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterEntryComponent::HandleBuffDataChange);
		BuffComponent->OnDataChange.AddDynamic(this, &ULxCharacterEntryComponent::HandleBuffDataChange);
	}

	m_bEntryInitialized = true;
	RefreshInstalledEntries();
}

void ULxCharacterEntryComponent::GetInstalledEntries(TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const
{
	OutEntryList = m_vInstalledEntries;
}

void ULxCharacterEntryComponent::DispatchBuffEntries(const TArray<TObjectPtr<ULxItemEntryLogic>>& InEntryList)
{
	for (ULxItemEntryLogic* EntryLogic : InEntryList)
	{
		if (!IsValidEntry(EntryLogic))
		{
			continue;
		}

		const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
		const FLxEntryValueInfo& EntryValueInfo = EntryData.BufferEnterValue.EntryValueInfo;
		if (EntryData.EnteryBaseInfo.EntryLogicType != ELxEntryLogicType::BufferEnter
			|| EntryData.EnteryBaseInfo.TargetTags.IsEmpty()
			|| EntryValueInfo.EntryTarget != ELxItemEntryTarget::ToValue
			|| EntryValueInfo.EffectivePeriod < 0.f)
		{
			continue;
		}

		const float EffectiveValue = EntryLogic->GetEffectiveValue();
		if (FMath::IsNearlyZero(EffectiveValue))
		{
			continue;
		}

		float TickEffectiveValue = EffectiveValue;
		if (EntryValueInfo.EffectivePeriod > 0.f)
		{
			TickEffectiveValue = EffectiveValue * BUFF_COMPONENT_TIMER_INTERVAL / EntryValueInfo.EffectivePeriod;
		}

		if (FMath::IsNearlyZero(TickEffectiveValue))
		{
			continue;
		}

		EntryLogic->SetTemporaryEffectiveValue(TickEffectiveValue);
		OnAttributeRecoveryEntryApplied.Broadcast(EntryLogic);
		EntryLogic->ClearTemporaryEffectiveValue();
	}
}

void ULxCharacterEntryComponent::HandleEquipmentDataChange()
{
	if (!m_bEntryInitialized)
	{
		BaseComponentInitialize();
		return;
	}

	RefreshInstalledEntries();
}

void ULxCharacterEntryComponent::HandleBackpackItemUsed(ULxItemLogicBase* UsedItem)
{
	if (UsedItem == nullptr || !UsedItem->ItemIsValid())
	{
		return;
	}

	TArray<TObjectPtr<ULxItemEntryLogic>> UsedEntryList;
	AppendConsumableEntries(UsedItem, UsedEntryList);
	DispatchUsedItemEntries(UsedEntryList);
}

void ULxCharacterEntryComponent::HandleBuffDataChange()
{
	if (!m_bEntryInitialized || m_bRefreshingInstalledEntries)
	{
		return;
	}

	RefreshInstalledEntries();
}

void ULxCharacterEntryComponent::RefreshInstalledEntries()
{
	TGuardValue<bool> RefreshingGuard(m_bRefreshingInstalledEntries, true);

	ClearEquipmentGeneratedBuffs();

	m_vInstalledEntries.Reset();
	m_EntryPackage = FLxCharacterEntryPackage();

	if (const ULxCharacterEquipmentComponent* EquipmentComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEquipmentComponent() : nullptr)
	{
		AppendEquipmentEntries(*EquipmentComponent, m_vInstalledEntries);
	}

	BuildEntryPackage();
	ApplyEquipmentBuffEntries();
	BuildEntryPackage();

	OnDataChange.Broadcast();
	OnEntryPackageChanged.Broadcast(m_EntryPackage);
}

void ULxCharacterEntryComponent::BuildEntryPackage()
{
	m_EntryPackage = FLxCharacterEntryPackage();
	m_EntryPackage.InstalledEntryList = m_vInstalledEntries;

	for (ULxItemEntryLogic* EntryLogic : m_vInstalledEntries)
	{
		AppendEntryByLogicType(EntryLogic, m_EntryPackage);
	}

	TArray<TObjectPtr<ULxItemEntryLogic>> ActiveBuffEntries;
	AppendActiveBuffEntries(ActiveBuffEntries);
	for (ULxItemEntryLogic* EntryLogic : ActiveBuffEntries)
	{
		if (!IsValidEntry(EntryLogic))
		{
			continue;
		}

		m_EntryPackage.BuffEntryList.Add(EntryLogic);

		const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
		if (EntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::BufferEnter
			&& EntryData.BufferEnterValue.EntryValueInfo.EffectivePeriod < 0.f
			&& !EntryData.EnteryBaseInfo.TargetTags.IsEmpty())
		{
			m_EntryPackage.CharacterAttributeEntryList.Add(EntryLogic);
		}
	}
}

void ULxCharacterEntryComponent::AppendEquipmentEntries(const ULxCharacterEquipmentComponent& InEquipmentComponent, TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const
{
	for (ULxEquipmentSlotData* EquipmentSlot : InEquipmentComponent.GetEquipmentSlots())
	{
		if (EquipmentSlot == nullptr || !EquipmentSlot->IsValid())
		{
			continue;
		}

		ULxEquipmentLogic* EquipmentLogic = Cast<ULxEquipmentLogic>(EquipmentSlot->ItemDataPtr);
		if (EquipmentLogic == nullptr || !EquipmentLogic->ItemIsValid())
		{
			continue;
		}

		const FLxEquipmentData* EquipmentData = EquipmentLogic->GetEquipmentData();
		if (EquipmentData == nullptr)
		{
			continue;
		}

		AppendEntryIfValid(EquipmentData->EquipmentEntyInfo.EquipmentBasicEntry, OutEntryList);
		for (ULxItemEntryLogic* EntryLogic : EquipmentData->EquipmentEntyInfo.EquipmentExtendEntryList)
		{
			AppendEntryIfValid(EntryLogic, OutEntryList);
		}
	}
}

void ULxCharacterEntryComponent::AppendActiveBuffEntries(TArray<TObjectPtr<ULxItemEntryLogic>>& OutEntryList) const
{
	const ULxCharacterBuffComponent* BuffComponent = GetOwnerBuffComponent();
	if (BuffComponent == nullptr)
	{
		return;
	}

	TArray<ULxBuffLogic*> ActiveBuffs;
	BuffComponent->GetActiveBuffs(ActiveBuffs);

	for (const ULxBuffLogic* BuffLogic : ActiveBuffs)
	{
		if (BuffLogic == nullptr || !BuffLogic->IsBuffValid())
		{
			continue;
		}

		for (ULxItemEntryLogic* EntryLogic : BuffLogic->GetBuffData().BuffEntryList)
		{
			AppendEntryIfValid(EntryLogic, OutEntryList);
		}
	}
}

void ULxCharacterEntryComponent::ClearEquipmentGeneratedBuffs()
{
	if (m_vEquipmentGeneratedBuffs.IsEmpty())
	{
		return;
	}

	if (ULxCharacterBuffComponent* BuffComponent = GetOwnerBuffComponent())
	{
		for (ULxBuffLogic* BuffLogic : m_vEquipmentGeneratedBuffs)
		{
			if (BuffLogic != nullptr)
			{
				BuffComponent->RemoveBuff(BuffLogic);
			}
		}
	}

	m_vEquipmentGeneratedBuffs.Reset();
}

void ULxCharacterEntryComponent::ApplyEquipmentBuffEntries()
{
	ULxCharacterBuffComponent* BuffComponent = GetOwnerBuffComponent();
	if (BuffComponent == nullptr)
	{
		return;
	}

	TArray<ULxBuffLogic*> ExistingBuffs;
	BuffComponent->GetActiveBuffs(ExistingBuffs);

	for (ULxItemEntryLogic* EntryLogic : m_EntryPackage.BuffEntryList)
	{
		if (!IsValidEntry(EntryLogic))
		{
			continue;
		}

		const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
		if (EntryData.EnteryBaseInfo.EntryLogicType != ELxEntryLogicType::CreaterBuffer)
		{
			continue;
		}

		if (ULxBuffLogic* BuffLogic = BuffComponent->AddBuffByCreatorValue(EntryData.CreaterBufferValue, EntryData.EffectiveRatio))
		{
			if (!ExistingBuffs.Contains(BuffLogic))
			{
				m_vEquipmentGeneratedBuffs.AddUnique(BuffLogic);
				ExistingBuffs.Add(BuffLogic);
			}
		}
	}
}

void ULxCharacterEntryComponent::DispatchUsedItemEntries(const TArray<TObjectPtr<ULxItemEntryLogic>>& InEntryList)
{
	for (ULxItemEntryLogic* EntryLogic : InEntryList)
	{
		if (!IsValidEntry(EntryLogic))
		{
			continue;
		}

		const FLxItemEntryData& EntryData = EntryLogic->GetItemEntryData();
		if (EntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::CreaterBuffer)
		{
			if (ULxCharacterBuffComponent* BuffComponent = GetOwnerBuffComponent())
			{
				BuffComponent->AddBuffByCreatorValue(EntryData.CreaterBufferValue, EntryData.EffectiveRatio);
			}
			continue;
		}

		if (EntryData.EnteryBaseInfo.EntryLogicType == ELxEntryLogicType::ChangeAttributeValue
			&& !EntryData.EnteryBaseInfo.TargetTags.IsEmpty()
			&& EntryData.ChangeAttributeValue.EntryValueInfo.EntryTarget == ELxItemEntryTarget::ToValue)
		{
			const float EffectiveValue = EntryLogic->GetEffectiveValue();
			if (!FMath::IsNearlyZero(EffectiveValue))
			{
				OnAttributeRecoveryEntryApplied.Broadcast(EntryLogic);
			}
		}
	}
}

ULxCharacterBuffComponent* ULxCharacterEntryComponent::GetOwnerBuffComponent() const
{
	return m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterBuffComponent() : nullptr;
}
