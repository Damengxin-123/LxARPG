#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxGameStateTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "LxLocalPlayerSubsystem.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxInputComponent;
class ULxInteractionUIManager;
class ULxUIManager;

#define GET_LOCAL_PLAYER_SYSTEM() ULxLocalPlayerSubsystem::GetFromLocalPlayer(GetLocalPlayer())

/** 本地玩家子系统，负责缓存本地玩家相关的控制器、输入组件和 UI 管理器引用。 */
UCLASS(DisplayName="本地玩家子系统")
class LXARPG_API ULxLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static ULxLocalPlayerSubsystem* GetFromLocalPlayer(const ULocalPlayer* LocalPlayer);

	void RegisterInputReceive(ELxInputActionID InInputActionID, TScriptInterface<ILxInputReceiveInterface> InRegisterObj);
	void UnregisterInputReceive(ELxInputActionID InInputActionID);
	void UnregisterInputReceive(ELxInputActionID InInputActionID, const UObject* InRegisterObj);

	void SetInputComponentQuote(ULxInputComponent* InUInputComponentQuote);
	void SetPlayerControllerQuote(ALxPlayerController* InPlayerController);
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	bool HasInputComponentQuote() const { return m_pInputComponentQuote != nullptr; }

	/** 获取主 UI 管理器。 */
	UFUNCTION(BlueprintCallable, Category="本地玩家子系统", DisplayName="获取UI管理器")
	ULxUIManager* GetUIManager() const { return m_pUIManager; }

	/** 获取主 UI 管理器内部的交互 UI 子管理器。 */
	UFUNCTION(BlueprintCallable, Category="本地玩家子系统", DisplayName="获取交互UI管理器")
	ULxInteractionUIManager* GetInteractionUIManager() const;

	/** 获取本地玩家当前所处的游戏流程状态。 */
	UFUNCTION(BlueprintPure, Category="本地玩家子系统|游戏状态", DisplayName="获取游戏状态")
	ELxGameState GetGameState() const { return m_GameState; }

	/** 设置本地玩家当前所处的游戏流程状态；状态未变化时不会重复广播事件。 */
	UFUNCTION(BlueprintCallable, Category="本地玩家子系统|游戏状态", DisplayName="设置游戏状态")
	void SetGameState(ELxGameState InGameState);

	/** 查询当前状态是否允许使用角色相关功能与 UI。 */
	UFUNCTION(BlueprintPure, Category="本地玩家子系统|游戏状态", DisplayName="是否允许使用角色相关功能")
	bool IsCharacterFeatureAvailable() const;

	/** 游戏流程状态变化事件，依次提供变化前与变化后的状态。 */
	UPROPERTY(BlueprintAssignable, Category="本地玩家子系统|游戏状态", DisplayName="游戏状态变化事件")
	FOnLxGameStateChanged OnGameStateChanged;

private:
	/** 本地玩家当前所处的游戏流程状态。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintGetter=GetGameState, Category="本地玩家子系统|游戏状态",
		DisplayName="当前游戏状态")
	ELxGameState m_GameState = ELxGameState::OpeningGame;

	UPROPERTY()
	TObjectPtr<ULxInputComponent> m_pInputComponentQuote = nullptr;

	UPROPERTY()
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	UPROPERTY()
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<ULxUIManager> m_pUIManager = nullptr;
};
