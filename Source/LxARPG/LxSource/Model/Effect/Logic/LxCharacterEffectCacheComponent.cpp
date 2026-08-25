#include "LxCharacterEffectCacheComponent.h"

#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterEffectCacheModule::ULxCharacterEffectCacheModule() = default;

void ULxCharacterEffectCacheModule::OnModuleInitialize()
{
	CacheOwnerComponents();
	RefreshCachedEffects();
}

bool ULxCharacterEffectCacheModule::ApplyOrUpdateCachedEffectPackage(FName EffectCacheHandle,
	const FLxEffectPackage& InEffectPackage)
{
	if (EffectCacheHandle.IsNone() || InEffectPackage.IsEmpty())
	{
		return false;
	}

	CachedEffectPackages.Add(EffectCacheHandle, InEffectPackage);
	RefreshCachedEffects();
	return true;
}

bool ULxCharacterEffectCacheModule::RemoveCachedEffectPackage(FName EffectCacheHandle)
{
	if (EffectCacheHandle.IsNone())
	{
		return false;
	}

	const int32 RemovedCount = CachedEffectPackages.Remove(EffectCacheHandle);
	if (RemovedCount <= 0 && !AppliedAttributeEffectHandles.Contains(EffectCacheHandle))
	{
		return false;
	}

	RefreshCachedEffects();
	return true;
}

void ULxCharacterEffectCacheModule::ClearCachedEffectPackages()
{
	if (CachedEffectPackages.IsEmpty() && AppliedAttributeEffectHandles.IsEmpty())
	{
		return;
	}

	CachedEffectPackages.Reset();
	RefreshCachedEffects();
}

FName ULxCharacterEffectCacheModule::MakeEffectCacheHandle(const FLxEffectSourceContext& SourceContext)
{
	return SourceContext.MakeSourceKey();
}

void ULxCharacterEffectCacheModule::CacheOwnerComponents()
{
	const ALxBaseCharacter* OwnerCharacter = GetCharacterOwner();
	if (OwnerCharacter == nullptr)
	{
		return;
	}

	AttributeComponent = OwnerCharacter->GetCharacterAttributeComponent();
}

void ULxCharacterEffectCacheModule::RefreshCachedEffects()
{
	CacheOwnerComponents();
	if (AttributeComponent == nullptr)
	{
		return;
	}

	TSet<FName> NewAttributeEffectHandles;
	for (const TPair<FName, FLxEffectPackage>& CachedPackagePair : CachedEffectPackages)
	{
		const FName EffectCacheHandle = CachedPackagePair.Key;
		const FLxEffectPackage& CachedPackage = CachedPackagePair.Value;
		if (EffectCacheHandle.IsNone() || CachedPackage.AttributeModifierEffects.IsEmpty())
		{
			continue;
		}

		const FLxEffectSourceContext CacheSourceContext = MakeCacheSourceContext(EffectCacheHandle, CachedPackage.SourceContext);
		AttributeComponent->ReceiveAttributeModifierEffects(CacheSourceContext,
			ELxEffectPackageApplyPolicy::ReplaceSameSource, CachedPackage.AttributeModifierEffects);
		NewAttributeEffectHandles.Add(EffectCacheHandle);
	}

	for (const FName& OldAttributeEffectHandle : AppliedAttributeEffectHandles)
	{
		if (NewAttributeEffectHandles.Contains(OldAttributeEffectHandle))
		{
			continue;
		}

		const FLxEffectSourceContext CacheSourceContext = MakeCacheSourceContext(
			OldAttributeEffectHandle, FLxEffectSourceContext());
		AttributeComponent->ReceiveAttributeModifierEffects(CacheSourceContext,
			ELxEffectPackageApplyPolicy::ReplaceSameSource, TArray<FLxAttributeModifierEffect>());
	}

	AppliedAttributeEffectHandles = MoveTemp(NewAttributeEffectHandles);
}

FLxEffectSourceContext ULxCharacterEffectCacheModule::MakeCacheSourceContext(FName EffectCacheHandle,
	const FLxEffectSourceContext& SourceContext)
{
	FLxEffectSourceContext CacheSourceContext = SourceContext;
	CacheSourceContext.SourceIDTag = FGameplayTag();
	CacheSourceContext.SourceName = EffectCacheHandle;
	return CacheSourceContext;
}
