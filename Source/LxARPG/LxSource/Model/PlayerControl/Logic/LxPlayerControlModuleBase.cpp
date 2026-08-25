#include "LxPlayerControlModuleBase.h"

#include "LxPlayerControlComponent.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

void ULxPlayerControlModuleBase::InitializeModule(ULxPlayerControlComponent* InOwnerComponent)
{
	OwnerComponent = InOwnerComponent;
}

void ULxPlayerControlModuleBase::ShutdownModule()
{
	UnregisterAllInputActionReceives();
	OwnerComponent = nullptr;
}

ALxPlayerCharacter* ULxPlayerControlModuleBase::GetPlayerCharacter() const
{
	return OwnerComponent ? Cast<ALxPlayerCharacter>(OwnerComponent->GetOwner()) : nullptr;
}

AActor* ULxPlayerControlModuleBase::GetOwner() const
{
	return OwnerComponent ? OwnerComponent->GetOwner() : nullptr;
}

UWorld* ULxPlayerControlModuleBase::GetWorld() const
{
	return OwnerComponent ? OwnerComponent->GetWorld() : nullptr;
}

void ULxPlayerControlModuleBase::RegisterInputActionReceive(const ELxInputActionID InInputActionID)
{
	if (InInputActionID == ELxInputActionID::None || RegisteredInputActionIDs.Contains(InInputActionID))
	{
		return;
	}

	TScriptInterface<ILxInputReceiveInterface> InputReceive;
	InputReceive.SetObject(this);
	InputReceive.SetInterface(Cast<ILxInputReceiveInterface>(this));
	LxInputActionConfig::RegisterInputReceive(InInputActionID, InputReceive);
	RegisteredInputActionIDs.Add(InInputActionID);
}

void ULxPlayerControlModuleBase::UnregisterAllInputActionReceives()
{
	for (const ELxInputActionID InputActionID : RegisteredInputActionIDs)
	{
		LxInputActionConfig::UnregisterInputReceive(InputActionID, this);
	}
	RegisteredInputActionIDs.Reset();
}

void ULxPlayerControlModuleBase::BroadcastModuleDataChanged() const
{
	if (OwnerComponent)
	{
		OwnerComponent->OnDataChange.Broadcast();
	}
}
