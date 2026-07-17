#include "LxCharacterStatusWidget.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"
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

void ULxCharacterStatusWidget::HandleCharacterAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	UpdateAttributeTextFromSnapshot(AttributeSnapshot);
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

	FLxTypedAttributeSnapshot AttributeSnapshot;
	m_pCharacterDataTransferComponent->GetAllCharacterAttributes(AttributeSnapshot);
	UpdateAttributeTextFromSnapshot(AttributeSnapshot);
}

void ULxCharacterStatusWidget::UpdateAttributeTextFromSnapshot(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	const FLxResourceAttributeData* HPAttribute = nullptr;
	const FLxResourceAttributeData* MPAttribute = nullptr;

	for (const FLxResourceAttributeData& AttributeData : AttributeSnapshot.ResourceAttributes)
	{
		if (AttributeData.AttributeIDTag == LxTag_Attribute_Resource_Health)
		{
			HPAttribute = &AttributeData;
		}
		else if (AttributeData.AttributeIDTag == LxTag_Attribute_Resource_Mana)
		{
			MPAttribute = &AttributeData;
		}
	}
	OnHUDAttributeValueUpdated(BuildProgressPercent(HPAttribute), BuildProgressPercent(MPAttribute));
	OnHUDAttributeTextUpdated(BuildResourceAttributeText(HPAttribute), BuildResourceAttributeText(MPAttribute));
}

FText ULxCharacterStatusWidget::BuildResourceAttributeText(const FLxResourceAttributeData* AttributeData)
{
	if (AttributeData == nullptr)
	{
		return FText::GetEmpty();
	}

	FLxString AttributeText;
	AttributeText << FLxString::DoubleToIntStr(AttributeData->Value)
		<< "/" << FLxString::DoubleToIntStr(AttributeData->ValueLimit);
	return AttributeText.ToFText();
}

float ULxCharacterStatusWidget::BuildProgressPercent(const FLxResourceAttributeData* AttributeData)
{
	if (AttributeData == nullptr || FMath::IsNearlyZero(AttributeData->ValueLimit))
	{
		return 0.0f;
	}

	return FMath::Clamp(AttributeData->Value / AttributeData->ValueLimit, 0.0f, 1.0f);
}
