#include "LxAIControlModuleBase.h"

#include "LxAIControlComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxAICharacter.h"

void ULxAIControlModuleBase::InitializeModule(ULxAIControlComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void ULxAIControlModuleBase::ShutdownModule()
{
	OwnerComponent = nullptr;
}

ALxAICharacter* ULxAIControlModuleBase::GetAICharacter() const
{
	return OwnerComponent ? Cast<ALxAICharacter>(OwnerComponent->GetOwner()) : nullptr;
}

AActor* ULxAIControlModuleBase::GetOwner() const
{
	return OwnerComponent ? OwnerComponent->GetOwner() : nullptr;
}

UWorld* ULxAIControlModuleBase::GetWorld() const
{
	return OwnerComponent ? OwnerComponent->GetWorld() : nullptr;
}
