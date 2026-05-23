// Fill out your copyright notice in the Description page of Project Settings.


#include "LxInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"


ULxInputComponent::ULxInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void ULxInputComponent::BaseComponentInitialize()
{
	if (bInputBindingsInitialized)
	{
		return;
	}

	if (!m_pDefaultMappingContext)
	{
		m_pDefaultMappingContext = NewObject<UInputMappingContext>(this);
	}
	if (m_pDefaultMappingContext)
	{
		const TMap<ELxInputActionID, FLxInputActionInfo>& InputActionInfoMap = LxInputActionConfig::GetInputActionInfoMap();
		if (InputActionInfoMap.IsEmpty())
		{
			ERROR_TO_SCREEN("InputActionInfoMap is empty!");
			return;
		}
		APlayerController* Parent = Cast<APlayerController>(GetOwner());
		if (!Parent)
		{
			ERROR_TO_SCREEN("PlayerController is null!");
			return;
		}
		ULocalPlayer* LocalPlayer = Parent->GetLocalPlayer();
		if (!LocalPlayer)
		{
			return;
		}
		UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if (!Subsystem)
		{
			ERROR_TO_SCREEN("Subsystem is null!");
			return;
		}
		Subsystem->AddMappingContext(m_pDefaultMappingContext, 0);

		UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Parent->InputComponent);
		if (!EnhancedInput)
		{
			ERROR_TO_SCREEN("EnhancedInput is null!");
			return;
		}

		UInputAction* Action = nullptr;
		for (const TPair<ELxInputActionID, FLxInputActionInfo>& InputActionInfoPair : InputActionInfoMap)
		{
			const FLxInputActionInfo* InputActionInfo = &InputActionInfoPair.Value;
			Action = NewObject<UInputAction>(this, LxInputActionConfig::GetInputActionName(InputActionInfo->InputActionID));
			Action->ValueType = InputActionInfo->ValueType;

			m_mapUserInputActionTable.Add(InputActionInfo->InputActionID, Action);
			LxInputActionConfig::SetInputActionObject(InputActionInfo->InputActionID, Action);

			switch (InputActionInfo->InteractionType)
			{
			case ELxInputInteractionType::Continuous:
				EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ULxInputComponent::HandleContinuousAction);
				break;
			case ELxInputInteractionType::PressAndRelease:
				EnhancedInput->BindAction(Action, ETriggerEvent::Started,
				this, &ULxInputComponent::HandlePressAndReleaseAction, ETriggerEvent::Started);
				EnhancedInput->BindAction(Action, ETriggerEvent::Completed,
				this, &ULxInputComponent::HandlePressAndReleaseAction, ETriggerEvent::Completed);
				break;
			case ELxInputInteractionType::SingleTrigger:
				EnhancedInput->BindAction(Action, ETriggerEvent::Started,
					this, &ULxInputComponent::HandleContinuousAction);
				break;
			}

			FEnhancedActionKeyMapping& Mapping = m_pDefaultMappingContext->MapKey(Action, InputActionInfo->DefaultKey);
			if (InputActionInfo->ValueDirection == ELxInputValueAxial::X)
			{
				UInputModifierScalar* Scalar =
					NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
				Scalar->Scalar = {InputActionInfo->ValueMagnification, 0, 0};

				Mapping.Modifiers.Add(Scalar);
			}
			else if (InputActionInfo->ValueDirection == ELxInputValueAxial::Y)
			{
				UInputModifierSwizzleAxis* Swizzle =
					NewObject<UInputModifierSwizzleAxis>(m_pDefaultMappingContext);
				Swizzle->Order = EInputAxisSwizzle::YXZ;

				UInputModifierScalar* Scalar =
					NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
				Scalar->Scalar = {InputActionInfo->ValueMagnification, InputActionInfo->ValueMagnification, 0};

				Mapping.Modifiers = { Swizzle, Scalar };
			}
		}

		bInputBindingsInitialized = true;
	}
}

void ULxInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

void ULxInputComponent::HandleContinuousAction(const FInputActionInstance& Instance)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	ELxInputActionID ActionID = LxInputActionConfig::GetInputActionIDByAction(Action);

	FLxInputValue InputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
	SendInputEvent(ActionID, InputValue, Cast<APlayerController>(GetOwner()));
}

void ULxInputComponent::HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	ELxInputActionID ActionID = LxInputActionConfig::GetInputActionIDByAction(Action);

	FLxInputValue InputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
	InputValue.m_blValue = Trigge == ETriggerEvent::Started;
	SendInputEvent(ActionID, InputValue, Cast<APlayerController>(GetOwner()));
}

void ULxInputComponent::RegisterInputReceive(ELxInputActionID InInputActionID,
	TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
{
	LxInputActionConfig::RegisterInputReceive(InInputActionID, InRegisterObj);
}

void ULxInputComponent::UnregisterInputReceive(ELxInputActionID InInputActionID)
{
	LxInputActionConfig::UnregisterInputReceive(InInputActionID);
}

void ULxInputComponent::UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj)
{
	LxInputActionConfig::UnregisterInputReceive(InInputActionID, InRegisterObj);
}

void ULxInputComponent::SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InInputValue, const APlayerController* SourcePlayerController)
{
	LxInputActionConfig::SendInputEvent(InInputActionID, InInputValue, SourcePlayerController);
}
