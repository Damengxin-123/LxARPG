// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxComponentBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComponentDataChange);

/**
 * 所有自定义 ActorComponent 的基础类型。
 * 提供统一初始化入口、数据变化事件，以及输入监听注册辅助函数。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXARPG_API ULxComponentBase : public UActorComponent, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	ULxComponentBase() {};

	/** 子类可重写的组件初始化入口。 */
	virtual void BaseComponentInitialize() {};

	/** 组件数据变化时广播。 */
	FOnComponentDataChange OnDataChange;

	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override {};
	virtual void InitMonitorRegistration() override {};

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="注册输入行为监听")
	void RegisterInputActionReceive(ELxInputActionID InInputActionID);

	UFUNCTION(BlueprintCallable, Category="Input", DisplayName="取消输入行为监听")
	void UnregisterInputActionReceive(ELxInputActionID InInputActionID);

	void UnregisterAllInputActionReceives();

protected:
	virtual void OnRegister() override { Super::OnRegister(); };
	virtual void BeginPlay() override { Super::BeginPlay(); };
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override {};

private:
	TSet<ELxInputActionID> RegisteredInputActionIDs;
};
