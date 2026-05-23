#pragma once

#include "CoreMinimal.h"
#include "LxInputData.h"
#include "LxInputReceiveInterface.h"

class UInputAction;
class APlayerController;

namespace LxInputActionConfig
{
	void ClearInputActionConfig();
	void SetInputActionInfo(const FLxInputActionInfo& InInputActionInfo);

	const FLxInputActionInfo* GetInputActionInfo(ELxInputActionID InInputActionID);
	const TMap<ELxInputActionID, FLxInputActionInfo>& GetInputActionInfoMap();

	FName GetInputActionName(ELxInputActionID InInputActionID);

	void SetInputActionObject(ELxInputActionID InInputActionID, UInputAction* InInputAction);
	ELxInputActionID GetInputActionIDByAction(const UInputAction* InInputAction);

	void RegisterInputReceive(ELxInputActionID InInputActionID, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);
	void UnregisterInputReceive(ELxInputActionID InInputActionID);
	void UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj);

	void SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InInputValue, const APlayerController* SourcePlayerController);
}
