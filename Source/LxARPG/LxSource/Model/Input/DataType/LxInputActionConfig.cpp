#include "LxInputActionConfig.h"

#include "InputAction.h"

namespace
{
	TMap<ELxInputActionID, FLxInputActionInfo> GInputActionInfoMap;
	TMap<ELxInputActionID, TWeakObjectPtr<UInputAction>> GInputActionObjectMap;
	TMap<const UInputAction*, ELxInputActionID> GInputActionIDByActionMap;
	TMap<ELxInputActionID, TArray<TScriptInterface<ILxInputReceiveInterface>>> GInputReceivedObjectMap;
}

namespace LxInputActionConfig
{
	void ClearInputActionConfig()
	{
		GInputActionInfoMap.Empty();
		GInputActionObjectMap.Empty();
		GInputActionIDByActionMap.Empty();
		GInputReceivedObjectMap.Empty();
	}

	void SetInputActionInfo(const FLxInputActionInfo& InInputActionInfo)
	{
		if (InInputActionInfo.InputActionID != ELxInputActionID::None)
		{
			GInputActionInfoMap.Add(InInputActionInfo.InputActionID, InInputActionInfo);
		}
	}

	const FLxInputActionInfo* GetInputActionInfo(ELxInputActionID InInputActionID)
	{
		return GInputActionInfoMap.Find(InInputActionID);
	}

	const TMap<ELxInputActionID, FLxInputActionInfo>& GetInputActionInfoMap()
	{
		return GInputActionInfoMap;
	}

	FName GetInputActionName(ELxInputActionID InInputActionID)
	{
		if (const UEnum* InputActionEnum = StaticEnum<ELxInputActionID>())
		{
			return FName(InputActionEnum->GetNameStringByValue(static_cast<int64>(InInputActionID)));
		}

		return NAME_None;
	}

	void SetInputActionObject(ELxInputActionID InInputActionID, UInputAction* InInputAction)
	{
		if (InInputActionID == ELxInputActionID::None || InInputAction == nullptr)
		{
			return;
		}

		GInputActionObjectMap.Add(InInputActionID, InInputAction);
		GInputActionIDByActionMap.Add(InInputAction, InInputActionID);
	}

	ELxInputActionID GetInputActionIDByAction(const UInputAction* InInputAction)
	{
		return InInputAction ? GInputActionIDByActionMap.FindRef(InInputAction) : ELxInputActionID::None;
	}

	void RegisterInputReceive(ELxInputActionID InInputActionID, TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
	{
		if (InInputActionID == ELxInputActionID::None || !InRegisterObj)
		{
			return;
		}

		TArray<TScriptInterface<ILxInputReceiveInterface>>& ReceivedObjects = GInputReceivedObjectMap.FindOrAdd(InInputActionID);
		for (const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject : ReceivedObjects)
		{
			if (ReceivedObject.GetObject() == InRegisterObj.GetObject())
			{
				return;
			}
		}

		ReceivedObjects.Add(InRegisterObj);
	}
void UnregisterInputReceive(ELxInputActionID InInputActionID)
	{
		GInputReceivedObjectMap.Remove(InInputActionID);
	}
void UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj)
	{
		if (InInputActionID == ELxInputActionID::None || InRegisterObj == nullptr)
		{
			return;
		}

		if (TArray<TScriptInterface<ILxInputReceiveInterface>>* ReceivedObjects = GInputReceivedObjectMap.Find(InInputActionID))
		{
			ReceivedObjects->RemoveAll([InRegisterObj](const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject)
			{
				return ReceivedObject.GetObject() == nullptr || ReceivedObject.GetObject() == InRegisterObj;
			});

			if (ReceivedObjects->IsEmpty())
			{
				GInputReceivedObjectMap.Remove(InInputActionID);
			}
		}
	}

	void SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InInputValue)
	{
		if (TArray<TScriptInterface<ILxInputReceiveInterface>>* ReceivedObjects = GInputReceivedObjectMap.Find(InInputActionID))
		{
			ReceivedObjects->RemoveAll([](const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject)
			{
				return ReceivedObject.GetObject() == nullptr;
			});

			for (const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject : *ReceivedObjects)
			{
				if (ReceivedObject)
				{
					ReceivedObject->HandleInputValue(InInputActionID, InInputValue);
				}
			}
		}
	}
}
