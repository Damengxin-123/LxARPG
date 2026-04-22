#include "LxCharacterEntryComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumable.h"
#include "LxARPG/LxSource/Model/Item/DataType/Consumable/LxConsumableLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ItemBase/LxItemLogicBase.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

namespace
{
	/** 统一过滤掉未正确解析出的空词条。 */
	bool IsValidEntry(const FLxItemEntryData& InEntryData)
	{
		return !InEntryData.EntryID.IsNone();
	}

	void AppendEntryIfValid(const FLxItemEntryData& InEntryData, TArray<FLxItemEntryData>& OutEntryList)
	{
		if (IsValidEntry(InEntryData))
		{
			OutEntryList.Add(InEntryData);
		}
	}

	void AppendConsumableEntries(const ULxItemLogicBase* InUsedItem, TArray<FLxItemEntryData>& OutEntryList)
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

		for (const FLxItemEntryData& EntryData : ConsumableData->ConsumableEntryInfo.ConsumableEntryList)
		{
			AppendEntryIfValid(EntryData, OutEntryList);
		}
	}

	void AppendEntryByLogicType(const FLxItemEntryData& InEntryData, FLxCharacterEntryPackage& OutEntryPackage)
	{
		// 词条组件只负责分类，不在这里执行具体效果。
		switch (InEntryData.ItemEntryLogicType)
		{
		case ELxItemEntryLogicType::None:
		case ELxItemEntryLogicType::AttributeModifier:
			OutEntryPackage.CharacterAttributeEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::AttributeModEquipment:
			OutEntryPackage.EquipmentAttributeEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::ApplyBuff:
			OutEntryPackage.BuffEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::InstantRestore:
			OutEntryPackage.InstantRestoreEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::ApplyStatus:
			OutEntryPackage.StatusEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::ExploreUtility:
			OutEntryPackage.UtilityEntryList.Add(InEntryData);
			break;
		case ELxItemEntryLogicType::ThrowConsumable:
			OutEntryPackage.ThrowConsumableEntryList.Add(InEntryData);
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
		// 装备组件只负责“装备变了”，词条组件在这里接手后续解释。
		EquipmentComponent->OnDataChange.RemoveDynamic(this, &ULxCharacterEntryComponent::HandleEquipmentDataChange);
		EquipmentComponent->OnDataChange.AddDynamic(this, &ULxCharacterEntryComponent::HandleEquipmentDataChange);
	}

	if (ULxCharacterBackpackComponent* BackpackComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterBackpackComponent() : nullptr)
	{
		// 背包组件只上报“某个物品被使用了”，词条效果解析统一收口到这里。
		BackpackComponent->OnItemUsed.RemoveDynamic(this, &ULxCharacterEntryComponent::HandleBackpackItemUsed);
		BackpackComponent->OnItemUsed.AddDynamic(this, &ULxCharacterEntryComponent::HandleBackpackItemUsed);
	}

	m_bEntryInitialized = true;
	RefreshInstalledEntries();
}

void ULxCharacterEntryComponent::GetInstalledEntries(TArray<FLxItemEntryData>& OutEntryList) const
{
	OutEntryList = m_vInstalledEntries;
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

	TArray<FLxItemEntryData> UsedEntryList;
	AppendConsumableEntries(UsedItem, UsedEntryList);
	DispatchUsedItemEntries(UsedEntryList);
}

void ULxCharacterEntryComponent::RefreshInstalledEntries()
{
	m_vInstalledEntries.Reset();
	m_EntryPackage = FLxCharacterEntryPackage();

	if (const ULxCharacterEquipmentComponent* EquipmentComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterEquipmentComponent() : nullptr)
	{
		AppendEquipmentEntries(*EquipmentComponent, m_vInstalledEntries);
	}

	BuildEntryPackage();

	// 保留通用数据变化事件，同时额外广播带分类结果的专用事件。
	OnDataChange.Broadcast();
	OnEntryPackageChanged.Broadcast(m_EntryPackage);
}

void ULxCharacterEntryComponent::BuildEntryPackage()
{
	m_EntryPackage = FLxCharacterEntryPackage();
	m_EntryPackage.InstalledEntryList = m_vInstalledEntries;

	// 这里先做静态分类，后续 Buff/技能/天赋来源接入时可以继续复用这一步。
	for (const FLxItemEntryData& EntryData : m_vInstalledEntries)
	{
		AppendEntryByLogicType(EntryData, m_EntryPackage);
	}
}

void ULxCharacterEntryComponent::AppendEquipmentEntries(const ULxCharacterEquipmentComponent& InEquipmentComponent, TArray<FLxItemEntryData>& OutEntryList) const
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

		// 词条组件直接读取装备运行时数据中的基础词条与扩展词条。
		AppendEntryIfValid(EquipmentData->EquipmentEntyInfo.EquipmentBasicEntry, OutEntryList);
		for (const FLxItemEntryData& EntryData : EquipmentData->EquipmentEntyInfo.EquipmentExtendEntryList)
		{
			AppendEntryIfValid(EntryData, OutEntryList);
		}
	}
}

void ULxCharacterEntryComponent::DispatchUsedItemEntries(const TArray<FLxItemEntryData>& InEntryList)
{
	for (const FLxItemEntryData& EntryData : InEntryList)
	{
		if (EntryData.ItemEntryLogicType != ELxItemEntryLogicType::InstantRestore)
		{
			continue;
		}

		if (EntryData.AttributeID == ELxCharacterAttributeID::X_None)
		{
			continue;
		}

		if (EntryData.ItemEntryDefineValue.EntryTarget != ELxItemEntryTarget::ToValue)
		{
			continue;
		}

		const float EffectiveValue = EntryData.ItemEntryDefineValue.Value * EntryData.EffectiveRatio;
		if (FMath::IsNearlyZero(EffectiveValue))
		{
			continue;
		}

		// 即时恢复先走独立事件派发，避免和常驻已安装词条混在同一条刷新链上。
		OnInstantRestoreEntryApplied.Broadcast(EntryData.AttributeID, EffectiveValue);
	}
}
