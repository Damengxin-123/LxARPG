#include "LxCharacterEffectModuleBase.h"

#include "LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxCharacterEffectModuleBase::InitializeModule(ULxCharacterEffectComponent* InEffectComponent)
{
	EffectComponent = InEffectComponent;
	CharacterOwner = InEffectComponent != nullptr
		? Cast<ALxBaseCharacter>(InEffectComponent->GetOwner())
		: nullptr;
	if (bModuleInitialized)
	{
		return;
	}

	bModuleInitialized = true;
	OnModuleInitialize();
}

void ULxCharacterEffectModuleBase::ShutdownModule()
{
	if (!bModuleInitialized)
	{
		return;
	}

	OnModuleShutdown();
	bModuleInitialized = false;
}

AActor* ULxCharacterEffectModuleBase::GetOwner() const
{
	return EffectComponent != nullptr ? EffectComponent->GetOwner() : nullptr;
}

UWorld* ULxCharacterEffectModuleBase::GetWorld() const
{
	return EffectComponent != nullptr ? EffectComponent->GetWorld() : nullptr;
}
