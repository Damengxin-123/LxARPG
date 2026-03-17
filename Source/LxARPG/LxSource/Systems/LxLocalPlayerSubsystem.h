// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "LxLocalPlayerSubsystem.generated.h"

class ULxInputComponent;

#define GET_LOCAL_PLAYER_SYSTEM() ULxLocalPlayerSubsystem::GetFromLocalPlayer(GetWorld()->GetFirstLocalPlayerFromController())


/**
 * 本地系统管理器
 */
UCLASS()
class LXARPG_API ULxLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()


public:

	// 获取本地系统
	static ULxLocalPlayerSubsystem* GetFromLocalPlayer(const ULocalPlayer* LocalPlayer);
	
	// 注册用户输入事件
	void RegisterInputReceive(FName InInputName, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);

	// 取消注册
	void UnregisterInputReceive(FName InInputName);

	// 设置控制器的用户输入组件引用
	void SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote);
private:
	// 控制器的用户输入组件引用
	UPROPERTY()
	TObjectPtr<ULxInputComponent> m_pInputComponentQuote;
};
