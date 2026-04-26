#include "LxBuffWidget.h"

#include "LxARPG/LxSource/Model/Buff/DataType/LxBuffLogic.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxBuffWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
}

void ULxBuffWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);

	ULxCharacterBuffComponent* BuffComponent = m_pPlayerCharacter ? m_pPlayerCharacter->GetCharacterBuffComponent() : nullptr;
	BindBuffComponent(BuffComponent);
	RefreshBuffList();
}

void ULxBuffWidget::NativeDestruct()
{
	UnbindBuffComponent();
	Super::NativeDestruct();
}

void ULxBuffWidget::RefreshBuffList()
{
	if (m_pCharacterBuffComponent == nullptr)
	{
		m_vBuffList.Reset();
		m_vBuffSlotList.Reset();
		NotifyBuffListUpdated();
		return;
	}

	TArray<ULxBuffLogic*> BuffList;
	if (bOnlyShowDisplayBuffs)
	{
		m_pCharacterBuffComponent->GetDisplayBuffs(BuffList);
	}
	else
	{
		m_pCharacterBuffComponent->GetActiveBuffs(BuffList);
	}

	m_vBuffList.Reset();
	for (ULxBuffLogic* BuffLogic : BuffList)
	{
		if (BuffLogic != nullptr && BuffLogic->IsBuffValid())
		{
			m_vBuffList.Add(BuffLogic);
		}
	}

	RebuildBuffSlots();
	NotifyBuffListUpdated();
}

TArray<UObject*> ULxBuffWidget::GetBuffUIDataList()
{
	TArray<UObject*> BuffUIDataList;
	for (UObject* BuffUIData : m_vBuffUIDataList)
	{
		if (BuffUIData != nullptr)
		{
			BuffUIDataList.Add(BuffUIData);
		}
	}
	return BuffUIDataList;
}

TArray<ULxItemSlotData*> ULxBuffWidget::GetBuffSlotList() const
{
	TArray<ULxItemSlotData*> BuffSlotList;
	for (ULxItemSlotData* BuffSlot : m_vBuffSlotList)
	{
		if (BuffSlot != nullptr)
		{
			BuffSlotList.Add(BuffSlot);
		}
	}
	return BuffSlotList;
}

void ULxBuffWidget::BindBuffComponent(ULxCharacterBuffComponent* InBuffComponent)
{
	if (m_pCharacterBuffComponent == InBuffComponent)
	{
		return;
	}

	UnbindBuffComponent();
	m_pCharacterBuffComponent = InBuffComponent;

	if (m_pCharacterBuffComponent == nullptr)
	{
		return;
	}

	m_pCharacterBuffComponent->OnDataChange.RemoveDynamic(this, &ULxBuffWidget::HandleBuffComponentDataChanged);
	m_pCharacterBuffComponent->OnDataChange.AddDynamic(this, &ULxBuffWidget::HandleBuffComponentDataChanged);
}

void ULxBuffWidget::UnbindBuffComponent()
{
	if (m_pCharacterBuffComponent == nullptr)
	{
		return;
	}

	m_pCharacterBuffComponent->OnDataChange.RemoveDynamic(this, &ULxBuffWidget::HandleBuffComponentDataChanged);
	m_pCharacterBuffComponent = nullptr;
}

void ULxBuffWidget::RebuildBuffSlots()
{
	m_vBuffSlotList.Reset();

	for (int32 Index = 0; Index < m_vBuffList.Num(); ++Index)
	{
		ULxBuffLogic* BuffLogic = m_vBuffList[Index];
		if (BuffLogic == nullptr || !BuffLogic->IsBuffValid())
		{
			continue;
		}

		ULxItemSlotData* BuffSlot = NewObject<ULxItemSlotData>(this);
		BuffSlot->ItemSlotType = ELxItemSlotType::BuffDisplay;
		BuffSlot->ID = Index;
		BuffSlot->SetItem(BuffLogic);
		m_vBuffSlotList.Add(BuffSlot);
	}
}

TArray<UObject*> ULxBuffWidget::BuildBuffUIDataList()
{
	m_vBuffUIDataList.Reset();

	TArray<UObject*> BuffUIDataList;
	for (ULxItemSlotData* BuffSlot : m_vBuffSlotList)
	{
		if (BuffSlot == nullptr)
		{
			continue;
		}

		ULxItemUIData* BuffUIData = NewObject<ULxItemUIData>(this);
		BuffUIData->m_pSlotData = BuffSlot;
		m_vBuffUIDataList.Add(BuffUIData);
		BuffUIDataList.Add(BuffUIData);
	}

	return BuffUIDataList;
}

void ULxBuffWidget::NotifyBuffListUpdated()
{
	TArray<UObject*> BuffUIDataList = BuildBuffUIDataList();
	OnBuffUIDataListUpdated.Broadcast(BuffUIDataList);
	OnBuffListUpdated(BuffUIDataList);
}

void ULxBuffWidget::HandleBuffComponentDataChanged()
{
	RefreshBuffList();
}
