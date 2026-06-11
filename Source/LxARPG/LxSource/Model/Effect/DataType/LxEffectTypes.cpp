#include "LxEffectTypes.h"

#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeTableConfig.h"
#include "UObject/Object.h"

FName FLxEffectSourceContext::MakeSourceKey() const
{
	if (SourceIDTag.IsValid())
	{
		return FName(*SourceIDTag.ToString());
	}

	if (!SourceName.IsNone())
	{
		return SourceName;
	}

	if (SourceObject != nullptr)
	{
		return FName(*SourceObject->GetPathName());
	}

	if (SourceActor != nullptr)
	{
		return FName(*SourceActor->GetPathName());
	}

	return FName(*StaticEnum<ELxEffectPackageSource>()->GetNameStringByValue(static_cast<int64>(SourceType)));
}

bool FLxAttributeModifierEffect::HasSameAggregationKey(const FLxAttributeModifierEffect& Other) const
{
	const FGameplayTag ThisAttributeIDTag = AttributeIDTag.IsValid()
		? AttributeIDTag
		: LxAttributeTools::GetAttributeIDTagByLegacyID(AttributeID);
	const FGameplayTag OtherAttributeIDTag = Other.AttributeIDTag.IsValid()
		? Other.AttributeIDTag
		: LxAttributeTools::GetAttributeIDTagByLegacyID(Other.AttributeID);

	return ThisAttributeIDTag == OtherAttributeIDTag
		&& ModifierTarget == Other.ModifierTarget
		&& ModifierOperation == Other.ModifierOperation
		&& TargetTags.Num() == Other.TargetTags.Num()
		&& TargetTags.HasAll(Other.TargetTags)
		&& Other.TargetTags.HasAll(TargetTags);
}

bool FLxEffectPackage::IsEmpty() const
{
	return AttributeModifierEffects.IsEmpty()
		&& AttributeRecoveryEffects.IsEmpty()
		&& DamageEffects.IsEmpty()
		&& StateChangeEffects.IsEmpty()
		&& BuffGrantEffects.IsEmpty();
}
