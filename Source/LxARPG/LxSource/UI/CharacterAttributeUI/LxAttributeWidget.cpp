#include "LxAttributeWidget.h"

#include "Algo/Sort.h"
#include "LxARPG/LxSource/Core/Tools/LxAttributeValueTool.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
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

void ULxAttributeWidget::HandleCharacterAttributesChanged(const TArray<FLxAttributeData>& AttributeList)
{
	OnAttributeListUpdated(BuildAttributesUIDataList(AttributeList));
}

FText ULxAttributeWidget::GetAttributeValueStringByIDTag(FGameplayTag InAttributeIDTag) const
{
	if (!m_pCharacterDataTransferComponent)
	{
		return FLxString().ToFText();
	}

	FLxAttributeData AttributeData;
	if (!m_pCharacterDataTransferComponent->QueryCharacterAttributeByIDTag(InAttributeIDTag, AttributeData))
	{
		return FLxString().ToFText();
	}
	return LxAttributeTools::GetAttributeDisplayText(AttributeData);
}


bool ULxAttributeWidget::GetAttributeDataByIDTag(FGameplayTag InAttributeIDTag, FLxAttributeData& OutAttributeData)
{
	if (!m_pCharacterDataTransferComponent)
	{
		return false;
	}

	return m_pCharacterDataTransferComponent->QueryCharacterAttributeByIDTag(InAttributeIDTag, OutAttributeData);
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

	TArray<FLxAttributeData> AttributeList;
	m_pCharacterDataTransferComponent->GetAllCharacterAttributes(AttributeList);
	HandleCharacterAttributesChanged(AttributeList);
}

TArray<ULxUITextData*> ULxAttributeWidget::BuildAttributesUIDataList(const TArray<FLxAttributeData>& AttributeList)
{
	TArray<ULxUITextData*> UIDataList;

	TArray<const FLxAttributeData*> VisibleAttributeList;
	VisibleAttributeList.Reserve(AttributeList.Num());
	for (const FLxAttributeData& AttributeData : AttributeList)
	{
		if (!AttributeData.ShowInfo.IsVisible)
		{
			continue;
		}

		VisibleAttributeList.Add(&AttributeData);
	}

	Algo::Sort(VisibleAttributeList, [](const FLxAttributeData* Left, const FLxAttributeData* Right)
	{
		return Left->AttributeIDTag.ToString() < Right->AttributeIDTag.ToString();
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

		// 属性列表显示文本统一由属性工具生成，UI 只负责接收文本数据并显示。
		AttributeUIData->DisplayText = LxAttributeTools::GetAttributeDisplayText(*AttributeData);
		AttributeUIData->IsDarkColor = IsDarkColor;
		UIDataList.Add(AttributeUIData);
		IsDarkColor = !IsDarkColor;
	}

	return UIDataList;
}
