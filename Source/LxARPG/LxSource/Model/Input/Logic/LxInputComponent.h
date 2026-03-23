// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"

#include "LxInputComponent.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXARPG_API ULxInputComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULxInputComponent();

	virtual void BaseComponentInitialize() override;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override{};

	UFUNCTION()
	void HandleContinuousAction(const FInputActionInstance& Instance);

	UFUNCTION()
	void HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge);
	
	// 注册用户输入事件
	void RegisterInputReceive(FName InInputName, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);

	// 取消注册
	void UnregisterInputReceive(FName InInputName);

private:

	void SendInputEvent(FName InInputActionID, FLxInputValue& InINputValue);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> m_pDefaultMappingContext;
	
	UPROPERTY()
	TMap<FName, TObjectPtr<UInputAction>> m_mapUserInputActionTable;
	
	// 输入监听表
	UPROPERTY()
	TMap<FName, TScriptInterface<ILxInputReceiveInterface>> m_mapInputReceivedObject;
};
