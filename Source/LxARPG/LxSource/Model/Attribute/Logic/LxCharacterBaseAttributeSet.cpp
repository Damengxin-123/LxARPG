#include "LxCharacterBaseAttributeSet.h"

#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTags.h"

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

	/** 使用分类索引读取一个属性副本。 */
	template<typename AttributeType>
	bool CopyIndexedAttribute(const TMap<FGameplayTag, AttributeType*>& InIndex, const FGameplayTag InAttributeIDTag, AttributeType& OutAttribute)
	{
		AttributeType* const* FoundAttribute = InIndex.Find(InAttributeIDTag);
		if (FoundAttribute == nullptr || *FoundAttribute == nullptr)
		{
			return false;
		}
		OutAttribute = **FoundAttribute;
		return true;
	}

	/** 将分类索引复制为网络快照数组。 */
	template<typename AttributeType>
	void CopyIndexToArray(const TMap<FGameplayTag, AttributeType*>& InIndex, TArray<AttributeType>& OutAttributes)
	{
		OutAttributes.Reset();
		OutAttributes.Reserve(InIndex.Num());
		for (const TPair<FGameplayTag, AttributeType*>& AttributePair : InIndex)
		{
			if (AttributePair.Value != nullptr)
			{
				AttributeType AttributeData = *AttributePair.Value;
				AttributeData.AttributeIDTag = AttributePair.Key;
				OutAttributes.Add(MoveTemp(AttributeData));
			}
		}
	}

	/** 将网络快照数组更新到已经注册的分类属性成员。 */
	template<typename AttributeType>
	void ApplyArrayToIndex(const TArray<AttributeType>& InAttributes, TMap<FGameplayTag, AttributeType*>& InOutIndex)
	{
		for (const AttributeType& AttributeData : InAttributes)
		{
			AttributeType* const* TargetAttribute = InOutIndex.Find(AttributeData.AttributeIDTag);
			if (TargetAttribute != nullptr && *TargetAttribute != nullptr)
			{
				**TargetAttribute = AttributeData;
			}
		}
	}
}

ULxCharacterBaseAttributeSet::ULxCharacterBaseAttributeSet()
{
	BaseMovementSpeed.Value = 6.f;
	RegisterBasicAttribute(LxTag_Attribute_Basic_Strength, Strength);
	RegisterBasicAttribute(LxTag_Attribute_Basic_Wisdom, Wisdom);
	RegisterBasicAttribute(LxTag_Attribute_Basic_Agility, Agility);
	RegisterBasicAttribute(LxTag_Attribute_Basic_Constitution, Constitution);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Health, Health);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Mana, Mana);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Stamina, Stamina);
	RegisterResourceAttribute(LxTag_Attribute_Resource_Shield, Shield);
	RegisterProbabilityAttribute(LxTag_Attribute_Judgement_CriticalChance, CriticalChance);
	RegisterPercentageAttribute(LxTag_Attribute_Percentage_CriticalDamage, CriticalDamage);
	RegisterPercentageAttribute(LxTag_Attribute_Percentage_BlockDamageReduction, BlockDamageReduction);
	RegisterPercentageAttribute(LxTag_Attribute_Percentage_MovementSpeedBonus, MovementSpeedBonus);
	RegisterNumericAttribute(LxTag_Attribute_Numeric_CarryWeight, CarryWeight);
	RegisterNumericAttribute(LxTag_Attribute_Numeric_Luck, Luck);
	RegisterNumericAttribute(LxTag_Attribute_Numeric_Armor, Armor);
	RegisterRangeAttribute(LxTag_Attribute_Range_AttackPower, AttackPower);
	RegisterNumericAttribute(LxTag_Attribute_Numeric_AttackSpeed, AttackSpeed);
	RegisterNumericAttribute(LxTag_Attribute_Numeric_BaseMovementSpeed, BaseMovementSpeed);
	RegisterNumericAttribute(LxTag_Attribute_Element_FireAffinity, FireAffinity);
	RegisterNumericAttribute(LxTag_Attribute_Element_WaterAffinity, WaterAffinity);
	RegisterNumericAttribute(LxTag_Attribute_Element_ElectricAffinity, ElectricAffinity);
	RegisterNumericAttribute(LxTag_Attribute_Faith_LightGod, LightGodFaith);
	RegisterNumericAttribute(LxTag_Attribute_Faith_Nature, NatureFaith);
}

bool ULxCharacterBaseAttributeSet::GetBasicAttribute(const FGameplayTag InAttributeIDTag, FLxBasicAttributeData& OutAttribute) const { return CopyIndexedAttribute(BasicAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetResourceAttribute(const FGameplayTag InAttributeIDTag, FLxResourceAttributeData& OutAttribute) const { return CopyIndexedAttribute(ResourceAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetProbabilityAttribute(const FGameplayTag InAttributeIDTag, FLxProbabilityAttributeData& OutAttribute) const { return CopyIndexedAttribute(ProbabilityAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetPercentageAttribute(const FGameplayTag InAttributeIDTag, FLxPercentageAttributeData& OutAttribute) const { return CopyIndexedAttribute(PercentageAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetNumericAttribute(const FGameplayTag InAttributeIDTag, FLxNumericAttributeData& OutAttribute) const { return CopyIndexedAttribute(NumericAttributeIndex, InAttributeIDTag, OutAttribute); }
bool ULxCharacterBaseAttributeSet::GetRangeAttribute(const FGameplayTag InAttributeIDTag, FLxRangeAttributeData& OutAttribute) const { return CopyIndexedAttribute(RangeAttributeIndex, InAttributeIDTag, OutAttribute); }

bool ULxCharacterBaseAttributeSet::ModifyBasicAttribute(const FLxBasicAttributeModifier& InModifier)
{
	FLxBasicAttributeData* Attribute = FindMutableBasicAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
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

bool ULxCharacterBaseAttributeSet::ModifyProbabilityAttribute(const FLxProbabilityAttributeModifier& InModifier)
{
	FLxProbabilityAttributeData* Attribute = FindMutableProbabilityAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
	Attribute->Value = FMath::Clamp(Attribute->Value, 0.f, 1.f);
	return true;
}

bool ULxCharacterBaseAttributeSet::ModifyPercentageAttribute(const FLxPercentageAttributeModifier& InModifier)
{
	FLxPercentageAttributeData* Attribute = FindMutablePercentageAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
	return true;
}

bool ULxCharacterBaseAttributeSet::ModifyNumericAttribute(const FLxNumericAttributeModifier& InModifier)
{
	FLxNumericAttributeData* Attribute = FindMutableNumericAttribute(InModifier.AttributeIDTag);
	if (Attribute == nullptr) return false;
	ApplyAttributeOperation(Attribute->Value, InModifier.Operation, InModifier.Value);
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

FLxBasicAttributeData* ULxCharacterBaseAttributeSet::FindMutableBasicAttribute(const FGameplayTag InAttributeIDTag) { FLxBasicAttributeData** Found = BasicAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxResourceAttributeData* ULxCharacterBaseAttributeSet::FindMutableResourceAttribute(const FGameplayTag InAttributeIDTag) { FLxResourceAttributeData** Found = ResourceAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxProbabilityAttributeData* ULxCharacterBaseAttributeSet::FindMutableProbabilityAttribute(const FGameplayTag InAttributeIDTag) { FLxProbabilityAttributeData** Found = ProbabilityAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxPercentageAttributeData* ULxCharacterBaseAttributeSet::FindMutablePercentageAttribute(const FGameplayTag InAttributeIDTag) { FLxPercentageAttributeData** Found = PercentageAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxNumericAttributeData* ULxCharacterBaseAttributeSet::FindMutableNumericAttribute(const FGameplayTag InAttributeIDTag) { FLxNumericAttributeData** Found = NumericAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }
FLxRangeAttributeData* ULxCharacterBaseAttributeSet::FindMutableRangeAttribute(const FGameplayTag InAttributeIDTag) { FLxRangeAttributeData** Found = RangeAttributeIndex.Find(InAttributeIDTag); return Found ? *Found : nullptr; }

void ULxCharacterBaseAttributeSet::GetAllBasicAttributes(TArray<FLxBasicAttributeData>& OutAttributes) const { CopyIndexToArray(BasicAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllResourceAttributes(TArray<FLxResourceAttributeData>& OutAttributes) const { CopyIndexToArray(ResourceAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllProbabilityAttributes(TArray<FLxProbabilityAttributeData>& OutAttributes) const { CopyIndexToArray(ProbabilityAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllPercentageAttributes(TArray<FLxPercentageAttributeData>& OutAttributes) const { CopyIndexToArray(PercentageAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllNumericAttributes(TArray<FLxNumericAttributeData>& OutAttributes) const { CopyIndexToArray(NumericAttributeIndex, OutAttributes); }
void ULxCharacterBaseAttributeSet::GetAllRangeAttributes(TArray<FLxRangeAttributeData>& OutAttributes) const { CopyIndexToArray(RangeAttributeIndex, OutAttributes); }

void ULxCharacterBaseAttributeSet::ApplyTypedSnapshots(const TArray<FLxBasicAttributeData>& InBasicAttributes,
	const TArray<FLxResourceAttributeData>& InResourceAttributes,
	const TArray<FLxProbabilityAttributeData>& InProbabilityAttributes,
	const TArray<FLxPercentageAttributeData>& InPercentageAttributes,
	const TArray<FLxNumericAttributeData>& InNumericAttributes,
	const TArray<FLxRangeAttributeData>& InRangeAttributes)
{
	ApplyArrayToIndex(InBasicAttributes, BasicAttributeIndex);
	ApplyArrayToIndex(InResourceAttributes, ResourceAttributeIndex);
	ApplyArrayToIndex(InProbabilityAttributes, ProbabilityAttributeIndex);
	ApplyArrayToIndex(InPercentageAttributes, PercentageAttributeIndex);
	ApplyArrayToIndex(InNumericAttributes, NumericAttributeIndex);
	ApplyArrayToIndex(InRangeAttributes, RangeAttributeIndex);
}

void ULxCharacterBaseAttributeSet::RegisterBasicAttribute(const FGameplayTag InAttributeIDTag, FLxBasicAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Basic; BasicAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
void ULxCharacterBaseAttributeSet::RegisterResourceAttribute(const FGameplayTag InAttributeIDTag, FLxResourceAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Resource; ResourceAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
void ULxCharacterBaseAttributeSet::RegisterProbabilityAttribute(const FGameplayTag InAttributeIDTag, FLxProbabilityAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Probability; ProbabilityAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
void ULxCharacterBaseAttributeSet::RegisterPercentageAttribute(const FGameplayTag InAttributeIDTag, FLxPercentageAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Percentage; PercentageAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
void ULxCharacterBaseAttributeSet::RegisterNumericAttribute(const FGameplayTag InAttributeIDTag, FLxNumericAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Numeric; NumericAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
void ULxCharacterBaseAttributeSet::RegisterRangeAttribute(const FGameplayTag InAttributeIDTag, FLxRangeAttributeData& InAttributeData) { InAttributeData.AttributeIDTag = InAttributeIDTag; InAttributeData.AttributeCategory = ELxCharacterAttributeCategoryType::Range; RangeAttributeIndex.Add(InAttributeIDTag, &InAttributeData); }
