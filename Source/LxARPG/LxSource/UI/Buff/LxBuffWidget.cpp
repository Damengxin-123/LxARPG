#include "LxBuffWidget.h"

#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxBuffWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	RefreshBuffList();
}

void ULxBuffWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxBuffWidget::RefreshBuffList()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		m_vBuffList.Reset();
		m_vBuffSlotList.Reset();
		NotifyBuffListUpdated();
		return;
	}

	TArray<ULxBuff*> BuffList;
	if (bOnlyShowDisplayBuffs)
	{
		m_pCharacterDataTransferComponent->GetDisplayBuffs(BuffList);
	}
	else
	{
		m_pCharacterDataTransferComponent->GetAllBuffs(BuffList);
	}

	m_vBuffList.Reset();
	for (ULxBuff* BuffLogic : BuffList)
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

void ULxBuffWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
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

	m_pCharacterDataTransferComponent->OnBuffChanged.RemoveDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
	m_pCharacterDataTransferComponent->OnBuffChanged.AddDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
}

void ULxBuffWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnBuffChanged.RemoveDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxBuffWidget::RebuildBuffSlots()
{
	m_vBuffSlotList.Reset();

	for (int32 Index = 0; Index < m_vBuffList.Num(); ++Index)
	{
		ULxBuff* BuffLogic = m_vBuffList[Index];
		if (BuffLogic == nullptr || !BuffLogic->IsBuffValid())
		{
			continue;
		}

		ULxItemSlotData* BuffSlot = NewObject<ULxItemSlotData>(this);
		BuffSlot->InitItemSlot(ELxItemSlotType::BuffDisplay, LxTag_Item_Buff, BuffLogic);
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

void ULxBuffWidget::HandleDataTransferBuffChanged(const TArray<ULxBuff*>& BuffList)
{
	if (bOnlyShowDisplayBuffs)
	{
		RefreshBuffList();
		return;
	}

	m_vBuffList.Reset();
	for (ULxBuff* BuffLogic : BuffList)
	{
		if (BuffLogic != nullptr && BuffLogic->IsBuffValid())
		{
			m_vBuffList.Add(BuffLogic);
		}
	}

	RebuildBuffSlots();
	NotifyBuffListUpdated();
}
