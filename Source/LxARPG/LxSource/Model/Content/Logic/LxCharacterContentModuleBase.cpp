#include "LxCharacterContentModuleBase.h"

#include "LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxCharacterContentModuleBase::InitializeModule(ULxCharacterContentComponent* InContentComponent)
{
	ContentComponent = InContentComponent;
	CharacterOwner = InContentComponent != nullptr
		? Cast<ALxBaseCharacter>(InContentComponent->GetOwner())
		: nullptr;
	if (bModuleInitialized)
	{
		return;
	}

	bModuleInitialized = true;
	OnModuleInitialize();
}

void ULxCharacterContentModuleBase::ShutdownModule()
{
	if (!bModuleInitialized)
	{
		return;
	}

	OnModuleShutdown();
	bModuleInitialized = false;
}

AActor* ULxCharacterContentModuleBase::GetOwner() const
{
	return ContentComponent != nullptr ? ContentComponent->GetOwner() : nullptr;
}

UWorld* ULxCharacterContentModuleBase::GetWorld() const
{
	return ContentComponent != nullptr ? ContentComponent->GetWorld() : nullptr;
}
