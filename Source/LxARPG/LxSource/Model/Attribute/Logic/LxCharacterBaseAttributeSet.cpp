#include "LxCharacterBaseAttributeSet.h"

#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"

namespace
{
	/** 按修改方式更新一个属性数值。 */
	void ApplyAttributeOperation(float& InOutValue, const ELxAttributeModifierOperation InOperation, const float InModifierValue)
	{
		switch (InOperation)
		{
		case ELxAttributeModifierOperation::AddValue: InOutValue += InModifierValue; break;
		case ELxAttributeModifierOperation::AddBasePercent:
		case ELxAttributeModifierOperation::AddTotalPercent: InOutValue += InOutValue * InModifierValue * 0.01f; break;
		case ELxAttributeModifierOperation::UseMaximumValue: InOutValue = FMath::Max(InOutValue, InModifierValue); break;
		case ELxAttributeModifierOperation::UseMinimumValue: InOutValue = FMath::Min(InOutValue, InModifierValue); break;
		}
	}

	/** 使用属性索引读取一个属性副本。 */
	template<typename AttributeType>
	bool CopyIndexedAttribute(const TMap<FGameplayTag, AttributeType*>& InIndex, const FGameplayTag InAttributeIDTag, AttributeType& OutAttribute)
	{
		AttributeType* const* FoundAttribute = InIndex.Find(InAttributeIDTag);
		if (FoundAttribute == nullptr || *FoundAttribute == nullptr) return false;
		OutAttribute = **FoundAttribute;
		return true;
	}

	/** 将属性索引复制为网络快照数组。 */
	template<typename AttributeType>
	void CopyIndexToArray(const TMap<FGameplayTag, AttributeType*>& InIndex, TArray<AttributeType>& OutAttributes)
	{
		OutAttributes.Reset();
		OutAttributes.Reserve(InIndex.Num());
		for (const TPair<FGameplayTag, AttributeType*>& AttributePair : InIndex)
		{
			if (AttributePair.Value == nullptr) continue;
			AttributeType AttributeData = *AttributePair.Value;
			AttributeData.AttributeIDTag = AttributePair.Key;
			OutAttributes.Add(MoveTemp(AttributeData));
		}
	}

	/** 将网络快照更新到已经注册的属性成员。 */
	template<typename AttributeType>
	void ApplyArrayToIndex(const TArray<AttributeType>& InAttributes, TMap<FGameplayTag, AttributeType*>& InOutIndex)
	{
		for (const AttributeType& AttributeData : InAttributes)
		{
			AttributeType* const* TargetAttribute = InOutIndex.Find(AttributeData.AttributeIDTag);
			if (TargetAttribute != nullptr && *TargetAttribute != nullptr) **TargetAttribute = AttributeData;
		}
	}
}

ULxCharacterBaseAttributeSet::ULxCharacterBaseAttributeSet()
{
	BaseMovementSpeed.Value = 6.f;

	RegisterScalarAttribute(LxTag_Attribute_Ability_Strength, Strength, ELxCharacterAttributeBusinessCategory::Ability);
	RegisterScalarAttribute(LxTag_Attribute_Ability_Wisdom, Wisdom, ELxCharacterAttributeBusinessCategory::Ability);
	RegisterScalarAttribute(LxTag_Attribute_Ability_Agility, Agility, ELxCharacterAttributeBusinessCategory::Ability);
	RegisterScalarAttribute(LxTag_Attribute_Ability_Constitution, Constitution, ELxCharacterAttributeBusinessCategory::Ability);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Health, Health, ELxCharacterAttributeBusinessCategory::Resource);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Mana, Mana, ELxCharacterAttributeBusinessCategory::Resource);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Stamina, Stamina, ELxCharacterAttributeBusinessCategory::Resource);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Shield, Shield, ELxCharacterAttributeBusinessCategory::Resource);
	RegisterScalarAttribute(LxTag_Attribute_Judgement_CriticalChance, CriticalChance, ELxCharacterAttributeBusinessCategory::Judgement);
	RegisterScalarAttribute(LxTag_Attribute_Combat_CriticalDamage, CriticalDamage, ELxCharacterAttributeBusinessCategory::Combat);
	RegisterScalarAttribute(LxTag_Attribute_Combat_BlockDamageReduction, BlockDamageReduction, ELxCharacterAttributeBusinessCategory::Combat);
	RegisterScalarAttribute(LxTag_Attribute_Action_MovementSpeedBonus, MovementSpeedBonus, ELxCharacterAttributeBusinessCategory::Action);
	RegisterScalarAttribute(LxTag_Attribute_Action_CarryWeight, CarryWeight, ELxCharacterAttributeBusinessCategory::Action);
	RegisterScalarAttribute(LxTag_Attribute_Loot_Luck, Luck, ELxCharacterAttributeBusinessCategory::Loot);
	RegisterScalarAttribute(LxTag_Attribute_Combat_Armor, Armor, ELxCharacterAttributeBusinessCategory::Combat);
	RegisterRangeAttribute(LxTag_Attribute_Combat_AttackPower, AttackPower, ELxCharacterAttributeBusinessCategory::Combat);
	RegisterScalarAttribute(LxTag_Attribute_Combat_AttackSpeed, AttackSpeed, ELxCharacterAttributeBusinessCategory::Combat);
	RegisterScalarAttribute(LxTag_Attribute_Action_BaseMovementSpeed, BaseMovementSpeed, ELxCharacterAttributeBusinessCategory::Action);
	RegisterScalarAttribute(LxTag_Attribute_Element_FireAffinity, FireAffinity, ELxCharacterAttributeBusinessCategory::Element);
	RegisterScalarAttribute(LxTag_Attribute_Element_WaterAffinity, WaterAffinity, ELxCharacterAttributeBusinessCategory::Element);
	RegisterScalarAttribute(LxTag_Attribute_Element_ElectricAffinity, ElectricAffinity, ELxCharacterAttributeBusinessCategory::Element);
	RegisterScalarAttribute(LxTag_Attribute_Faith_LightGod, LightGodFaith, ELxCharacterAttributeBusinessCategory::Faith);
	RegisterScalarAttribute(LxTag_Attribute_Faith_Nature, NatureFaith, ELxCharacterAttributeBusinessCategory::Faith);

	CriticalChance.ScalarRule.bClampMinimum = true;
	CriticalChance.ScalarRule.bClampMaximum = true;
	CriticalChance.ScalarRule.MinimumValue = 0.f;
	CriticalChance.ScalarRule.MaximumValue = 1.f;
	CriticalChance.ScalarRule.DisplayFormat = ELxScalarAttributeDisplayFormat::Percentage;
	CriticalDamage.ScalarRule.DisplayFormat = ELxScalarAttributeDisplayFormat::Percentage;
	BlockDamageReduction.ScalarRule.bClampMinimum = true;
	BlockDamageReduction.ScalarRule.bClampMaximum = true;
	BlockDamageReduction.ScalarRule.MinimumValue = 0.f;
	BlockDamageReduction.ScalarRule.MaximumValue = 1.f;
	BlockDamageReduction.ScalarRule.DisplayFormat = ELxScalarAttributeDisplayFormat::Percentage;
	MovementSpeedBonus.ScalarRule.DisplayFormat = ELxScalarAttributeDisplayFormat::Percentage;
}

bool ULxCharacterBaseAttributeSet::GetScalarAttribute(const FGameplayTag InAttributeIDTag, FLxScalarAttributeData& OutAttribute) const { return CopyIndexedAttribute(ScalarAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetResourceAttribute(const FGameplayTag InAttributeIDTag, FLxResourceAttributeData& OutAttribute) const { return CopyIndexedAttribute(ResourceAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetRangeAttribute(const FGameplayTag InAttributeIDTag, FLxRangeAttributeData& OutAttribute) const { return CopyIndexedAttribute(RangeAttributeIndex, InAttributeIDTag, OutAttribute); }

bool ULxCharacterBaseAttributeSet::ModifyScalarAttribute(const FLxScalarAttributeModifier& InModifier)
{
	FLxScalarAttributeData* Attribute = FindMutableScalarAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
	Attribute->Value = Attribute->ScalarRule.NormalizeValue(Attribute->Value);
	return true;
}

bool ULxCharacterBaseAttributeSet::ModifyResourceAttribute(const FLxResourceAttributeModifier& InModifier)
{
	FLxResourceAttributeData* Attribute = FindMutableResourceAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->ValueLimit, InModifier.Operation, InModifier.ValueLimit);
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
	Attribute->ValueLimit = FMath::Max(0.f, Attribute->ValueLimit);
	Attribute->Value = FMath::Clamp(Attribute->Value, 0.f, Attribute->ValueLimit);
	return true;
}

bool ULxCharacterBaseAttributeSet::ModifyRangeAttribute(const FLxRangeAttributeModifier& InModifier)
{
	FLxRangeAttributeData* Attribute = FindMutableRangeAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
	ApplyAttributeOperation(Attribute->UpwardFloatingRatio, InModifier.Operation, InModifier.UpwardFloatingRatio);
	ApplyAttributeOperation(Attribute->DownwardFloatingRatio, InModifier.Operation, InModifier.DownwardFloatingRatio);
	return true;
}

FLxScalarAttributeData* ULxCharacterBaseAttributeSet::FindMutableScalarAttribute(const FGameplayTag InAttributeIDTag) { FLxScalarAttributeData** Found = ScalarAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxResourceAttributeData* ULxCharacterBaseAttributeSet::FindMutableResourceAttribute(const FGameplayTag InAttributeIDTag) { FLxResourceAttributeData** Found = ResourceAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxRangeAttributeData* ULxCharacterBaseAttributeSet::FindMutableRangeAttribute(const FGameplayTag InAttributeIDTag) { FLxRangeAttributeData** Found = RangeAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }

void ULxCharacterBaseAttributeSet::GetAllScalarAttributes(TArray<FLxScalarAttributeData>& OutAttributes) const { CopyIndexToArray(ScalarAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllResourceAttributes(TArray<FLxResourceAttributeData>& OutAttributes) const { CopyIndexToArray(ResourceAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllRangeAttributes(TArray<FLxRangeAttributeData>& OutAttributes) const { CopyIndexToArray(RangeAttributeIndex, OutAttributes); }

void ULxCharacterBaseAttributeSet::ApplyTypedSnapshots(const TArray<FLxScalarAttributeData>& InScalarAttributes,
	const TArray<FLxResourceAttributeData>& InResourceAttributes, const TArray<FLxRangeAttributeData>& InRangeAttributes)
{
	ApplyArrayToIndex(InScalarAttributes, ScalarAttributeIndex);
	ApplyArrayToIndex(InResourceAttributes, ResourceAttributeIndex);
	ApplyArrayToIndex(InRangeAttributes, RangeAttributeIndex);
}

void ULxCharacterBaseAttributeSet::RegisterScalarAttribute(const FGameplayTag InAttributeIDTag, FLxScalarAttributeData& InAttributeData, const ELxCharacterAttributeBusinessCategory InBusinessCategory)
{
	InAttributeData.AttributeIDTag = InAttributeIDTag;
	InAttributeData.BusinessCategory = InBusinessCategory;
	InAttributeData.ValueType = ELxCharacterAttributeValueType::Scalar;
	InAttributeData.ScalarRule.bRoundValue = true;
	ScalarAttributeIndex.Add(InAttributeIDTag, &InAttributeData);
}

void ULxCharacterBaseAttributeSet::RegisterResourceAttribute(const FGameplayTag InAttributeIDTag, FLxResourceAttributeData& InAttributeData, const ELxCharacterAttributeBusinessCategory InBusinessCategory)
{
	InAttributeData.AttributeIDTag = InAttributeIDTag;
	InAttributeData.BusinessCategory = InBusinessCategory;
	InAttributeData.ValueType = ELxCharacterAttributeValueType::Resource;
	ResourceAttributeIndex.Add(InAttributeIDTag, &InAttributeData);
}

void ULxCharacterBaseAttributeSet::RegisterRangeAttribute(const FGameplayTag InAttributeIDTag, FLxRangeAttributeData& InAttributeData, const ELxCharacterAttributeBusinessCategory InBusinessCategory)
{
	InAttributeData.AttributeIDTag = InAttributeIDTag;
	InAttributeData.BusinessCategory = InBusinessCategory;
	InAttributeData.ValueType = ELxCharacterAttributeValueType::Range;
	RangeAttributeIndex.Add(InAttributeIDTag, &InAttributeData);
}
