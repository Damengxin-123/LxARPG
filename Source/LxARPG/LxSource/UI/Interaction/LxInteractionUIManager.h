#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxInteractionUIManager.generated.h"

class ALxPlayerCharacter;
class ULxDialogueInteractionWidget;
class ULxInteractionEntranceWidget;
class ULxPlayerInteractionComponent;
class ULxTreasureChestWidget;
class ULxWarehouseWidget;

/** 交互UI管理器，用于统一持有和初始化交互相关UI。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="交互UI管理器")
class LXARPG_API ULxInteractionUIManager : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	/** 设置玩家交互组件，并同步给已注册的交互UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家交互组件")
	void SetPlayerInteractionComponent(ULxPlayerInteractionComponent* InPlayerInteractionComponent);

	/** 从玩家角色身上读取玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="设置玩家角色")
	void SetPlayerCharacter(ALxPlayerCharacter* InPlayerCharacter);

	/** 注册交互入口UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册交互入口UI")
	void RegisterEntranceWidget(ULxInteractionEntranceWidget* InEntranceWidget);

	/** 注册对话交互UI。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册对话交互UI")
	void RegisterDialogueInteractionWidget(ULxDialogueInteractionWidget* InDialogueInteractionWidget);

	/** 注册仓库交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册仓库UI")
	void RegisterWarehouseWidget(ULxWarehouseWidget* InWarehouseWidget);

	/** 注册宝箱交互 UI，并同步当前玩家交互组件。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="注册宝箱UI")
	void RegisterTreasureChestWidget(ULxTreasureChestWidget* InTreasureChestWidget);

	/** 刷新所有交互UI持有的组件引用。 */
	UFUNCTION(BlueprintCallable, Category="交互UI", DisplayName="刷新交互UI")
	void RefreshInteractionUI();

private:
	UPROPERTY(Transient)
	TObjectPtr<ULxPlayerInteractionComponent> PlayerInteractionComponent = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxInteractionEntranceWidget> EntranceWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxDialogueInteractionWidget> DialogueInteractionWidget = nullptr;

	/** 仓库交互界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxWarehouseWidget> WarehouseWidget = nullptr;

	/** 宝箱交互界面。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxTreasureChestWidget> TreasureChestWidget = nullptr;
};
