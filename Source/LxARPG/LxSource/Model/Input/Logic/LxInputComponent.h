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

	/**
	 * @brief 初始化输入组件。
	 *
	 * 会创建输入映射上下文、加载输入配置、绑定输入动作并接入增强输入系统。
	 */
	virtual void BaseComponentInitialize() override;
	
protected:
	/**
	 * @brief 游戏开始时触发输入组件生命周期。
	 */
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override{};

	UFUNCTION()
	/**
	 * @brief 处理持续触发类型的输入事件。
	 *
	 * @param Instance 当前输入动作实例，包含来源动作和输入值。
	 */
	void HandleContinuousAction(const FInputActionInstance& Instance);

	UFUNCTION()
	/**
	 * @brief 处理按下/释放类型的输入事件。
	 *
	 * @param Instance 当前输入动作实例。
	 * @param Trigge 当前触发阶段，通常为 Started 或 Completed。
	 */
	void HandlePressAndReleaseAction(const FInputActionInstance& Instance, ETriggerEvent Trigge);
	
	// 注册用户输入事件
	void RegisterInputReceive(FName InInputName, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);

	// 取消注册
	void UnregisterInputReceive(FName InInputName);

private:

	/**
	 * @brief 分发输入事件给已注册的监听对象。
	 *
	 * @param InInputActionID 输入行为 ID。
	 * @param InINputValue 输入行为对应的值。
	 */
	void SendInputEvent(FName InInputActionID, FLxInputValue& InINputValue);

	UPROPERTY()
	TObjectPtr<UInputMappingContext> m_pDefaultMappingContext;
	
	UPROPERTY()
	TMap<FName, TObjectPtr<UInputAction>> m_mapUserInputActionTable;
	
	// 输入监听表
	UPROPERTY()
	TMap<FName, TScriptInterface<ILxInputReceiveInterface>> m_mapInputReceivedObject;
};
