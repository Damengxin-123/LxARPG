// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "LxLocalPlayerSubsystem.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxInputComponent;
class ULxUIManager;

#define GET_LOCAL_PLAYER_SYSTEM() ULxLocalPlayerSubsystem::GetFromLocalPlayer(GetLocalPlayer())

UCLASS()
class LXARPG_API ULxLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化本地玩家子系统。
	 *
	 * 会在子系统创建时读取游戏设置中的 UI 管理器类型，并实例化对应的 UI 管理器对象。
	 *
	 * @param Collection 子系统集合，用于引擎初始化阶段的子系统注册与依赖管理。
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/**
	 * @brief 反初始化本地玩家子系统。
	 *
	 * 清理缓存的输入组件、控制器、角色和 UI 管理器引用。
	 */
	virtual void Deinitialize() override;

	/**
	 * @brief 通过本地玩家对象获取本地玩家子系统实例。
	 *
	 * @param LocalPlayer 目标本地玩家对象。
	 * @return 若 LocalPlayer 有效则返回对应的子系统实例，否则返回 nullptr。
	 */
	static ULxLocalPlayerSubsystem* GetFromLocalPlayer(const ULocalPlayer* LocalPlayer);

	/**
	 * @brief 注册输入行为监听对象。
	 *
	 * 最终会转发给输入组件保存，由指定输入行为 ID 驱动对应监听对象接收事件。
	 *
	 * @param InInputName 需要监听的输入行为名称。
	 * @param InRegisterObj 处理该输入行为的监听对象接口。
	 */
	void RegisterInputReceive(FName InInputName, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);

	/**
	 * @brief 取消指定输入行为的监听对象注册。
	 *
	 * @param InInputName 需要取消注册的输入行为名称。
	 */
	void UnregisterInputReceive(FName InInputName);

	/**
	 * @brief 设置当前玩家控制器持有的输入组件引用。
	 *
	 * 子系统保存该引用后，可为其他系统注册输入监听；若 UI 管理器已存在，还会尝试补注册其输入监听。
	 *
	 * @param InUInputComponentQuote 玩家控制器上的输入组件实例。
	 */
	void SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote);

	/**
	 * @brief 设置当前本地玩家对应的控制器引用。
	 *
	 * 同步后会把控制器转交给 UI 管理器，用于创建和驱动界面对象。
	 *
	 * @param InPlayerController 当前本地玩家控制器；为空时表示控制器不可用。
	 */
	void SetPlayerControllerQuote(ALxPlayerController* InPlayerController);

	/**
	 * @brief 设置当前控制器正在持有的角色。
	 *
	 * 同步后会把角色转交给 UI 管理器，从而刷新背包和属性界面的展示数据。
	 *
	 * @param InCharacter 当前被控制的角色；为空时表示当前没有受控角色。
	 */
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	/**
	 * @brief 判断输入组件引用是否已经准备完成。
	 *
	 * @return 若子系统已持有有效的输入组件引用则返回 true，否则返回 false。
	 */
	bool HasInputComponentQuote() const { return m_pInputComponentQuote != nullptr; }

	/**
	 * @brief 获取当前本地玩家子系统持有的 UI 管理器。
	 *
	 * @return 当前 UI 管理器实例；若未配置或未创建则返回 nullptr。
	 */
	UFUNCTION(BlueprintCallable, Category="LocalPlayerSubsystem")
	ULxUIManager* GetUIManager() const { return m_pUIManager; }

private:
	UPROPERTY()
	TObjectPtr<ULxInputComponent> m_pInputComponentQuote = nullptr;

	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<ULxUIManager> m_pUIManager = nullptr;
};
