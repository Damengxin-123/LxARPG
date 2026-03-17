// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxComponentBase.generated.h"


/**
 * @class ULxComponentBase
 * @brief 基础组件类，继承自UActorComponent并实现了IGameFrameworkInitStateInterface接口。
 *
 * 该类为所有自定义组件提供了一个基础框架，包括初始化状态管理和基本的生命周期方法。派生类可以通过重写特定的方法来实现自定义行为。
 *
 * @note 该类被标记为蓝图可生成组件（BlueprintSpawnableComponent），意味着它可以在虚幻引擎的蓝图中使用和扩展。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXARPG_API ULxComponentBase : public UActorComponent
{
	GENERATED_BODY()

public:
	ULxComponentBase(){};
	/**
	 * @brief 初始化基础组件。
	 *
	 * 该方法用于初始化基础组件。应在派生类中重写此方法，以提供不同组件的具体初始化逻辑。
	 *
	 * @note 该方法应在组件的设置阶段调用，并且根据组件的生命周期可能被多次调用。
	 */
	virtual void BaseComponentInitialize() {};

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;

public:
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override{};
};


