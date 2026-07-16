#include "LxCharacterBaseAttributeSet.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"

ULxCharacterBaseAttributeSet::ULxCharacterBaseAttributeSet()
{
	RegisterAttribute(LxTag_Attribute_Basic_Strength, Strength, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Basic_Wisdom, Wisdom, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Basic_Agility, Agility, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Basic_Constitution, Constitution, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Resource_Health, Health, ELxCharacterValueType::RangedNumeric);
	RegisterAttribute(LxTag_Attribute_Resource_Mana, Mana, ELxCharacterValueType::RangedNumeric);
	RegisterAttribute(LxTag_Attribute_Resource_Stamina, Stamina, ELxCharacterValueType::RangedNumeric);
	RegisterAttribute(LxTag_Attribute_Resource_Shield, Shield, ELxCharacterValueType::RangedNumeric);
	RegisterAttribute(LxTag_Attribute_Judgement_CriticalChance, CriticalChance, ELxCharacterValueType::Probabilistic);
	RegisterAttribute(LxTag_Attribute_Percentage_CriticalDamage, CriticalDamage, ELxCharacterValueType::Percentage);
	RegisterAttribute(LxTag_Attribute_Percentage_BlockDamageReduction, BlockDamageReduction, ELxCharacterValueType::Percentage);
	RegisterAttribute(LxTag_Attribute_Numeric_CarryWeight, CarryWeight, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Numeric_Luck, Luck, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Numeric_Armor, Armor, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Numeric_AttackPower, AttackPower, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Numeric_AttackSpeed, AttackSpeed, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Element_FireAffinity, FireAffinity, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Element_WaterAffinity, WaterAffinity, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Element_ElectricAffinity, ElectricAffinity, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Faith_LightGod, LightGodFaith, ELxCharacterValueType::FixedNumeric);
	RegisterAttribute(LxTag_Attribute_Faith_Nature, NatureFaith, ELxCharacterValueType::FixedNumeric);
}

FLxAttributeData* ULxCharacterBaseAttributeSet::FindMutableAttribute(const FGameplayTag InAttributeIDTag)
{
	FLxAttributeData* const* FoundAttribute = AttributeIndex.Find(InAttributeIDTag);
	return FoundAttribute != nullptr ? *FoundAttribute : nullptr;
}

const FLxAttributeData* ULxCharacterBaseAttributeSet::FindAttribute(const FGameplayTag InAttributeIDTag) const
{
	FLxAttributeData* const* FoundAttribute = AttributeIndex.Find(InAttributeIDTag);
	return FoundAttribute != nullptr ? *FoundAttribute : nullptr;
}

void ULxCharacterBaseAttributeSet::BuildAttributeDataMap(TMap<FGameplayTag, FLxAttributeData>& OutAttributeDataMap) const
{
	OutAttributeDataMap.Reset();
	OutAttributeDataMap.Reserve(AttributeIndex.Num());
	for (const TPair<FGameplayTag, FLxAttributeData*>& AttributePair : AttributeIndex)
	{
		if (AttributePair.Value != nullptr)
		{
			FLxAttributeData AttributeData = *AttributePair.Value;
			AttributeData.AttributeIDTag = AttributePair.Key;
			OutAttributeDataMap.Add(AttributePair.Key, MoveTemp(AttributeData));
		}
	}
}

void ULxCharacterBaseAttributeSet::ApplyAttributeDataMap(const TMap<FGameplayTag, FLxAttributeData>& InAttributeDataMap)
{
	for (const TPair<FGameplayTag, FLxAttributeData>& AttributePair : InAttributeDataMap)
	{
		if (FLxAttributeData* TargetAttribute = FindMutableAttribute(AttributePair.Key))
		{
			*TargetAttribute = AttributePair.Value;
		}
	}
}

void ULxCharacterBaseAttributeSet::RegisterAttribute(const FGameplayTag InAttributeIDTag, FLxAttributeData& InAttributeData, const ELxCharacterValueType InValueType)
{
	InAttributeData.AttributeIDTag = InAttributeIDTag;
	InAttributeData.AttributeValue.ValueType = InValueType;
	InAttributeData.CalculatedAttributeValue = InAttributeData.AttributeValue;
	AttributeIndex.Add(InAttributeIDTag, &InAttributeData);
}
