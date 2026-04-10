#include "LxAttrText.h"

#include "LxARPG/LxSource/Core/Database/LxConstValue.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Entry/LxItemEntryData.h"
#include "LxARPG/LxSource/UI/TextUI/LxUITextData.h"

namespace
{
	constexpr FLinearColor DarkColor(0.f, 0.f, 0.f, 0.6f);
	constexpr FLinearColor LightColor(0.f, 0.f, 0.f, 0.2f);
	const FName TitleStyleName(TEXT("Title"));
	const FName ChatStyleName(TEXT("Chat"));

	int32 ResolveDisplayValue(const FLxItemEntryValueInfo& InValueSet)
	{
		if (InValueSet.ValueLimit != ERR_ATTRIBUTE)
		{
			return InValueSet.ValueLimit;
		}

		if (InValueSet.Value != ERR_ATTRIBUTE)
		{
			return InValueSet.Value;
		}

		return 0;
	}
}

void ULxAttrText::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	m_pUIData = Cast<ULxUITextData>(ListItemObject);
	UpdateShowData();
}

void ULxAttrText::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	OnAttrTextHoverChanged.Broadcast(m_pUIData, true);
	ReceiveAttrTextHoverChanged(m_pUIData, true);
}

void ULxAttrText::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnAttrTextHoverChanged.Broadcast(m_pUIData, false);
	ReceiveAttrTextHoverChanged(m_pUIData, false);
}

void ULxAttrText::UpdateShowData()
{
	if (!m_pUIData)
	{
		return;
	}

	FText DisplayText = FText::GetEmpty();
	FName StyleName = NAME_None;
	bool bShowIcon = true;

	const bool bHasAttributeDisplay = BuildAttributeDisplay(DisplayText, StyleName);
	const bool bHasEntryDisplay = !bHasAttributeDisplay && BuildEntryDisplay(DisplayText, StyleName);
	const bool bHasPlainDisplay = !bHasAttributeDisplay && !bHasEntryDisplay && BuildPlainTextDisplay(DisplayText, StyleName, bShowIcon);

	if (!bHasAttributeDisplay && !bHasEntryDisplay && !bHasPlainDisplay)
	{
		return;
	}

	const FLinearColor BackgroundColor = GetBackgroundColor();
	OnAttrTextDataChanged.Broadcast(m_pUIData, DisplayText, StyleName, bShowIcon, BackgroundColor);
	ReceiveAttrTextDataChanged(m_pUIData, DisplayText, StyleName, bShowIcon, BackgroundColor);
}

FLinearColor ULxAttrText::GetBackgroundColor() const
{
	return (m_pUIData && m_pUIData->m_bIsDarkColor) ? DarkColor : LightColor;
}

bool ULxAttrText::BuildAttributeDisplay(FText& OutDisplayText, FName& OutStyleName) const
{
	if (!m_pUIData || !m_pUIData->m_pCharacterAttributeDataPtr)
	{
		return false;
	}

	const FLxAttributeData* AttributeData = m_pUIData->m_pCharacterAttributeDataPtr;

	FString ValueString;
	switch (AttributeData->CalculatedAttributeValue.ValueType)
	{
	case ELxCharacterValueType::FixedNumeric:
	case ELxCharacterValueType::FloatNumeric:
		ValueString = FString::FromInt(AttributeData->CalculatedAttributeValue.Value);
		break;
	case ELxCharacterValueType::FixedPercentage:
	case ELxCharacterValueType::Probabilistic:
		ValueString = FString::Printf(TEXT("%d%%"), AttributeData->CalculatedAttributeValue.Value);
		break;
	case ELxCharacterValueType::Mechanism:
		ValueString = AttributeData->CalculatedAttributeValue.Value != 0 ? TEXT("True") : TEXT("False");
		break;
	default:
		ValueString = FString::FromInt(AttributeData->CalculatedAttributeValue.Value);
		break;
	}

	OutDisplayText = FText::FromString(FString::Printf(TEXT("%s : %s"), *AttributeData->AttributeShowInfo.AttributeName.ToString(), *ValueString));
	OutStyleName = (AttributeData->AttributeTextStyle ? AttributeData->AttributeTextStyle->ID : NAME_None);
	return true;
}

bool ULxAttrText::BuildEntryDisplay(FText& OutDisplayText, FName& OutStyleName) const
{
	if (!m_pUIData || !m_pUIData->m_pItemEntryDataPtr)
	{
		return false;
	}

	const FLxItemEntryData* EntryData = m_pUIData->m_pItemEntryDataPtr;
	const int32 DisplayValue = ResolveDisplayValue(EntryData->ItemEntryDefineValue);

	FString ValueString;
	switch (EntryData->ItemEntryDefineValue.EntryType)
	{
	case ELxItemEntryType::BasicImprove:
	case ELxItemEntryType::AdditionalImprove:
		ValueString = FString::Printf(TEXT("%d%%"), DisplayValue);
		break;
	case ELxItemEntryType::BasicValue:
		ValueString = FString::FromInt(DisplayValue);
		break;
	case ELxItemEntryType::Mechanism:
		ValueString = DisplayValue != 0 ? TEXT("True") : TEXT("False");
		break;
	default:
		ValueString = FString::FromInt(DisplayValue);
		break;
	}

	OutDisplayText = FText::FromString(FString::Printf(TEXT("%s : %s"), *EntryData->DisplayName.ToString(), *ValueString));
	OutStyleName = (EntryData->TextStyle ? EntryData->TextStyle->ID : NAME_None);
	return true;
}

bool ULxAttrText::BuildPlainTextDisplay(FText& OutDisplayText, FName& OutStyleName, bool& bOutShowIcon) const
{
	if (!m_pUIData)
	{
		return false;
	}

	if (!m_pUIData->m_Title.IsEmpty())
	{
		OutDisplayText = FText::FromString(m_pUIData->m_Title);
		OutStyleName = TitleStyleName;
		bOutShowIcon = false;
		return true;
	}

	if (!m_pUIData->m_strChatText.IsEmpty())
	{
		OutDisplayText = FText::FromString(m_pUIData->m_strChatText);
		OutStyleName = ChatStyleName;
		bOutShowIcon = false;
		return true;
	}

	return false;
}

