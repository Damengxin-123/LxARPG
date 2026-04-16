#include "LxAttributeValueTool.h"

namespace
{
	int32 ToDiscreteValue(float InValue)
	{
		return FMath::RoundToInt(InValue);
	}

	FText BuildPercentText(float InValue)
	{
		FNumberFormattingOptions NumberFormatOptions;
		NumberFormatOptions.MinimumFractionalDigits = 0;
		NumberFormatOptions.MaximumFractionalDigits = 2;
		return FText::Format(FText::FromString(TEXT("{0}%")), FText::AsNumber(InValue * 100.0f, &NumberFormatOptions));
	}
}

int32 FLxAttributeValueTool::GetFloatingValueMin(const FLxAttributeValue& AttributeValue)
{
	const int32 DownwardValue = ToDiscreteValue(AttributeValue.Value - AttributeValue.Value * AttributeValue.DownwardFloatingRatio);
	const int32 UpwardValue = ToDiscreteValue(AttributeValue.Value + AttributeValue.Value * AttributeValue.UpwardFloatingRatio);
	return FMath::Min(DownwardValue, UpwardValue);
}

int32 FLxAttributeValueTool::GetFloatingValueMax(const FLxAttributeValue& AttributeValue)
{
	const int32 DownwardValue = ToDiscreteValue(AttributeValue.Value - AttributeValue.Value * AttributeValue.DownwardFloatingRatio);
	const int32 UpwardValue = ToDiscreteValue(AttributeValue.Value + AttributeValue.Value * AttributeValue.UpwardFloatingRatio);
	return FMath::Max(DownwardValue, UpwardValue);
}

FText FLxAttributeValueTool::BuildAttributeValueText(const FLxAttributeData& AttributeData)
{
	const FLxAttributeValue& AttributeValue = AttributeData.CalculatedAttributeValue;
	switch (AttributeValue.ValueType)
	{
	case ELxCharacterValueType::RangedNumeric:
		return FText::AsNumber(ToDiscreteValue(AttributeValue.ValueLimit));

	case ELxCharacterValueType::FloatingNumeric:
		return FText::FromString(FString::Printf(TEXT("%d~%d"), GetFloatingValueMin(AttributeValue), GetFloatingValueMax(AttributeValue)));

	case ELxCharacterValueType::Percentage:
	case ELxCharacterValueType::Probabilistic:
		return BuildPercentText(AttributeValue.Value);

	case ELxCharacterValueType::Switch:
		return AttributeValue.Value != 0 ? FText::FromString(TEXT("Yes")) : FText::FromString(TEXT("No"));

	case ELxCharacterValueType::Setting:
	case ELxCharacterValueType::FixedNumeric:
	default:
		return FText::AsNumber(ToDiscreteValue(AttributeValue.Value));
	}
}
