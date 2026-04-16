#include "LxAttributeWidget.h"

#include "Algo/Sort.h"
#include "LxARPG/LxSource/Core/Tools/LxAttributeValueTool.h"
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

TArray<ULxUITextData*> ULxAttributeWidget::GetAttributesUIDataList()
{
	TArray<ULxUITextData*> UIDataList;

	if (!m_pCharacterAttributeComponent)
	{
		return UIDataList;
	}

	const TMap<FName, FLxAttributeData>* CharacterAttributeTable = m_pCharacterAttributeComponent->GetCharacterAttributeTable();
	if (!CharacterAttributeTable)
	{
		return UIDataList;
	}

	TArray<const FLxAttributeData*> VisibleAttributeList;
	VisibleAttributeList.Reserve(CharacterAttributeTable->Num());
	for (const TPair<FName, FLxAttributeData>& AttributePair : *CharacterAttributeTable)
	{
		const FLxAttributeData& AttributeData = AttributePair.Value;
		// 只生成需要在属性面板中展示的条目。
		if (!AttributeData.AttributeShowInfo.IsVisible)
		{
			continue;
		}

		VisibleAttributeList.Add(&AttributeData);
	}

	// 按属性类型枚举值排序，保证属性列表在 UI 中稳定、有序地显示。
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

		// 文本主体使用属性配置中的富文本描述，数值部分单独填充给列表项控件。
		AttributeUIData->RichTextDescriptionGroupData = const_cast<FLxRichTextDescriptionGroupData*>(&AttributeData->AttributeShowInfo.AttributeName);
		AttributeUIData->ValueText = FLxAttributeValueTool::BuildAttributeValueText(*AttributeData);
		AttributeUIData->IsDarkColor = IsDarkColor;
		UIDataList.Add(AttributeUIData);
		IsDarkColor = !IsDarkColor;
		// 交替设置。
	}

	return UIDataList;
}
