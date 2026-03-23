// Fill out your copyright notice in the Description page of Project Settings.


#include "LxInputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "LxARPG/LxSource/Core/Tools/LxString.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTable.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxDataTableTypeEnum.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"


// Sets default values for this component's properties
ULxInputComponent::ULxInputComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void ULxInputComponent::BaseComponentInitialize()
{
	if (!m_pDefaultMappingContext)
	{
		m_pDefaultMappingContext = NewObject<UInputMappingContext>(this);
	}
	if (m_pDefaultMappingContext)
	{
		ULxGameInstanceSubsystem*  GameInstanceSubsystem = ULxGameInstanceSubsystem::GetInstance(GetWorld());
		if (!GameInstanceSubsystem)
		{
			ERROR_TO_SCREEN("GameInstanceSubsystem is null!");
			return;
		}
		const ULxGameDataTablesManager* GameDataTablesManager = GameInstanceSubsystem->GetGameDataManager();
		if (!GameDataTablesManager)
		{
			ERROR_TO_SCREEN("GameDataTablesManager is null!");
			return;
		}
		ULxDataTable* InputDataTable = GameDataTablesManager->GetDataTables(ELxDataTableTypeEnum::InputActionInfo);
		if (!InputDataTable)
		{
			ERROR_TO_SCREEN("InputDataTable is null!");
			return;
		}
		APlayerController* Parent = Cast<APlayerController>(GetOwner());
		if (!Parent)
		{
			ERROR_TO_SCREEN("PlayerController is null!");
			return;
		}
		UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Parent->GetLocalPlayer());
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
		uint16 index = 0;
		for (InputDataTable->SetIteratorIndex(index);
			const FLxInputActionInfo* InputActionInfo = InputDataTable->GetIteratorData<FLxInputActionInfo>();
			InputDataTable->SetIteratorIndex( ++index))
		{
			Action = NewObject<UInputAction>(this, InputActionInfo->RowID);
			Action->ValueType = InputActionInfo->ValueType;

			m_mapUserInputActionTable.Add(InputActionInfo->RowID, Action);

			// 绑定输入接收函数
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
			// 根据输入设定轴向，选择是否进行轴向拌合
			if (InputActionInfo->ValueDirection == ELxInputValueAxial::X)
			{
				// Scalar：正方向
				UInputModifierScalar* Scalar =
					NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
				Scalar->Scalar = {InputActionInfo->ValueMagnification, 0, 0};

				Mapping.Modifiers.Add(Scalar);
			}
			else if (InputActionInfo->ValueDirection == ELxInputValueAxial::Y)
			{
				// 在虚幻5.4之后，使用此方式叠加轴向缩放
				// Swizzle：X → Y
				UInputModifierSwizzleAxis* Swizzle =
					NewObject<UInputModifierSwizzleAxis>(m_pDefaultMappingContext);
				Swizzle->Order = EInputAxisSwizzle::YXZ;

				// Scalar：正方向
				UInputModifierScalar* Scalar =
					NewObject<UInputModifierScalar>(m_pDefaultMappingContext);
				Scalar->Scalar = {InputActionInfo->ValueMagnification, InputActionInfo->ValueMagnification, 0};

				Mapping.Modifiers = { Swizzle, Scalar };
			}
		}
	}
}


// Called when the game starts
void ULxInputComponent::BeginPlay()
{
	Super::BeginPlay();
}


void ULxInputComponent::HandleContinuousAction(const FInputActionInstance& Instance)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	FName ActionID = Action->GetFName();

	FLxInputValue inputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
		
	// FLxString("输入事件触发，ActionID: {0}, Value: {1}, {2}").Arg(ActionID).Arg(inputValue.m_sVector2D.X)
	// .Arg(inputValue.m_sVector2D.Y).LogeToScreenLog(ELxLogeLevelType::Debug);
	SendInputEvent(ActionID, inputValue);
}

void ULxInputComponent::HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge)
{
	const UInputAction* Action = Instance.GetSourceAction();
	const FInputActionValue Value = Instance.GetValue();
	FName ActionID = Action->GetFName();

	FLxInputValue inputValue(Value.Get<bool>(), Value.Get<float>(), Value.Get<FVector2D>(), Value.Get<FVector>());
	inputValue.m_blValue = Trigge == ETriggerEvent::Started;
	// FLxString("输入事件触发，ActionID: {0}, Value: {1}").Arg(ActionID).Arg(inputValue.m_blValue).LogeToScreenLog(ELxLogeLevelType::Debug);
	SendInputEvent(ActionID, inputValue);
}

void ULxInputComponent::RegisterInputReceive(FName InInputName,
	TScriptInterface<ILxInputReceiveInterface> InRegisterObj)
{
	if (InRegisterObj)
	{
		if (m_mapInputReceivedObject.Contains(InInputName))
		{
			// ERROR_TO_SCREEN(FLxString(TEXT("重复的输入行为ID : {0}")).Arg(InInputName));
			return;
		}
		m_mapInputReceivedObject.Add(InInputName, InRegisterObj);
	}
}

void ULxInputComponent::UnregisterInputReceive(FName InInputName)
{

	if (!m_mapInputReceivedObject.Contains(InInputName))
	{
		// ERROR_TO_SCREEN(FLxString(TEXT("并没有已注册的输入行为 : {0}")).Arg(InInputName));
		return;
	}
	m_mapInputReceivedObject.Remove(InInputName);

}

void ULxInputComponent::SendInputEvent(FName InInputActionID, FLxInputValue& InINputValue)
{
	if (!m_mapInputReceivedObject.Contains(InInputActionID))
	{
		// ERROR_TO_SCREEN(FLxString(TEXT("没有模块对此输入行为进行监听 : {0}")).Arg(InInputActionID));
		return;
	}
	if (m_mapInputReceivedObject[InInputActionID] == nullptr)
	{
		// ERROR_TO_SCREEN(FLxString(TEXT("监听模块注册为空 : {0}")).Arg(InInputActionID));
		return;
	}
	m_mapInputReceivedObject[InInputActionID]->HandleInputValue(InInputActionID, InINputValue);
}

