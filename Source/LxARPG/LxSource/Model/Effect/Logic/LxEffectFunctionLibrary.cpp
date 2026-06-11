#include "LxEffectFunctionLibrary.h"

void ULxEffectFunctionLibrary::AggregateAttributeModifierEffects(const TArray<FLxAttributeModifierEffect>& InEffects, TArray<FLxAttributeModifierEffect>& OutEffects)
{
	OutEffects.Reset();
	for (const FLxAttributeModifierEffect& Effect : InEffects)
	{
		AddAggregatedAttributeModifierEffect(OutEffects, Effect);
	}
}

void ULxEffectFunctionLibrary::AddAggregatedAttributeModifierEffect(TArray<FLxAttributeModifierEffect>& InOutEffects, const FLxAttributeModifierEffect& InEffect)
{
	for (FLxAttributeModifierEffect& ExistingEffect : InOutEffects)
	{
		if (ExistingEffect.HasSameAggregationKey(InEffect))
		{
			ExistingEffect.ModifierValue += InEffect.ModifierValue;
			return;
		}
	}

	InOutEffects.Add(InEffect);
}

FLxEffectPackage ULxEffectFunctionLibrary::MakeEmptyEffectPackage(FLxEffectSourceContext SourceContext, AActor* TargetActor, ELxEffectPackageApplyPolicy ApplyPolicy)
{
	FLxEffectPackage EffectPackage;
	EffectPackage.SourceContext = SourceContext;
	EffectPackage.TargetActor = TargetActor;
	EffectPackage.ApplyPolicy = ApplyPolicy;
	return EffectPackage;
}
