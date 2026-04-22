#include "LxCharacterEquipmentComponent.h"

#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipmentLogic.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEquipmentComponent::ULxCharacterEquipmentComponent()
{
}

void ULxCharacterEquipmentComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();

	// 缓存所属角色，便于后续访问角色相关数据。
	if (m_pOwnerCharacter == nullptr)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	// 初始化装备槽，并同步当前已装备缓存。
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

inline void ULxCharacterEquipmentComponent::InitializeEquipmentSlots()
{
	// 槽位已经创建过时不再重复初始化。
	if (!m_vEquipmentSlots.IsEmpty())
	{
		return;
	}

	// 如果没有配置槽位，则自动生成一套默认装备槽布局。
	if (EquipmentSlotsConfig.IsEmpty())
	{
		SetDefauitEquipmentSlotsConfig();
	}

	// 逐个创建运行时装备槽，并绑定槽位变化事件。
	for (int i = 0; i < EquipmentSlotsConfig.Num(); ++i)
	{
		TObjectPtr<ULxEquipmentSlotData> NewSlot = NewObject<ULxEquipmentSlotData>(this);
		NewSlot->ItemSlotType = ELxItemSlotType::Equipment;
		NewSlot->EquipmentType = EquipmentSlotsConfig[i];
		NewSlot->ID = i;
		NewSlot->OnSlotChanged.AddDynamic(this, &ULxCharacterEquipmentComponent::HandleEquipmentSlotChanged);
		m_vEquipmentSlots.Add(NewSlot);
	}
}

void ULxCharacterEquipmentComponent::SetDefauitEquipmentSlotsConfig()
{
	// 按装备类型枚举顺序填充默认槽位配置。
	EquipmentSlotsConfig.Empty();
	for (int i = 0; i < static_cast<int32>(ELxEquipmentType::EquipmentSizeMax); ++i)
	{
		EquipmentSlotsConfig.Add(static_cast<ELxEquipmentType>(i));
	}
}

void ULxCharacterEquipmentComponent::BroadcastEquipmentChanged()
{
	OnDataChange.Broadcast();
}

void ULxCharacterEquipmentComponent::HandleEquipmentSlotChanged()
{
	// 任意装备槽变化后，都重新构建已装备缓存。
	RefreshEquipmentList();
}

void ULxCharacterEquipmentComponent::RefreshEquipmentList()
{
	// 先清空旧缓存，再从装备槽重新构建。
	m_vEquipmentList.Empty();

	// 只把有效的装备逻辑对象同步到缓存中。
	for (ULxEquipmentSlotData* EquipmentSlot : m_vEquipmentSlots)
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

		m_vEquipmentList.Add(EquipmentLogic);
	}

	// 缓存更新完成后，再向外广播数据变化。
	BroadcastEquipmentChanged();
}
