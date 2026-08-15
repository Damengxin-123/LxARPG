// Fill out your copyright notice in the Description page of Project Settings.

#include "LxEntry.h"

#include "LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Effect/DataType/LxEffectTypes.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxEffectFunctionLibrary.h"

namespace
{
	/** 将词条作用对象转换为属性效果作用目标。 */
	ELxAttributeModifierTarget ConvertToModifierTarget(ELxEntryTarget InEntryTarget)
	{
		switch (InEntryTarget)
		{
		case ELxEntryTarget::ToValueLimit:
			return ELxAttributeModifierTarget::ToValueLimit;
		case ELxEntryTarget::ToValue:
			return ELxAttributeModifierTarget::ToValue;
		case ELxEntryTarget::ToUpwardFloatingRatio:
			return ELxAttributeModifierTarget::ToUpwardFloatingRatio;
		case ELxEntryTarget::ToDownwardFloatingRatio:
			return ELxAttributeModifierTarget::ToDownwardFloatingRatio;
		default:
			return ELxAttributeModifierTarget::ToValue;
		}
	}

	/** 将词条作用方式转换为属性效果修改方式。 */
	ELxAttributeModifierOperation ConvertToModifierOperation(ELxEntryEffectiveType InEffectiveType)
	{
		switch (InEffectiveType)
		{
		case ELxEntryEffectiveType::BasicValue:
			return ELxAttributeModifierOperation::AddValue;
		case ELxEntryEffectiveType::BasicImprove:
			return ELxAttributeModifierOperation::AddBasePercent;
		case ELxEntryEffectiveType::AdditionalImprove:
			return ELxAttributeModifierOperation::AddTotalPercent;
		case ELxEntryEffectiveType::Mechanism:
			return ELxAttributeModifierOperation::UseMaximumValue;
		default:
			return ELxAttributeModifierOperation::AddValue;
		}
	}

	/** 将状态词条操作转换为状态效果操作。 */
	ELxStateEffectOperation ConvertToStateOperation(ELxEntryStateValue InStateValue)
	{
		switch (InStateValue)
		{
		case ELxEntryStateValue::Remove:
			return ELxStateEffectOperation::Remove;
		case ELxEntryStateValue::Toggle:
			return ELxStateEffectOperation::Toggle;
		case ELxEntryStateValue::Add:
		default:
			return ELxStateEffectOperation::Add;
		}
	}

	/** 计算词条最终缩放比例。 */
	float MakeEntryEffectScale(const FLxEntryQuote& InEntryQuote, float InEffectScale)
	{
		return InEntryQuote.EntryProportion * InEffectScale;
	}

	/** 计算词条显示用最终数值。 */
	float MakeEntryDisplayValue(float InEntryValue, const FLxEntryQuote& InEntryQuote)
	{
		return InEntryValue * InEntryQuote.EntryProportion;
	}

	/** 将词条显示名称中的数值占位符替换后，再转换为 UI 可识别的富文本字符串。 */
	FText MakeStyledEntryDisplayName(const FLxRichStyledText& InDisplayName)
	{
		return InDisplayName.GetStyledText();
	}

	/** 将词条显示名称中的数值占位符替换后，再转换为 UI 可识别的富文本字符串。 */
	FText MakeStyledEntryDisplayName(const FLxRichStyledText& InDisplayName, const FString& InDisplayValue)
	{
		FLxRichStyledText DisplayName = InDisplayName;
		FLxString DisplayText(DisplayName.Text);
		DisplayText.Arg(InDisplayValue);
		DisplayName.Text = DisplayText.ToFText();
		return DisplayName.GetStyledText();
	}
}

void ULxEntryObjectBase::InitEntry(const FLxEntryQuote& InEntryQuote)
{
	EntryQuote = InEntryQuote;

	if (InEntryQuote.EntryID.IsValid())
	{
		SetEntryData(LxEntryConfig::GetEntryData(InEntryQuote.EntryID));
	}
}

ELxEntryType ULxEntryObjectBase::GetEntryType() const
{
	if (GetEntryBase())
	{
		return GetEntryBase()->EntryType;
	}
	return ELxEntryType::NoneEntryType;
}

ULxEntryObjectBase* ULxEntryObjectBase::CreateEnterObject(UObject* InParent, const FLxEntryQuote& InEntryQuote)
{
	ULxEntryObjectBase* OutEntryObject = nullptr;

	if (InEntryQuote.EntryID.IsValid())
	{
		const FLxEntryBase* EntryData = LxEntryConfig::GetEntryData(InEntryQuote.EntryID);
		if (EntryData == nullptr)
		{
			return nullptr;
		}

		switch (EntryData->EntryType)
		{
		case ELxEntryType::AttributeGain:
			OutEntryObject = NewObject<ULxEntryObjectAttributeGain>(InParent);
			break;
		case ELxEntryType::AttributeInfluence:
			OutEntryObject = NewObject<ULxEntryObjectAttributeInfluence>(InParent);
			break;
		case ELxEntryType::AttributeRecovery:
			OutEntryObject = NewObject<ULxEntryObjectAttributeRecovery>(InParent);
			break;
		case ELxEntryType::ChangeState:
			OutEntryObject = NewObject<ULxEntryObjectChangeState>(InParent);
			break;
		case ELxEntryType::CreateBuff:
			OutEntryObject = NewObject<ULxEntryObjectCreateBuff>(InParent);
			break;
		case ELxEntryType::MultiTarget:
			OutEntryObject = NewObject<ULxEntryObjectMultiTarget>(InParent);
			break;
		case ELxEntryType::DisplayText:
			OutEntryObject = NewObject<ULxEntryObjectDisplayText>(InParent);
			break;
		case ELxEntryType::GrantSkill:
			OutEntryObject = NewObject<ULxEntryObjectGrantSkill>(InParent);
			break;
		case ELxEntryType::GrantProfession:
			OutEntryObject = NewObject<ULxEntryObjectGrantProfession>(InParent);
			break;
		case ELxEntryType::Damage:
			OutEntryObject = NewObject<ULxEntryObjectDamage>(InParent);
			break;
		default:
			return nullptr;
		}
	}
	if (OutEntryObject)
	{
		OutEntryObject->InitEntry(InEntryQuote);
	}

	return OutEntryObject;
}
/////////////////////////////////////////////////////

FText ULxEntryObjectAttributeGain::GetDisplayName() const
{
	const float DisplayEntryValue = MakeEntryDisplayValue(AttributeGainData.EntryValue, GetEntryQuote());
	FString DisplayValueString;

	switch (AttributeGainData.EffectiveType)
	{
	case ELxEntryEffectiveType::BasicValue:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue).ToFString();
		break;
	case ELxEntryEffectiveType::BasicImprove:
	case ELxEntryEffectiveType::AdditionalImprove:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue).ToFString() + TEXT("%");
		break;
	case ELxEntryEffectiveType::Mechanism:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue).ToFString();
		break;
	}

	return MakeStyledEntryDisplayName(AttributeGainData.EntryText.EntryDisplayName, DisplayValueString);
}

void ULxEntryObjectAttributeGain::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::AttributeGain)
	{
		AttributeGainData = *static_cast<const FLxEntryAttributeGain*>(InEntryData);
	}
}

void ULxEntryObjectAttributeGain::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!AttributeGainData.AttributeIDTag.IsValid() && AttributeGainData.TargetBusinessCategories.IsEmpty())
	{
		return;
	}

	FLxAttributeModifierEffect ModifierEffect;
	ModifierEffect.AttributeIDTag = AttributeGainData.AttributeIDTag;
	ModifierEffect.ModifierTarget = ConvertToModifierTarget(AttributeGainData.EntryTarget);
	ModifierEffect.ModifierOperation = ConvertToModifierOperation(AttributeGainData.EffectiveType);
	ModifierEffect.ModifierValue = AttributeGainData.EntryValue * MakeEntryEffectScale(GetEntryQuote(), InEffectScale);
	ModifierEffect.TargetBusinessCategories = AttributeGainData.TargetBusinessCategories;
	ULxEffectFunctionLibrary::AddAggregatedAttributeModifierEffect(InOutEffectPackage.AttributeModifierEffects, ModifierEffect);
}

//////////////////////////////////////////////////

FText ULxEntryObjectAttributeInfluence::GetDisplayName() const
{
	const float DisplayRatio = MakeEntryDisplayValue(AttributeInfluenceData.InfluenceRatio, GetEntryQuote());
	return MakeStyledEntryDisplayName(AttributeInfluenceData.EntryText.EntryDisplayName,
		FLxString::DoubleToIntStr(DisplayRatio).ToFString());
}

void ULxEntryObjectAttributeInfluence::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData != nullptr && InEntryData->EntryType == ELxEntryType::AttributeInfluence)
	{
		AttributeInfluenceData = *static_cast<const FLxEntryAttributeInfluence*>(InEntryData);
	}
}

void ULxEntryObjectAttributeInfluence::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, const float InEffectScale) const
{
	if (!AttributeInfluenceData.SourceAttributeIDTag.IsValid()
		|| !AttributeInfluenceData.TargetAttributeIDTag.IsValid()
		|| AttributeInfluenceData.SourceAttributeIDTag == AttributeInfluenceData.TargetAttributeIDTag)
	{
		return;
	}

	FLxAttributeModifierEffect InfluenceEffect;
	InfluenceEffect.AttributeIDTag = AttributeInfluenceData.TargetAttributeIDTag;
	InfluenceEffect.ModifierTarget = ConvertToModifierTarget(AttributeInfluenceData.TargetEntryTarget);
	InfluenceEffect.ModifierOperation = ConvertToModifierOperation(AttributeInfluenceData.EffectiveType);
	InfluenceEffect.SourceAttributeIDTag = AttributeInfluenceData.SourceAttributeIDTag;
	InfluenceEffect.SourceAttributeTarget = ConvertToModifierTarget(AttributeInfluenceData.SourceEntryTarget);
	InfluenceEffect.SourceAttributeRatio = AttributeInfluenceData.InfluenceRatio
		* MakeEntryEffectScale(GetEntryQuote(), InEffectScale);
	ULxEffectFunctionLibrary::AddAggregatedAttributeModifierEffect(InOutEffectPackage.AttributeModifierEffects, InfluenceEffect);
}

//////////////////////////////////////////////////

FText ULxEntryObjectAttributeRecovery::GetDisplayName() const
{
	const float DisplayEntryValue = MakeEntryDisplayValue(AttributeRecoveryData.EntryValue, GetEntryQuote());
	FString DisplayValueString;

	switch (AttributeRecoveryData.EffectiveType)
	{
	case ELxEntryEffectiveType::BasicValue:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue).ToFString();
		break;
	case ELxEntryEffectiveType::BasicImprove:
	case ELxEntryEffectiveType::AdditionalImprove:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue * 100).ToFString() + TEXT("%");
		break;
	case ELxEntryEffectiveType::Mechanism:
		DisplayValueString = FLxString::DoubleToIntStr(DisplayEntryValue).ToFString();
		break;
	}

	return MakeStyledEntryDisplayName(AttributeRecoveryData.EntryText.EntryDisplayName, DisplayValueString);
}

void ULxEntryObjectAttributeRecovery::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::AttributeRecovery)
	{
		AttributeRecoveryData = *static_cast<const FLxEntryAttributeRecovery*>(InEntryData);
	}
}

void ULxEntryObjectAttributeRecovery::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!AttributeRecoveryData.AttributeIDTag.IsValid() && AttributeRecoveryData.TargetBusinessCategories.IsEmpty())
	{
		return;
	}

	FLxAttributeRecoveryEffect RecoveryEffect;
	RecoveryEffect.AttributeIDTag = AttributeRecoveryData.AttributeIDTag;
	RecoveryEffect.RecoveryOperation = ConvertToModifierOperation(AttributeRecoveryData.EffectiveType);
	RecoveryEffect.RecoveryValue = AttributeRecoveryData.EntryValue * MakeEntryEffectScale(GetEntryQuote(), InEffectScale);
	if (AttributeRecoveryData.EffectiveType == ELxEntryEffectiveType::BasicImprove
		|| AttributeRecoveryData.EffectiveType == ELxEntryEffectiveType::AdditionalImprove)
	{
		RecoveryEffect.RecoveryValue *= 100.f;
	}
	RecoveryEffect.TargetBusinessCategories = AttributeRecoveryData.TargetBusinessCategories;
	InOutEffectPackage.AttributeRecoveryEffects.Add(RecoveryEffect);
}
/////////////////////////////////////////////////////
///
FText ULxEntryObjectChangeState::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(ChangeStateData.EntryText.EntryDisplayName);
}

void ULxEntryObjectChangeState::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::ChangeState)
	{
		ChangeStateData = *static_cast<const FLxEntryChangeState*>(InEntryData);
	}
}

void ULxEntryObjectChangeState::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!ChangeStateData.StateCategoryTag.IsValid() || !ChangeStateData.StateTag.IsValid())
	{
		return;
	}

	FLxStateChangeEffect StateChangeEffect;
	StateChangeEffect.StateCategoryTag = ChangeStateData.StateCategoryTag;
	StateChangeEffect.StateTag = ChangeStateData.StateTag;
	StateChangeEffect.Operation = ConvertToStateOperation(ChangeStateData.StateValue);
	InOutEffectPackage.StateChangeEffects.Add(StateChangeEffect);
}
/////////////////////////////////////////////////////
FText ULxEntryObjectCreateBuff::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(CreateBuffData.EntryText.EntryDisplayName);
}

void ULxEntryObjectCreateBuff::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::CreateBuff)
	{
		CreateBuffData = *static_cast<const FLxEntryCreateBuff*>(InEntryData);
	}
}

void ULxEntryObjectCreateBuff::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!CreateBuffData.BuffIDTag.IsValid())
	{
		return;
	}

	FLxBuffGrantEffect BuffGrantEffect;
	BuffGrantEffect.BuffIDTag = CreateBuffData.BuffIDTag;
	BuffGrantEffect.EffectProportion = MakeEntryEffectScale(GetEntryQuote(), InEffectScale);
	BuffGrantEffect.Duration = CreateBuffData.BuffDuration;
	InOutEffectPackage.BuffGrantEffects.Add(BuffGrantEffect);
}
/////////////////////////////////////////////////////
FText ULxEntryObjectMultiTarget::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(MultiTargetData.EntryText.EntryDisplayName);
}

void ULxEntryObjectMultiTarget::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::MultiTarget)
	{
		MultiTargetData = *static_cast<const FLxEntryMultiTarget*>(InEntryData);
	}
}

FText ULxEntryObjectDisplayText::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(DisplayTextData.EntryText.EntryDisplayName);
}

void ULxEntryObjectDisplayText::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::DisplayText)
	{
		DisplayTextData = *static_cast<const FLxEntryDisplayText*>(InEntryData);
	}
}

FText ULxEntryObjectGrantSkill::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(GrantSkillData.EntryText.EntryDisplayName);
}

void ULxEntryObjectGrantSkill::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::GrantSkill)
	{
		GrantSkillData = *static_cast<const FLxEntryGrantSkill*>(InEntryData);
	}
}

void ULxEntryObjectGrantSkill::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!GrantSkillData.SkillItemIDTag.IsValid())
	{
		return;
	}

	FLxSkillGrantEffect SkillGrantEffect;
	SkillGrantEffect.SkillItemIDTag = GrantSkillData.SkillItemIDTag;
	InOutEffectPackage.SkillGrantEffects.Add(SkillGrantEffect);
}

FText ULxEntryObjectGrantProfession::GetDisplayName() const
{
	return MakeStyledEntryDisplayName(GrantProfessionData.EntryText.EntryDisplayName,
		FString::FromInt(GrantProfessionData.ProfessionLevel));
}

void ULxEntryObjectGrantProfession::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::GrantProfession)
	{
		GrantProfessionData = *static_cast<const FLxEntryGrantProfession*>(InEntryData);
	}
}

void ULxEntryObjectGrantProfession::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!GrantProfessionData.ProfessionIDTag.IsValid())
	{
		return;
	}

	FLxProfessionGrantEffect ProfessionGrantEffect;
	ProfessionGrantEffect.ProfessionIDTag = GrantProfessionData.ProfessionIDTag;
	ProfessionGrantEffect.ProfessionLevel = FMath::Max(1, GrantProfessionData.ProfessionLevel);
	ProfessionGrantEffect.bCanUpgrade = GrantProfessionData.bCanUpgrade;
	InOutEffectPackage.ProfessionGrantEffects.Add(ProfessionGrantEffect);
}


FText ULxEntryObjectDamage::GetDisplayName() const
{
	const float DisplayRatio = MakeEntryDisplayValue(DamageData.SourceAttributeRatio, GetEntryQuote());
	const FString DisplayValueString = FLxString::DoubleToIntStr(DisplayRatio * 100.f).ToFString() + TEXT("%");
	return MakeStyledEntryDisplayName(DamageData.EntryText.EntryDisplayName, DisplayValueString);
}

void ULxEntryObjectDamage::SetEntryData(const FLxEntryBase* InEntryData)
{
	Super::SetEntryData(InEntryData);
	if (InEntryData && InEntryData->EntryType == ELxEntryType::Damage)
	{
		DamageData = *static_cast<const FLxEntryDamage*>(InEntryData);
	}
}

void ULxEntryObjectDamage::AppendEffectsToPackage(FLxEffectPackage& InOutEffectPackage, float InEffectScale) const
{
	if (!DamageData.SourceAttributeIDTag.IsValid() || !DamageData.DamageTypeTag.IsValid())
	{
		return;
	}

	FLxDamageValue DamageValue;
	DamageValue.DamageTypeTag = DamageData.DamageTypeTag;
	DamageValue.SourceAttributeIDTag = DamageData.SourceAttributeIDTag;
	DamageValue.SourceAttributeRatio = DamageData.SourceAttributeRatio * MakeEntryEffectScale(GetEntryQuote(), InEffectScale);

	FLxDamageEffect DamageEffect;
	DamageEffect.TargetAttributeIDTag = DamageData.TargetAttributeIDTag;
	DamageEffect.DamageTags.AddTag(DamageData.DamageTypeTag);
	DamageEffect.DamageValues.Add(DamageValue);
	InOutEffectPackage.DamageEffects.Add(DamageEffect);
}
