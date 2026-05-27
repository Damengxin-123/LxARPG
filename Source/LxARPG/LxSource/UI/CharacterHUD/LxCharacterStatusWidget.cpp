#include "LxCharacterStatusWidget.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"

void ULxCharacterStatusWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	RefreshAttributeTextFromDataTransfer();
}

void ULxCharacterStatusWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxCharacterStatusWidget::HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList)
{
	UpdateAttributeTextFromList(AttributeList);
}

void ULxCharacterStatusWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
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

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterStatusWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.AddDynamic(this, &ULxCharacterStatusWidget::HandleCharacterAttributesChanged);
}

void ULxCharacterStatusWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxCharacterStatusWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxCharacterStatusWidget::RefreshAttributeTextFromDataTransfer()
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

void ULxCharacterStatusWidget::UpdateAttributeTextFromList(const TArray<FLxAttributeData>& AttributeList)
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

FText ULxCharacterStatusWidget::BuildRangedAttributeText(const FLxAttributeData* AttributeData)
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

float ULxCharacterStatusWidget::BuildProgressPercent(const FLxAttributeData* AttributeData)
{
	if (AttributeData == nullptr || FMath::IsNearlyZero(AttributeData->CalculatedAttributeValue.ValueLimit))
	{
		return 0.0f;
	}

	return FMath::Clamp(AttributeData->CalculatedAttributeValue.Value / AttributeData->CalculatedAttributeValue.ValueLimit, 0.0f, 1.0f);
}
