#include "LxCharacterHUDWidget.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemUIData.h"

void ULxCharacterHUDWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	RefreshAttributeTextFromDataTransfer();
}

void ULxCharacterHUDWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

bool ULxCharacterHUDWidget::BindShortcutItemGridInput(ULxItemGridWidget* InItemGridWidget, ELxInputActionID InInputActionID)
{
	if (!InItemGridWidget || InInputActionID == ELxInputActionID::None)
	{
		return false;
	}

	ULxItemSlotData* ShortcutSlot = NewObject<ULxItemSlotData>(InItemGridWidget);
	ShortcutSlot->InitItemSlot(ELxItemSlotType::Shortcut);

	ULxItemUIData* ItemUIData = NewObject<ULxItemUIData>(InItemGridWidget);
	ItemUIData->m_pSlotData = ShortcutSlot;
	
	InItemGridWidget->NativeOnListItemObjectSet(ItemUIData);
	InItemGridWidget->RegisterInputActionReceive(InInputActionID);

	return true;
}

void ULxCharacterHUDWidget::HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList)
{
	UpdateAttributeTextFromList(AttributeList);
}


void ULxCharacterHUDWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
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

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.AddDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
}

void ULxCharacterHUDWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterHUDWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxCharacterHUDWidget::RefreshAttributeTextFromDataTransfer()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		OnHUDAttributeValueUpdated(0.0f, 0.0f);
		OnHUDAttributeTextUpdated(FText::GetEmpty(), FText::GetEmpty());
		return;
	}

	TArray<FLxAttributeData> AttributeList;
	m_pCharacterDataTransferComponent->GetAllCharacterAttributes(AttributeList);
	UpdateAttributeTextFromList(AttributeList);
}

void ULxCharacterHUDWidget::UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList)
{
	const FLxAttributeData* HPAttribute = nullptr;
	const FLxAttributeData* MPAttribute = nullptr;

	for (const FLxAttributeData& AttributeData : AttributeList)
	{
		if (AttributeData.AttributeID == ELxCharacterAttributeID::C_HP)
		{
			HPAttribute = &AttributeData;
		}
		else if (AttributeData.AttributeID == ELxCharacterAttributeID::C_MP)
		{
			MPAttribute = &AttributeData;
		}
	}
	OnHUDAttributeValueUpdated(BuildProgressPercent(HPAttribute), BuildProgressPercent(MPAttribute));
	OnHUDAttributeTextUpdated(BuildRangedAttributeText(HPAttribute), BuildRangedAttributeText(MPAttribute));
}

FText ULxCharacterHUDWidget::BuildRangedAttributeText(const FLxAttributeData* AttributeData)
{
	if (AttributeData == nullptr)
	{
		return FText::GetEmpty();
	}

	FLxString AttributeText;
	AttributeText << FLxString::DoubleToIntStr(AttributeData->CalculatedAttributeValue.Value)
		<< "/" << FLxString::DoubleToIntStr(AttributeData->CalculatedAttributeValue.ValueLimit);
	return AttributeText.ToFText();
}

float ULxCharacterHUDWidget::BuildProgressPercent(const FLxAttributeData* AttributeData)
{
	if (AttributeData == nullptr || FMath::IsNearlyZero(AttributeData->CalculatedAttributeValue.ValueLimit))
	{
		return 0.0f;
	}

	return FMath::Clamp(AttributeData->CalculatedAttributeValue.Value / AttributeData->CalculatedAttributeValue.ValueLimit, 0.0f, 1.0f);
}
