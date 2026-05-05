#include "LxBuffWidget.h"

#include "LxARPG/LxSource/Model/Buff/DataType/LxBuff.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
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

	ULxCharacterDataTransferComponent* DataTransferComponent = m_pPlayerCharacter ? m_pPlayerCharacter->GetCharacterDataTransferComponent() : nullptr;
	BindDataTransferComponent(DataTransferComponent);
	RefreshBuffList();
}

void ULxBuffWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxBuffWidget::RefreshBuffList()
{
	if (CharacterDataTransferComponent == nullptr)
	{
		m_vBuffList.Reset();
		m_vBuffSlotList.Reset();
		NotifyBuffListUpdated();
		return;
	}

	TArray<ULxBuff*> BuffList;
	if (bOnlyShowDisplayBuffs)
	{
		CharacterDataTransferComponent->GetDisplayBuffs(BuffList);
	}
	else
	{
		CharacterDataTransferComponent->GetAllBuffs(BuffList);
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
	if (CharacterDataTransferComponent == InDataTransferComponent)
	{
		return;
	}

	UnbindDataTransferComponent();
	CharacterDataTransferComponent = InDataTransferComponent;

	if (CharacterDataTransferComponent == nullptr)
	{
		return;
	}

	CharacterDataTransferComponent->OnBuffChanged.RemoveDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
	CharacterDataTransferComponent->OnBuffChanged.AddDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
}

void ULxBuffWidget::UnbindDataTransferComponent()
{
	if (CharacterDataTransferComponent == nullptr)
	{
		return;
	}

	CharacterDataTransferComponent->OnBuffChanged.RemoveDynamic(this, &ULxBuffWidget::HandleDataTransferBuffChanged);
	CharacterDataTransferComponent = nullptr;
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
