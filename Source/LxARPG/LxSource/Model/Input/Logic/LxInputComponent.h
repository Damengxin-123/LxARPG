// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxInputComponent.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="输入组件")
class LXARPG_API ULxInputComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	ULxInputComponent();

	/** 初始化输入映射上下文、输入配置和增强输入绑定。 */
	virtual void BaseComponentInitialize() override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override {};

	/** 处理持续触发类型的输入事件。 */
	UFUNCTION()
	void HandleContinuousAction(const FInputActionInstance& Instance);

	/** 处理按下/释放类型的输入事件。 */
	UFUNCTION()
	void HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge);

	void RegisterInputReceive(ELxInputActionID InInputActionID, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);
	void UnregisterInputReceive(ELxInputActionID InInputActionID);
	void UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj);

private:
	void SendInputEvent(ELxInputActionID InInputActionID, FLxInputValue& InINputValue);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> m_pDefaultMappingContext;

	UPROPERTY()
	TMap<ELxInputActionID, TObjectPtr<UInputAction>> m_mapUserInputActionTable;
};
