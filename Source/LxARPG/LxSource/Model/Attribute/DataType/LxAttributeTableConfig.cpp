#include "LxAttributeTableConfig.h"

#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxAttributeTags.h"

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
		else if (!InOutAttributeData.AttributeIDTag.IsValid())
		{
			InOutAttributeData.AttributeIDTag = LxAttributeTools::GetAttributeIDTagByLegacyID(InValueConfig.AttributeID);
		}

		InOutAttributeData.AttributeValue.UpwardFloatingRatio = InValueConfig.UpwardFloatingRatio;
		InOutAttributeData.AttributeValue.DownwardFloatingRatio = InValueConfig.DownwardFloatingRatio;
		InOutAttributeData.AttributeValue.ValueLimit = InValueConfig.ValueLimit;
		InOutAttributeData.AttributeValue.Value = InValueConfig.Value;
		InOutAttributeData.CalculatedAttributeValue = InOutAttributeData.AttributeValue;
	}

	FGameplayTag ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig)
	{
		return LxAttributeTools::ResolveAttributeIDTag(InValueConfig);
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
			LxAttributeTools::NormalizeAttributeIDTag(AttributeData);
			const FGameplayTag AttributeIDTag = AttributeData.AttributeIDTag.IsValid() ? AttributeData.AttributeIDTag : AttributePair.Key;
			if (AttributeIDTag.IsValid())
			{
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
			const FGameplayTag AttributeIDTag = ResolveAttributeIDTag(ValueConfig);
			if (!AttributeIDTag.IsValid())
			{
				continue;
			}

			FLxAttributeData* FoundAttributeData = ResultMap.Find(AttributeIDTag);
			if (FoundAttributeData == nullptr)
			{
				FLxAttributeData NewData;
				ApplyBaseValueConfigToAttribute(ValueConfig, NewData);
				if (!NewData.AttributeIDTag.IsValid())
				{
					NewData.AttributeIDTag = AttributeIDTag;
				}
				ResultMap.Add(AttributeIDTag, NewData);
				continue;
			}

			ApplyBaseValueConfigToAttribute(ValueConfig, *FoundAttributeData);
		}

		return ResultMap;
	}
}

FGameplayTag LxAttributeTools::GetAttributeIDTagByLegacyID(ELxCharacterAttributeID InAttributeID)
{
	switch (InAttributeID)
	{
	case ELxCharacterAttributeID::A_Race:
		return LxTag_Attribute_Setting_Race;
	case ELxCharacterAttributeID::A_Camp:
		return LxTag_Attribute_Setting_Camp;
	case ELxCharacterAttributeID::A_CarryWeight:
		return LxTag_Attribute_Numeric_CarryWeight;
	case ELxCharacterAttributeID::A_Luck:
		return LxTag_Attribute_Numeric_Luck;
	case ELxCharacterAttributeID::B_Power:
		return LxTag_Attribute_Basic_Strength;
	case ELxCharacterAttributeID::B_Agility:
		return LxTag_Attribute_Basic_Agility;
	case ELxCharacterAttributeID::B_Intelligence:
		return LxTag_Attribute_Basic_Wisdom;
	case ELxCharacterAttributeID::B_Constitution:
		return LxTag_Attribute_Basic_Constitution;
	case ELxCharacterAttributeID::C_HP:
		return LxTag_Attribute_Resource_Health;
	case ELxCharacterAttributeID::C_MP:
		return LxTag_Attribute_Resource_Mana;
	case ELxCharacterAttributeID::C_Stamina:
		return LxTag_Attribute_Resource_Stamina;
	case ELxCharacterAttributeID::D_PhysicalAttack:
		return LxTag_Attribute_Numeric_AttackPower;
	case ELxCharacterAttributeID::D_CriticalChance:
		return LxTag_Attribute_Judgement_CriticalChance;
	case ELxCharacterAttributeID::D_CriticalDamage:
		return LxTag_Attribute_Percentage_CriticalDamage;
	case ELxCharacterAttributeID::D_AttackSpeed:
		return LxTag_Attribute_Numeric_AttackSpeed;
	case ELxCharacterAttributeID::E_Defense:
		return LxTag_Attribute_Numeric_Armor;
	case ELxCharacterAttributeID::E_Shield:
		return LxTag_Attribute_Resource_Shield;
	case ELxCharacterAttributeID::F_Fire:
		return LxTag_Attribute_Element_FireAffinity;
	case ELxCharacterAttributeID::F_Water:
		return LxTag_Attribute_Element_WaterAffinity;
	case ELxCharacterAttributeID::F_Electric:
		return LxTag_Attribute_Element_ElectricAffinity;
	case ELxCharacterAttributeID::G_LightGod:
		return LxTag_Attribute_Faith_LightGod;
	case ELxCharacterAttributeID::G_Nature:
		return LxTag_Attribute_Faith_Nature;
	default:
		return FGameplayTag();
	}
}

FGameplayTag LxAttributeTools::ResolveAttributeIDTag(const FLxAttributeData& InAttributeData)
{
	return InAttributeData.AttributeIDTag.IsValid()
		? InAttributeData.AttributeIDTag
		: GetAttributeIDTagByLegacyID(InAttributeData.AttributeID);
}

FGameplayTag LxAttributeTools::ResolveAttributeIDTag(const FLxAttributeValueConfig& InValueConfig)
{
	return InValueConfig.AttributeIDTag.IsValid()
		? InValueConfig.AttributeIDTag
		: GetAttributeIDTagByLegacyID(InValueConfig.AttributeID);
}

FGameplayTag LxAttributeTools::ResolveAttributeIDTag(const FLxAttributeDerivedRule& InDerivedRule)
{
	return InDerivedRule.AttributeIDTag.IsValid()
		? InDerivedRule.AttributeIDTag
		: GetAttributeIDTagByLegacyID(InDerivedRule.AttributeID);
}

bool LxAttributeTools::NormalizeAttributeIDTag(FLxAttributeData& InOutAttributeData)
{
	if (!InOutAttributeData.AttributeIDTag.IsValid())
	{
		InOutAttributeData.AttributeIDTag = GetAttributeIDTagByLegacyID(InOutAttributeData.AttributeID);
	}

	return InOutAttributeData.AttributeIDTag.IsValid();
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
