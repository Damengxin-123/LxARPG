#include "LxInputActionConfig.h"

#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"

namespace
{
	TMap<ELxInputActionID, FLxInputActionInfo> GInputActionInfoMap;
	TMap<ELxInputActionID, TWeakObjectPtr<UInputAction>> GInputActionObjectMap;
	TMap<const UInputAction*, ELxInputActionID> GInputActionIDByActionMap;
	TMap<ELxInputActionID, TArray<TScriptInterface<ILxInputReceiveInterface>>> GInputReceivedObjectMap;

	bool IsActorInInputScope(const AActor* InActor, const APlayerController* SourcePlayerController)
	{
		if (!InActor || !SourcePlayerController)
		{
			return false;
		}

		if (InActor == SourcePlayerController)
		{
			return true;
		}

		if (const APawn* Pawn = Cast<APawn>(InActor))
		{
			return Pawn == SourcePlayerController->GetPawn() || Pawn->GetController() == SourcePlayerController;
		}

		return false;
	}

	bool IsObjectInInputScope(const UObject* InObject, const APlayerController* SourcePlayerController)
	{
		if (!InObject || !SourcePlayerController)
		{
			return false;
		}

		if (InObject == SourcePlayerController)
		{
			return true;
		}

		if (const UActorComponent* ActorComponent = Cast<UActorComponent>(InObject))
		{
			return IsActorInInputScope(ActorComponent->GetOwner(), SourcePlayerController);
		}

		if (const AActor* Actor = Cast<AActor>(InObject))
		{
			return IsActorInInputScope(Actor, SourcePlayerController);
		}

		if (const UUserWidget* UserWidget = Cast<UUserWidget>(InObject))
		{
			return UserWidget->GetOwningPlayer() == SourcePlayerController;
		}

		for (const UObject* Outer = InObject->GetOuter(); Outer; Outer = Outer->GetOuter())
		{
			if (IsObjectInInputScope(Outer, SourcePlayerController))
			{
				return true;
			}
		}

		return false;
	}
}

namespace LxInputActionConfig
{
	void ClearInputActionConfig()
	{
		GInputActionInfoMap.Empty();
		// Runtime bindings can belong to other PIE worlds in the same process.
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

	void SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InInputValue, const APlayerController* SourcePlayerController)
	{
		if (TArray<TScriptInterface<ILxInputReceiveInterface>>* ReceivedObjects = GInputReceivedObjectMap.Find(InInputActionID))
		{
			ReceivedObjects->RemoveAll([](const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject)
			{
				return ReceivedObject.GetObject() == nullptr;
			});

			// 输入回调中可能注册或反注册监听者，例如交互 UI 显隐时会调整输入监听。
			// 这里使用快照遍历，避免回调期间修改原数组触发 ranged-for 的数组变化断言。
			const TArray<TScriptInterface<ILxInputReceiveInterface>> ReceivedObjectSnapshot = *ReceivedObjects;
			for (const TScriptInterface<ILxInputReceiveInterface>& ReceivedObject : ReceivedObjectSnapshot)
			{
				if (ReceivedObject && IsObjectInInputScope(ReceivedObject.GetObject(), SourcePlayerController))
				{
					ReceivedObject->HandleInputValue(InInputActionID, InInputValue);
				}
			}
		}
	}
}
