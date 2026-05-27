#include "LxSkillBackpackWidget.h"

#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxSkillBackpackWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	UpdateSkillBackpack();
}

void ULxSkillBackpackWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

TArray<UObject*> ULxSkillBackpackWidget::GetSkillUIDataList()
{
	TArray<UObject*> SkillUIDataList;
	for (ULxItemSlotData* SkillSlot : SkillSlotList)
	{
		ULxItemUIData* SkillUIData = NewObject<ULxItemUIData>(this);
		SkillUIData->m_pSlotData = SkillSlot;
		SkillUIDataList.Add(SkillUIData);
	}
	return SkillUIDataList;
}

void ULxSkillBackpackWidget::SwitchSkillTag(FGameplayTag NewSkillTag)
{
	CurrentSkillTagFilter = NewSkillTag;
	UpdateSkillBackpack();
}

void ULxSkillBackpackWidget::UpdateSkillBackpack()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		SkillSlotList.Reset();
		OnSkillListUpdated(GetSkillUIDataList());
		return;
	}

	TArray<ULxItemSlotData*> SkillSlots;
	if (CurrentSkillTagFilter.IsValid())
	{
		m_pCharacterDataTransferComponent->QuerySkillBackpackSlotsByTag(CurrentSkillTagFilter, SkillSlots);
	}
	else
	{
		m_pCharacterDataTransferComponent->GetAllSkillBackpackSlots(SkillSlots);
	}
	HandleSkillSlotsChanged(SkillSlots);
}

void ULxSkillBackpackWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
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

	m_pCharacterDataTransferComponent->OnSkillBackpackChanged.RemoveDynamic(this, &ULxSkillBackpackWidget::HandleSkillSlotsChanged);
	m_pCharacterDataTransferComponent->OnSkillBackpackChanged.AddDynamic(this, &ULxSkillBackpackWidget::HandleSkillSlotsChanged);
}

void ULxSkillBackpackWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnSkillBackpackChanged.RemoveDynamic(this, &ULxSkillBackpackWidget::HandleSkillSlotsChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxSkillBackpackWidget::HandleSkillSlotsChanged(const TArray<ULxItemSlotData*>& SkillSlots)
{
	SkillSlotList.Reset();
	if (CurrentSkillTagFilter.IsValid() && m_pCharacterDataTransferComponent)
	{
		TArray<ULxItemSlotData*> FilteredSlots;
		m_pCharacterDataTransferComponent->QuerySkillBackpackSlotsByTag(CurrentSkillTagFilter, FilteredSlots);
		for (ULxItemSlotData* SlotData : FilteredSlots)
		{
			SkillSlotList.Add(SlotData);
		}
	}
	else
	{
		for (ULxItemSlotData* SlotData : SkillSlots)
		{
			SkillSlotList.Add(SlotData);
		}
	}
	OnSkillListUpdated(GetSkillUIDataList());
}
