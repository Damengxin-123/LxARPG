#include "LxAttributeWidget.h"

#include "Algo/Sort.h"
#include "LxARPG/LxSource/Core/Tools/LxAttributeValueTool.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

void ULxAttributeWidget::UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent)
{
	BindDataTransferComponent(CharacterDataTransferComponent);
	Super::UpdateUIComponents(CharacterDataTransferComponent);
	RefreshAttributeListFromDataTransfer();
}

void ULxAttributeWidget::NativeDestruct()
{
	UnbindDataTransferComponent();
	Super::NativeDestruct();
}

void ULxAttributeWidget::HandleCharacterAttributesChanged(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	OnAttributeListUpdated(BuildAttributesUIDataList(AttributeSnapshot));
}

FText ULxAttributeWidget::GetAttributeValueStringByIDTag(FGameplayTag InAttributeIDTag) const
{
	if (!m_pCharacterDataTransferComponent)
	{
		return FText::GetEmpty();
	}

	FLxAttributeDisplayData AttributeData;
	if (!GetAttributeDisplayDataByIDTag(InAttributeIDTag, AttributeData))
	{
		return FText::GetEmpty();
	}
	return AttributeData.ValueText;
}

bool ULxAttributeWidget::GetAttributeDisplayDataByIDTag(const FGameplayTag InAttributeIDTag, FLxAttributeDisplayData& OutAttributeData) const
{
	if (!m_pCharacterDataTransferComponent)
	{
		return false;
	}

	FLxTypedAttributeSnapshot AttributeSnapshot;
	m_pCharacterDataTransferComponent->GetAllCharacterAttributes(AttributeSnapshot);
	return FLxAttributeValueTool::FindDisplayDataByIDTag(AttributeSnapshot, InAttributeIDTag, OutAttributeData);
}

void ULxAttributeWidget::BindDataTransferComponent(ULxCharacterDataTransferComponent* InDataTransferComponent)
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

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxAttributeWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.AddDynamic(this, &ULxAttributeWidget::HandleCharacterAttributesChanged);
}

void ULxAttributeWidget::UnbindDataTransferComponent()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		return;
	}

	m_pCharacterDataTransferComponent->OnCharacterAttributeChanged.RemoveDynamic(this, &ULxAttributeWidget::HandleCharacterAttributesChanged);
	m_pCharacterDataTransferComponent = nullptr;
}

void ULxAttributeWidget::RefreshAttributeListFromDataTransfer()
{
	if (m_pCharacterDataTransferComponent == nullptr)
	{
		OnAttributeListUpdated(TArray<ULxUITextData*>());
		return;
	}

	FLxTypedAttributeSnapshot AttributeSnapshot;
	m_pCharacterDataTransferComponent->GetAllCharacterAttributes(AttributeSnapshot);
	HandleCharacterAttributesChanged(AttributeSnapshot);
}

TArray<ULxUITextData*> ULxAttributeWidget::BuildAttributesUIDataList(const FLxTypedAttributeSnapshot& AttributeSnapshot)
{
	TArray<ULxUITextData*> UIDataList;
	TArray<FLxAttributeDisplayData> DisplayDataList;
	FLxAttributeValueTool::BuildDisplayDataList(AttributeSnapshot, DisplayDataList);
	DisplayDataList.RemoveAll([](const FLxAttributeDisplayData& AttributeData)
	{
		return !AttributeData.ShowInfo.IsVisible;
	});

	Algo::Sort(DisplayDataList, [](const FLxAttributeDisplayData& Left, const FLxAttributeDisplayData& Right)
	{
		return Left.AttributeIDTag.ToString() < Right.AttributeIDTag.ToString();
	});

	UIDataList.Reserve(DisplayDataList.Num());
	bool IsDarkColor = true;
	for (const FLxAttributeDisplayData& AttributeData : DisplayDataList)
	{
		ULxUITextData* AttributeUIData = NewObject<ULxUITextData>(this);
		if (!AttributeUIData)
		{
			continue;
		}

		AttributeUIData->DisplayText = AttributeData.DisplayText;
		AttributeUIData->IsDarkColor = IsDarkColor;
		UIDataList.Add(AttributeUIData);
		IsDarkColor = !IsDarkColor;
	}

	return UIDataList;
}
