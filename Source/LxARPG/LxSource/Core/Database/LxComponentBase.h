// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxComponentBase.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnComponentDataChange);

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

	/**
	 * @brief用于响应组件数据变化的委托。
	 *
	 * 当组件的数据发生变化时，可以通过这个委托来通知监听者。派生类可以绑定自定义的处理函数到此委托上，以便在数据变更时执行特定的操作。
	 *
	 * @note 绑定到此委托上的函数应该能够处理组件数据的变化，并根据需要更新UI、状态或其他相关逻辑。
	 */
	FOnComponentDataChange  OnDataChange;

protected:
	/**
	 * @brief 组件注册到宿主对象时触发。
	 *
	 * 用于在组件被引擎注册阶段执行基础准备逻辑。
	 */
	virtual void OnRegister() override{Super::OnRegister();};

	/**
	 * @brief 游戏开始时触发组件初始化流程。
	 *
	 * 适合执行依赖世界与宿主对象均已有效的运行时初始化。
	 */
	virtual void BeginPlay() override{Super::BeginPlay();};

public:
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override{};
};


