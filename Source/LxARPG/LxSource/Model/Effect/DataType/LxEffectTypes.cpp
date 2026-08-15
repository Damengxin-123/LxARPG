#include "LxEffectTypes.h"

#include "GameFramework/Actor.h"
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
	return AttributeIDTag == Other.AttributeIDTag
		&& ModifierTarget == Other.ModifierTarget
		&& ModifierOperation == Other.ModifierOperation
		&& SourceAttributeIDTag == Other.SourceAttributeIDTag
		&& SourceAttributeTarget == Other.SourceAttributeTarget
		&& TargetBusinessCategories == Other.TargetBusinessCategories;
}

bool FLxEffectPackage::IsEmpty() const
{
	return AttributeModifierEffects.IsEmpty()
		&& AttributeRecoveryEffects.IsEmpty()
		&& DamageEffects.IsEmpty()
		&& StateChangeEffects.IsEmpty()
		&& BuffGrantEffects.IsEmpty()
		&& SkillGrantEffects.IsEmpty()
		&& ProfessionGrantEffects.IsEmpty();
}
