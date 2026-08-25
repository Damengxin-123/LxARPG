#include "LxCharacterBaseAttributeConfig.h"

#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

namespace
{
	/** 初始化标量属性不可由数据表修改的固定元数据。 */
	void InitializeScalarAttribute(FLxScalarAttributeData& OutAttribute, const FGameplayTag InAttributeTag,
		const ELxCharacterAttributeBusinessCategory InBusinessCategory)
	{
		OutAttribute.AttributeIDTag = InAttributeTag;
		OutAttribute.BusinessCategory = InBusinessCategory;
		OutAttribute.ValueType = ELxCharacterAttributeValueType::Scalar;
		OutAttribute.ScalarRule.bRoundValue = true;
	}

	/** 初始化资源属性不可由数据表修改的固定元数据。 */
	void InitializeResourceAttribute(FLxResourceAttributeData& OutAttribute, const FGameplayTag InAttributeTag,
		const ELxCharacterAttributeBusinessCategory InBusinessCategory)
	{
		OutAttribute.AttributeIDTag = InAttributeTag;
		OutAttribute.BusinessCategory = InBusinessCategory;
		OutAttribute.ValueType = ELxCharacterAttributeValueType::Resource;
	}

	/** 初始化区间属性不可由数据表修改的固定元数据。 */
	void InitializeRangeAttribute(FLxRangeAttributeData& OutAttribute, const FGameplayTag InAttributeTag,
		const ELxCharacterAttributeBusinessCategory InBusinessCategory)
	{
		OutAttribute.AttributeIDTag = InAttributeTag;
		OutAttribute.BusinessCategory = InBusinessCategory;
		OutAttribute.ValueType = ELxCharacterAttributeValueType::Range;
	}
}

FLxCharacterBaseAttributeConfig::FLxCharacterBaseAttributeConfig()
{
	BaseMovementSpeed.Value = 6.f;

	InitializeScalarAttribute(Strength, LxTag_Attribute_Ability_Strength, ELxCharacterAttributeBusinessCategory::Ability);
	InitializeScalarAttribute(Wisdom, LxTag_Attribute_Ability_Wisdom, ELxCharacterAttributeBusinessCategory::Ability);
	InitializeScalarAttribute(Agility, LxTag_Attribute_Ability_Agility, ELxCharacterAttributeBusinessCategory::Ability);
	InitializeScalarAttribute(Constitution, LxTag_Attribute_Ability_Constitution, ELxCharacterAttributeBusinessCategory::Ability);
	InitializeResourceAttribute(Health, LxTag_Attribute_Resource_Health, ELxCharacterAttributeBusinessCategory::Resource);
	InitializeResourceAttribute(Mana, LxTag_Attribute_Resource_Mana, ELxCharacterAttributeBusinessCategory::Resource);
	InitializeResourceAttribute(Stamina, LxTag_Attribute_Resource_Stamina, ELxCharacterAttributeBusinessCategory::Resource);
	InitializeResourceAttribute(Shield, LxTag_Attribute_Resource_Shield, ELxCharacterAttributeBusinessCategory::Resource);
	InitializeScalarAttribute(CriticalChance, LxTag_Attribute_Judgement_CriticalChance, ELxCharacterAttributeBusinessCategory::Judgement);
	InitializeScalarAttribute(CriticalDamage, LxTag_Attribute_Combat_CriticalDamage, ELxCharacterAttributeBusinessCategory::Combat);
	InitializeScalarAttribute(BlockDamageReduction, LxTag_Attribute_Combat_BlockDamageReduction, ELxCharacterAttributeBusinessCategory::Combat);
	InitializeScalarAttribute(Armor, LxTag_Attribute_Combat_Armor, ELxCharacterAttributeBusinessCategory::Combat);
	InitializeRangeAttribute(AttackPower, LxTag_Attribute_Combat_AttackPower, ELxCharacterAttributeBusinessCategory::Combat);
	InitializeScalarAttribute(AttackSpeed, LxTag_Attribute_Combat_AttackSpeed, ELxCharacterAttributeBusinessCategory::Combat);
	InitializeScalarAttribute(MovementSpeedBonus, LxTag_Attribute_Action_MovementSpeedBonus, ELxCharacterAttributeBusinessCategory::Action);
	InitializeScalarAttribute(CarryWeight, LxTag_Attribute_Action_CarryWeight, ELxCharacterAttributeBusinessCategory::Action);
	InitializeScalarAttribute(BaseMovementSpeed, LxTag_Attribute_Action_BaseMovementSpeed, ELxCharacterAttributeBusinessCategory::Action);
	InitializeScalarAttribute(Luck, LxTag_Attribute_Loot_Luck, ELxCharacterAttributeBusinessCategory::Loot);
	InitializeScalarAttribute(FireAffinity, LxTag_Attribute_Element_FireAffinity, ELxCharacterAttributeBusinessCategory::Element);
	InitializeScalarAttribute(WaterAffinity, LxTag_Attribute_Element_WaterAffinity, ELxCharacterAttributeBusinessCategory::Element);
	InitializeScalarAttribute(ElectricAffinity, LxTag_Attribute_Element_ElectricAffinity, ELxCharacterAttributeBusinessCategory::Element);
	InitializeScalarAttribute(LightGodFaith, LxTag_Attribute_Faith_LightGod, ELxCharacterAttributeBusinessCategory::Faith);
	InitializeScalarAttribute(NatureFaith, LxTag_Attribute_Faith_Nature, ELxCharacterAttributeBusinessCategory::Faith);

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

bool ULxCharacterBaseAttributeConfigFunctionLibrary::GetCharacterBaseAttributeConfig(
	const FGameplayTag InCharacterIDTag, FLxCharacterBaseAttributeConfig& OutAttributeConfig)
{
	OutAttributeConfig = FLxCharacterBaseAttributeConfig();
	if (!InCharacterIDTag.IsValid())
	{
		return false;
	}

	const ULxGameSettings* GameSettings = GetDefault<ULxGameSettings>();
	const UDataTable* AttributeTable = GameSettings != nullptr ? GameSettings->CharacterBaseAttributeTable.LoadSynchronous() : nullptr;
	if (AttributeTable == nullptr || AttributeTable->GetRowStruct() != FLxCharacterBaseAttributeConfig::StaticStruct())
	{
		return false;
	}

	const FString ContextString = TEXT("ULxCharacterBaseAttributeConfigFunctionLibrary::GetCharacterBaseAttributeConfig");
	TArray<FLxCharacterBaseAttributeConfig*> AttributeConfigs;
	AttributeTable->GetAllRows<FLxCharacterBaseAttributeConfig>(ContextString, AttributeConfigs);
	const FLxCharacterBaseAttributeConfig* MatchedConfig = nullptr;
	for (const FLxCharacterBaseAttributeConfig* CurrentConfig : AttributeConfigs)
	{
		if (CurrentConfig == nullptr || CurrentConfig->CharacterTag != InCharacterIDTag)
		{
			continue;
		}

		if (MatchedConfig != nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("角色基础属性表存在重复角色ID：%s。角色ID必须唯一。"),
				*InCharacterIDTag.ToString());
			return false;
		}

		MatchedConfig = CurrentConfig;
	}

	if (MatchedConfig != nullptr)
	{
		OutAttributeConfig = *MatchedConfig;
		return true;
	}

	return false;
}
