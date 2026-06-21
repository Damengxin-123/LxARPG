#include "LxAttributeTableConfig.h"

#include "LxARPG/LxSource/Core/Tools/LxString.h"

namespace
{
	TMap<FGameplayTag, FLxAttributeData> GAttributeDataMap;
	TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>> GCharacterRaceBaseAttributeValueMap;

	void ApplyBaseValueConfigToAttribute(const FLxAttributeValueConfig& InValueConfig, FLxAttributeData& InOutAttributeData)
	{
		if (InValueConfig.AttributeIDTag.IsValid())
		{
			InOutAttributeData.AttributeIDTag = InValueConfig.AttributeIDTag;
		}

		InOutAttributeData.AttributeValue.UpwardFloatingRatio = InValueConfig.UpwardFloatingRatio;
		InOutAttributeData.AttributeValue.DownwardFloatingRatio = InValueConfig.DownwardFloatingRatio;
		InOutAttributeData.AttributeValue.ValueLimit = InValueConfig.ValueLimit;
		InOutAttributeData.AttributeValue.Value = InValueConfig.Value;
		InOutAttributeData.CalculatedAttributeValue = InOutAttributeData.AttributeValue;
	}
}

namespace LxAttributeConfig
{
	void ClearAttributeConfig()
	{
		GAttributeDataMap.Empty();
		GCharacterRaceBaseAttributeValueMap.Empty();
	}

	void SetAttributeDataMap(const TMap<FGameplayTag, FLxAttributeData>& InAttributeDataMap)
	{
		GAttributeDataMap.Empty();
		for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : InAttributeDataMap)
		{
			FLxAttributeData AttributeData = AttributePair.Value;
			const FGameplayTag AttributeIDTag = AttributeData.AttributeIDTag.IsValid() ? AttributeData.AttributeIDTag : AttributePair.Key;
			if (AttributeIDTag.IsValid())
			{
				AttributeData.AttributeIDTag = AttributeIDTag;
				GAttributeDataMap.Add(AttributeIDTag, AttributeData);
			}
		}
	}

	void SetCharacterRaceBaseAttributeValueMap(const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& InRaceBaseValueMap)
	{
		GCharacterRaceBaseAttributeValueMap = InRaceBaseValueMap;
	}

	void SetAttributeDataConfig(const FLxAttributeData& InAttributeData)
	{
		FLxAttributeData AttributeData = InAttributeData;
		if (!LxAttributeTools::NormalizeAttributeIDTag(AttributeData))
		{
			return;
		}

		GAttributeDataMap.Add(AttributeData.AttributeIDTag, AttributeData);
	}

	void SetCharacterRaceBaseAttributeValues(ELxCharacterRaceType InRaceType, const TArray<FLxAttributeValueConfig>& InBaseValueList)
	{
		GCharacterRaceBaseAttributeValueMap.Add(InRaceType, InBaseValueList);
	}

	const TMap<FGameplayTag, FLxAttributeData>& GetAttributeDataMap()
	{
		return GAttributeDataMap;
	}

	const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& GetCharacterRaceBaseAttributeValueMap()
	{
		return GCharacterRaceBaseAttributeValueMap;
	}

	const FLxAttributeData* GetAttributeDataConfig(FGameplayTag InAttributeIDTag)
	{
		return GAttributeDataMap.Find(InAttributeIDTag);
	}

	TMap<FGameplayTag, FLxAttributeData> GetCharacterAttributeDataByRaceType(ELxCharacterRaceType InRaceType)
	{
		TMap<FGameplayTag, FLxAttributeData> ResultMap = GAttributeDataMap;
		const TArray<FLxAttributeValueConfig>* RaceBaseValues = GCharacterRaceBaseAttributeValueMap.Find(InRaceType);
		if (RaceBaseValues == nullptr)
		{
			return ResultMap;
		}

		for (const FLxAttributeValueConfig& ValueConfig : *RaceBaseValues)
		{
			const FGameplayTag AttributeIDTag = LxAttributeTools::ResolveAttributeIDTag(ValueConfig);
			if (!AttributeIDTag.IsValid())
			{
				continue;
			}

			FLxAttributeData* FoundAttributeData = ResultMap.Find(AttributeIDTag);
			if (FoundAttributeData == nullptr)
			{
				FLxAttributeData NewData;
				ApplyBaseValueConfigToAttribute(ValueConfig, NewData);
				NewData.AttributeIDTag = AttributeIDTag;
				ResultMap.Add(AttributeIDTag, NewData);
				continue;
			}

			ApplyBaseValueConfigToAttribute(ValueConfig, *FoundAttributeData);
		}

		return ResultMap;
	}
}

namespace LxAttributeTools
{
	FGameplayTag ResolveAttributeIDTag(const FLxAttributeData& InAttributeData)
	{
		return InAttributeData.AttributeIDTag;
	}

	FGameplayTag ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig)
	{
		return InValueConfig.AttributeIDTag;
	}

	FGameplayTag ResolveAttributeIDTag(const FLxAttributeDerivedRule& InDerivedRule)
	{
		return InDerivedRule.AttributeIDTag;
	}

	bool NormalizeAttributeIDTag(FLxAttributeData& InOutAttributeData)
	{
		return InOutAttributeData.AttributeIDTag.IsValid();
	}

	FText GetAttributeDisplayText(const FLxAttributeData& AttributeData)
	{
		FLxString OutText(AttributeData.ShowInfo.AttributeName);
		const FLxAttributeValue& AttributeValue = AttributeData.CalculatedAttributeValue;

		switch (AttributeValue.ValueType)
		{
		case ELxCharacterValueType::FixedNumeric:
			OutText.Arg(FLxString::DoubleToIntStr(AttributeValue.Value));
			break;
		case ELxCharacterValueType::RangedNumeric:
			OutText.Arg(FLxString::DoubleToIntStr(AttributeValue.ValueLimit));
			break;
		case ELxCharacterValueType::FloatingNumeric:
			{
				FLxString Value;
				Value << FLxString::DoubleToIntStr(AttributeValue.Value - (AttributeValue.Value * AttributeValue.DownwardFloatingRatio));
				Value << TEXT("~");
				Value << FLxString::DoubleToIntStr(AttributeValue.Value + (AttributeValue.Value * AttributeValue.UpwardFloatingRatio));
				OutText.Arg(Value);
			}
			break;
		case ELxCharacterValueType::Probabilistic:
		case ELxCharacterValueType::Percentage:
			OutText.Arg(FLxString::DoubleToIntStr(AttributeValue.Value * 100).ToFString() + TEXT("%"));
			break;
		case ELxCharacterValueType::Switch:
			OutText.Arg(AttributeValue.Value != 0 ? TEXT("Yes") : TEXT("No"));
			break;
		case ELxCharacterValueType::Setting:
			OutText.Arg(FLxString::DoubleToIntStr(AttributeValue.Value));
			break;
		}

		return OutText.ToFText();
	}
}
