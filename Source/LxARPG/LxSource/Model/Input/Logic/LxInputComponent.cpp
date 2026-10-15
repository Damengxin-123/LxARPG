// Fill out your copyright notice in the Description page of Project Settings.


#include "LxInputComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerInput.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxPlayerSettingsSubsystem.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputActionConfig.h"


ULxInputComponent::ULxInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
		LxInputActionConfig::EnsureDefaultSprintInputActionInfo();
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

		UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(Parent->InputComponent);
		if (!EnhancedInput)
		{
			ERROR_TO_SCREEN("EnhancedInput is null!");
			return;
		}

		BoundInputComponent = EnhancedInput;
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
				InputBindingHandles.Add(EnhancedInput->BindAction(Action, ETriggerEvent::Triggered, this, &ULxInputComponent::HandleContinuousAction).GetHandle());
				break;
			case ELxInputInteractionType::PressAndRelease:
				InputBindingHandles.Add(EnhancedInput->BindAction(Action, ETriggerEvent::Started,
				this, &ULxInputComponent::HandlePressAndReleaseAction, ETriggerEvent::Started).GetHandle());
				InputBindingHandles.Add(EnhancedInput->BindAction(Action, ETriggerEvent::Completed,
				this, &ULxInputComponent::HandlePressAndReleaseAction, ETriggerEvent::Completed).GetHandle());
				InputBindingHandles.Add(EnhancedInput->BindAction(Action, ETriggerEvent::Canceled,
				this, &ULxInputComponent::HandlePressAndReleaseAction, ETriggerEvent::Canceled).GetHandle());
				break;
			case ELxInputInteractionType::SingleTrigger:
				InputBindingHandles.Add(EnhancedInput->BindAction(Action, ETriggerEvent::Started,
					this, &ULxInputComponent::HandleContinuousAction).GetHandle());
				break;
			}

		}

		if (UGameInstance* Game = Parent->GetGameInstance())
		{
			PlayerSettings = Game->GetSubsystem<ULxPlayerSettingsSubsystem>();
			if (PlayerSettings.IsValid())
			{
				PlayerSettings->EnsureLoaded();
				PlayerSettings->OnInputSettingsChanged.AddUObject(this, &ThisClass::RebuildInputMappings);
			}
		}
		bInputBindingsInitialized = true;
		RebuildInputMappings();
		Subsystem->AddMappingContext(m_pDefaultMappingContext, 0);
	}
}

void ULxInputComponent::HandleContinuousAction(const FInputActionInstance& Instance)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	ELxInputActionID ActionID = LxInputActionConfig::GetInputActionIDByAction(Action);

	FLxInputValue InputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
	LxInputActionConfig::SendInputEvent(ActionID, InputValue, Cast<APlayerController>(GetOwner()));
}

void ULxInputComponent::HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	ELxInputActionID ActionID = LxInputActionConfig::GetInputActionIDByAction(Action);

	FLxInputValue InputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
	InputValue.m_blValue = Trigge == ETriggerEvent::Started;
	if (InputValue.m_blValue) HeldInputActions.Add(ActionID);
	else HeldInputActions.Remove(ActionID);
	LxInputActionConfig::SendInputEvent(ActionID, InputValue, Cast<APlayerController>(GetOwner()));
}

void ULxInputComponent::RebuildInputMappings()
{
	if (!bInputBindingsInitialized || !m_pDefaultMappingContext) return;
	APlayerController* Parent = Cast<APlayerController>(GetOwner());
	ULocalPlayer* LocalPlayer = Parent ? Parent->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer
		? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;
	if (!Subsystem) return;

	// 修改映射前主动派发释放，避免业务状态依赖增强输入稍后清理的时序。
	ReleaseHeldInputActions();
	if (Parent->PlayerInput) Parent->PlayerInput->FlushPressedKeys();
	m_pDefaultMappingContext->UnmapAll();
	for (const TPair<ELxInputActionID, TObjectPtr<UInputAction>>& Pair : m_mapUserInputActionTable)
	{
		const FLxInputActionInfo* Info = LxInputActionConfig::GetInputActionInfo(Pair.Key);
		if (!Info) continue;
		const FKey Key = PlayerSettings.IsValid() ? PlayerSettings->GetEffectiveKey(Pair.Key) : Info->DefaultKey;
		if (!Key.IsValid()) continue;
		FEnhancedActionKeyMapping& Mapping = m_pDefaultMappingContext->MapKey(Pair.Value, Key);
		if (Info->ValueDirection == ELxInputValueAxial::X)
		{
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
			Scalar->Scalar = { Info->ValueMagnification, 0, 0 };
			Mapping.Modifiers.Add(Scalar);
		}
		else if (Info->ValueDirection == ELxInputValueAxial::Y)
		{
			UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(m_pDefaultMappingContext);
			Swizzle->Order = EInputAxisSwizzle::YXZ;
			UInputModifierScalar* Scalar = NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
			Scalar->Scalar = { Info->ValueMagnification, Info->ValueMagnification, 0 };
			Mapping.Modifiers = { Swizzle, Scalar };
		}
	}
	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Subsystem->RequestRebuildControlMappings(Options, EInputMappingRebuildType::Rebuild);
}

void ULxInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ReleaseHeldInputActions();
	if (PlayerSettings.IsValid()) PlayerSettings->OnInputSettingsChanged.RemoveAll(this);
	if (const APlayerController* Parent = Cast<APlayerController>(GetOwner()))
	{
		if (ULocalPlayer* LocalPlayer = Parent->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				Subsystem->RemoveMappingContext(m_pDefaultMappingContext);
			}
		}
	}
	if (BoundInputComponent.IsValid())
	{
		for (const uint32 Handle : InputBindingHandles) BoundInputComponent->RemoveBindingByHandle(Handle);
	}
	InputBindingHandles.Reset();
	m_mapUserInputActionTable.Reset();
	m_pDefaultMappingContext = nullptr;
	bInputBindingsInitialized = false;
	Super::EndPlay(EndPlayReason);
}

void ULxInputComponent::ReleaseHeldInputActions()
{
	const TSet<ELxInputActionID> PreviousHeldActions = MoveTemp(HeldInputActions);
	HeldInputActions.Reset();
	for (const ELxInputActionID ActionID : PreviousHeldActions)
	{
		FLxInputValue ReleasedValue(false);
		LxInputActionConfig::SendInputEvent(ActionID, ReleasedValue, Cast<APlayerController>(GetOwner()));
	}
}
