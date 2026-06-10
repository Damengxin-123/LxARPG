#include "LxAttributeTableConfig.h"

#include "LxARPG/LxSource/Core/Tools/LxString.h"

namespace
{
	TMap<ELxCharacterAttributeID, FLxAttributeData> GAttributeDataMap;
	TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>> GCharacterRaceBaseAttributeValueMap;

	void ApplyBaseValueConfigToAttribute(const FLxAttributeValueConfig& InValueConfig, FLxAttributeData& InOutAttributeData)
	{
		if (InValueConfig.AttributeID != ELxCharacterAttributeID::X_None)
		{
			InOutAttributeData.AttributeID = InValueConfig.AttributeID;
		}
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

	ELxCharacterAttributeID ResolveAttributeID(const TMap<ELxCharacterAttributeID, FLxAttributeData>& InAttributeDataMap, const FLxAttributeValueConfig& InValueConfig)
	{
		if (InValueConfig.AttributeID != ELxCharacterAttributeID::X_None)
		{
			return InValueConfig.AttributeID;
		}

		if (!InValueConfig.AttributeIDTag.IsValid())
		{
			return ELxCharacterAttributeID::X_None;
		}

		for (const TPair<ELxCharacterAttributeID, FLxAttributeData>& AttributePair : InAttributeDataMap)
		{
			if (AttributePair.Value.AttributeIDTag == InValueConfig.AttributeIDTag)
			{
				return AttributePair.Key;
			}
		}

		return ELxCharacterAttributeID::X_None;
	}
}

namespace LxAttributeConfig
{
	void ClearAttributeConfig()
	{
		GAttributeDataMap.Empty();
		GCharacterRaceBaseAttributeValueMap.Empty();
	}

	void SetAttributeDataMap(const TMap<ELxCharacterAttributeID, FLxAttributeData>& InAttributeDataMap)
	{
		GAttributeDataMap = InAttributeDataMap;
	}

	void SetCharacterRaceBaseAttributeValueMap(const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& InRaceBaseValueMap)
	{
		GCharacterRaceBaseAttributeValueMap = InRaceBaseValueMap;
	}

	void SetAttributeDataConfig(const FLxAttributeData& InAttributeData)
	{
		if (InAttributeData.AttributeID == ELxCharacterAttributeID::X_None)
		{
			return;
		}

		GAttributeDataMap.Add(InAttributeData.AttributeID, InAttributeData);
	}

	void SetCharacterRaceBaseAttributeValues(ELxCharacterRaceType InRaceType, const TArray<FLxAttributeValueConfig>& InBaseValueList)
	{
		GCharacterRaceBaseAttributeValueMap.Add(InRaceType, InBaseValueList);
	}

	const TMap<ELxCharacterAttributeID, FLxAttributeData>& GetAttributeDataMap()
	{
		return GAttributeDataMap;
	}

	const TMap<ELxCharacterRaceType, TArray<FLxAttributeValueConfig>>& GetCharacterRaceBaseAttributeValueMap()
	{
		return GCharacterRaceBaseAttributeValueMap;
	}

	const FLxAttributeData* GetAttributeDataConfig(ELxCharacterAttributeID InAttributeID)
	{
		return GAttributeDataMap.Find(InAttributeID);
	}

	TMap<ELxCharacterAttributeID, FLxAttributeData> GetCharacterAttributeDataByRaceType(ELxCharacterRaceType InRaceType)
	{
		TMap<ELxCharacterAttributeID, FLxAttributeData> ResultMap = GAttributeDataMap;
		const TArray<FLxAttributeValueConfig>* RaceBaseValues = GCharacterRaceBaseAttributeValueMap.Find(InRaceType);
		if (RaceBaseValues == nullptr)
		{
			return ResultMap;
		}

		for (const FLxAttributeValueConfig& ValueConfig : *RaceBaseValues)
		{
			const ELxCharacterAttributeID AttributeID = ResolveAttributeID(ResultMap, ValueConfig);
			if (AttributeID == ELxCharacterAttributeID::X_None)
			{
				continue;
			}

			FLxAttributeData* FoundAttributeData = ResultMap.Find(AttributeID);
			if (FoundAttributeData == nullptr)
			{
				FLxAttributeData NewData;
				ApplyBaseValueConfigToAttribute(ValueConfig, NewData);
				if (NewData.AttributeID == ELxCharacterAttributeID::X_None)
				{
					NewData.AttributeID = AttributeID;
				}
				ResultMap.Add(AttributeID, NewData);
				continue;
			}

			ApplyBaseValueConfigToAttribute(ValueConfig, *FoundAttributeData);
		}

		return ResultMap;
	}
}

FText LxAttributeTools::GetAttributeDisplayText(const FLxAttributeData& AttributeData)
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
			Value << "—";
			Value << FLxString::DoubleToIntStr(AttributeValue.Value + (AttributeValue.Value * (AttributeValue.UpwardFloatingRatio)));
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
