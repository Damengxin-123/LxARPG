// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxInputData.h"
#include "LxInputReceiveInterface.generated.h"

/**
 * @brief 接收游戏或应用程序输入的接口。
 *
 * 该接口设计用于任何需要处理输入事件的类。它作为一个蓝图，定义了当特定输入动作发生时（如按键、鼠标移动或触摸输入）应调用的方法。
 *
 * @note 在你的类中实现此接口，以便在整个项目中以一致的方式接收和处理输入。
 */
UINTERFACE(BlueprintType)
class ULxInputReceiveInterface : public UInterface
{
	GENERATED_BODY()
};
class ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	/**
 	 * @brief 处理输入值的纯虚函数。
 	 *
 	 * 该函数用于处理具有特定名称的输入事件，并接收一个浮点数值作为输入数据。子类需要实现此方法以定义如何响应这些输入事件。
 	 *
 	 * @param InName 输入事件的名称，用于标识不同的输入动作。
 	 * @param InValue 与输入事件关联的浮点数值，例如轴输入的位置或旋转角度。
 	 */
	virtual void HandleInputValue(FName InName, FLxInputValue InValue) = 0;

	/**
	 * @brief 初始化监控注册的纯虚函数。
	 *
	 * 该函数用于初始化与输入监控相关的注册过程。子类必须实现此方法，以确保正确设置并注册所有必要的监听器或处理程序，以便能够接收和响应后续的输入事件。
	 *
	 * @note 实现此接口时，请确保在此方法中完成所有必需的初始化步骤，如注册输入设备、配置监听器等，从而为后续的输入处理做好准备。
	 */
	virtual void InitMonitorRegistration() = 0;
};

