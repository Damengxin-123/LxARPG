// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "Blueprint/UserWidget.h"
#include "LxUIBaseObject.generated.h"

class ULxCharacterDataTransferComponent;

UCLASS()
class LXARPG_API ULxUIBaseObject : public UUserWidget, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	virtual void UpdateUIComponents(ULxCharacterDataTransferComponent* CharacterDataTransferComponent);
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	virtual bool HandleInputEvent(ELxInputActionID InputID, const FLxInputValue& Value){ return false; };
	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;
	virtual void InitMonitorRegistration() override;

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="注册行为监听")
	void RegisterInputActionReceive(ELxInputActionID InInputActionID);

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="取消注册行为监听")
	void UnregisterInputActionReceive(ELxInputActionID InInputActionID);

	void UnregisterAllInputActionReceives();

public:
	UPROPERTY()
	TObjectPtr<ULxCharacterDataTransferComponent> m_pCharacterDataTransferComponent = nullptr;

private:
	TSet<ELxInputActionID> RegisteredInputActionIDs;
};
