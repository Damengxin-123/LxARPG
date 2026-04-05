// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LxPlayerController.generated.h"


class ALxBaseCharacter;
class ULxInputComponent;
class ULxPlayerSystemOperateComponent;

/**
 * @class ALxPlayerController
 * @brief 玩家控制器类，负责管理和控制玩家角色。
 *
 * 该类继承自APlayerController，提供了创建、控制和同步玩家角色的功能。它支持在不同游戏模式下（如单机或客户端-服务器模式）创建和管理玩家角色，并处理与角色相关的各种事件。
 */
UCLASS(Blueprintable, DisplayName="玩家控制器")
class LXARPG_API ALxPlayerController : public APlayerController
{
	GENERATED_BODY()
public:

	ALxPlayerController();
	
	/**
	 * @brief 创建玩家角色。
	 *
	 * 该函数作为创建角色的总接口，负责根据当前的游戏环境（如单机或客户端-服务器模式）来实例化并初始化玩家角色。
	 * 它可能调用其他更具体的函数来完成角色的创建过程，确保角色能够在适当的游戏模式下正确地被生成和配置。
	 */
	UFUNCTION(BlueprintCallable, Category="创建角色", DisplayName="创建玩家角色")
	void CreatePlayerCharacter();

	// 显示鼠标光标
	/**
	 * @brief 显示鼠标光标并切换到游戏与界面混合输入模式。
	 */
	inline void ShowCursorFun();
	// 隐藏鼠标光标
	/**
	 * @brief 隐藏鼠标光标并切换回纯游戏输入模式。
	 */
	inline void HideCursorFun();
	
protected:
	/**
	 * @brief 玩家控制器开始运行时触发。
	 *
	 * 会初始化输入组件、系统操作组件以及本地玩家子系统引用。
	 */
	virtual void BeginPlay() override;

private:
	
	/**
 	 * @brief 当玩家控制器开始控制一个新的Pawn时调用。
 	 *
 	 * 此函数在每次玩家控制器接管一个新的Pawn对象（例如角色）的控制权时被调用。
 	 * 它可以用于初始化与新Pawn相关的设置，比如输入绑定、UI更新等。
 	 *
 	 * @param InPawn 即将被此玩家控制器所控制的新Pawn对象。
 	 */
	virtual void OnPossess(APawn* InPawn) override;

	/**
 	 * @brief 设置当前控制的Pawn。
 	 *
 	 * 该函数重写自APlayerController，用于设置当前控制的Pawn对象。
 	 *
 	 * @param InPawn 要设置的Pawn对象指针。
 	 */
	virtual void SetPawn(APawn* InPawn) override;

	/**
 	 * @brief 当玩家控制器不再控制任何Pawn时调用。
 	 *
 	 * 此函数在每次玩家控制器失去对Pawn对象（例如角色）的控制权时被调用。
 	 * 它可以用于清理与之前控制的Pawn相关的设置，比如解除输入绑定、更新UI等。
 	 */
	virtual void OnUnPossess() override;

	/**
 	 * @brief 同步控制的角色。
 	 *
 	 * 该方法用于同步当前控制的角色状态。
 	 *
 	 * @param InPawn 要同步的Pawn对象指针。
 	 */
	void SyncControlledCharacter(APawn* InPawn);

	/**
	 * @brief 创建本地玩家角色。
	 *
	 * 该函数专门用于在本地（单机或客户端）环境中创建并初始化玩家角色。它会根据当前的上下文环境设置必要的参数，确保角色能够正确地在游戏中生成和配置。
	 */
	void CreateLocalPlayerCharacter();

	/**
	 * @brief 在服务器上创建玩家角色。
	 *
	 * 该函数仅在服务器端调用，用于在服务器环境中创建并初始化玩家角色。通过标记为`Server`和`Reliable`，确保了该调用的可靠性和安全性，适用于需要在服务器端生成角色的场景，如网络游戏中的新玩家加入或角色重生等。
	 */
	UFUNCTION(Server, Reliable)
	void CreateServerPlayerCharacter();


	UPROPERTY(Blueprintable, EditAnywhere, Category="组件|输入组件",DisplayName="用户输入组件")
	TObjectPtr<ULxInputComponent> m_pInputComponent;

	UPROPERTY(Blueprintable, EditAnywhere, Category="组件|系统组件",DisplayName="系统操作组件")
	TObjectPtr<ULxPlayerSystemOperateComponent> m_pSystemOperateComponent;


	
private:
	/**
	 * @brief 当前控制的角色指针。
	 *
	 * 该变量存储了当前由玩家控制器所控制的Pawn对象（通常是角色）的指针。它用于在游戏逻辑中快速访问和操作当前控制的角色，例如更新角色状态、处理输入等。
	 */
	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pCurrentCharacter;
	
};
