#include "LxAttributeWidget.h"

#include "Algo/Sort.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

void ULxAttributeWidget::InitializeUIComponents()
{
	Super::InitializeUIComponents();
}

void ULxAttributeWidget::UpdateUIComponents(ALxBaseCharacter* PlayerCharacter)
{
	Super::UpdateUIComponents(PlayerCharacter);
	if (PlayerCharacter)
	{
		m_pCharacterAttributeComponent = PlayerCharacter->FindComponentByClass<ULxCharacterAttributeComponent>();
		if (m_pCharacterAttributeComponent)
		{
			m_pCharacterAttributeComponent->OnDataChange.AddDynamic(this, &ULxAttributeWidget::HandleAttributeChanged);
			HandleAttributeChanged();
		}
	}
}

void ULxAttributeWidget::HandleAttributeChanged()
{
	OnAttributeUpdate.Broadcast();
}

bool ULxAttributeWidget::GetAttributeDataByID(ELxCharacterAttributeID InAttributeID, FLxAttributeData& OutAttributeData)
{
	if (!m_pCharacterAttributeComponent)
	{
		return false;
	}

	const FLxAttributeData* AttributeData = m_pCharacterAttributeComponent->GetCharacterAttributeByID(InAttributeID);
	if (!AttributeData)
	{
		return false;
	}

	OutAttributeData = *AttributeData;
	return true;
}

TArray<ULxUITextData*> ULxAttributeWidget::GetAttributesUIDataList()
{
	TArray<ULxUITextData*> UIDataList;

	if (!m_pCharacterAttributeComponent)
	{
		return UIDataList;
	}

	const TMap<ELxCharacterAttributeID, FLxAttributeData>* CharacterAttributeTable = m_pCharacterAttributeComponent->GetCharacterAttributeTable();
	if (!CharacterAttributeTable)
	{
		return UIDataList;
	}

	TArray<const FLxAttributeData*> VisibleAttributeList;
	VisibleAttributeList.Reserve(CharacterAttributeTable->Num());
	for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : *CharacterAttributeTable)
	{
		const FLxAttributeData& AttributeData = AttributePair.Value;
		if (!AttributeData.AttributeShowInfo.IsVisible)
		{
			continue;
		}

		VisibleAttributeList.Add(&AttributeData);
	}

	Algo::Sort(VisibleAttributeList, [](const FLxAttributeData* Left, const FLxAttributeData* Right)
	{
		return static_cast<uint8>(Left->AttributeInfo.AttributeType) < static_cast<uint8>(Right->AttributeInfo.AttributeType);
	});

	UIDataList.Reserve(VisibleAttributeList.Num());
	bool IsDarkColor = true;
	for (const FLxAttributeData* AttributeData : VisibleAttributeList)
	{
		ULxUITextData* AttributeUIData = NewObject<ULxUITextData>(this);
		if (!AttributeUIData)
		{
			continue;
		}

		AttributeUIData->DisplayText = ULxCharacterAttributeComponent::BuildAttributeDisplayText(*AttributeData);
		AttributeUIData->IsDarkColor = IsDarkColor;
		UIDataList.Add(AttributeUIData);
		IsDarkColor = !IsDarkColor;
	}

	return UIDataList;
}
