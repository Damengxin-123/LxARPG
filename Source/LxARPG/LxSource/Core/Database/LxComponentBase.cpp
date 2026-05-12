#include "LxComponentBase.h"

#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"

void ULxComponentBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterAllInputActionReceives();
	Super::EndPlay(EndPlayReason);
}

void ULxComponentBase::RegisterInputActionReceive(ELxInputActionID InInputActionID)
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

void ULxComponentBase::UnregisterInputActionReceive(ELxInputActionID InInputActionID)
{
	if (InInputActionID == ELxInputActionID::None)
	{
		return;
	}

	LxInputActionConfig::UnregisterInputReceive(InInputActionID, this);
	RegisteredInputActionIDs.Remove(InInputActionID);
}

void ULxComponentBase::UnregisterAllInputActionReceives()
{
	for (const ELxInputActionID InputActionID : RegisteredInputActionIDs)
	{
		LxInputActionConfig::UnregisterInputReceive(InputActionID, this);
	}

	RegisteredInputActionIDs.Reset();
}
