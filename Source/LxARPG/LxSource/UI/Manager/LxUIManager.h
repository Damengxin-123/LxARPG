#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "UObject/Object.h"
#include "LxUIManager.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxAttributeWidget;
class ULxBackpackWidget;
class ULxLocalPlayerSubsystem;
class UUserWidget;

UCLASS(Blueprintable, BlueprintType, DisplayName="UI管理器")
class LXARPG_API ULxUIManager : public UObject, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	/**
	 * @brief 初始化 UI 管理器。
	 *
	 * 保存本地玩家子系统引用，并尝试创建界面对象、注册输入监听和刷新界面状态。
	 *
	 * @param InLocalPlayerSubsystem 当前本地玩家对应的子系统，用于注册输入和获取运行时上下文。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void InitializeManager(ULxLocalPlayerSubsystem* InLocalPlayerSubsystem);

	/**
	 * @brief 设置当前 UI 所属的玩家控制器。
	 *
	 * 当控制器可用后，管理器会基于该控制器创建界面实例，并将控制器引用同步到各个界面对象。
	 *
	 * @param InPlayerController 当前本地玩家控制器；为空时表示暂时没有可用控制器。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	/**
	 * @brief 设置当前由控制器持有的角色。
	 *
	 * 管理器会把该角色同步给背包界面和属性界面，用于刷新显示数据。
	 *
	 * @param InCharacter 当前被控制的角色；为空时表示清空界面关联角色。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	/**
	 * @brief 刷新管理器持有的所有界面。
	 *
	 * 该函数会确保界面已创建，并把当前控制器与角色重新同步到各个界面对象。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void RefreshUI();

	/**
	 * @brief 处理已注册的输入事件。
	 *
	 * 根据输入行为 ID 决定切换背包界面或角色属性界面，仅在按下触发时生效。
	 *
	 * @param InName 当前输入行为的名称。
	 * @param InValue 当前输入行为携带的输入值，通常使用其中的布尔值判断按下/释放。
	 */
	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;

	/**
	 * @brief 注册 UI 管理器需要监听的输入行为。
	 *
	 * 会向本地玩家子系统注册打开背包和打开角色属性两个输入监听。
	 */
	virtual void InitMonitorRegistration() override;

	/**
	 * @brief 切换背包界面的显示状态。
	 *
	 * 若背包界面尚未创建，会先尝试创建；切换后会同步更新鼠标显示状态。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void ToggleBackpackUI();

	/**
	 * @brief 切换角色属性界面的显示状态。
	 *
	 * 若属性界面尚未创建，会先尝试创建；切换后会同步更新鼠标显示状态。
	 */
	UFUNCTION(BlueprintCallable, Category="UIManager")
	void ToggleAttributeUI();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UIManager|WidgetClass", DisplayName="Backpack Widget Class")
	TSubclassOf<ULxBackpackWidget> BackpackWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UIManager|WidgetClass", DisplayName="Attribute Widget Class")
	TSubclassOf<ULxAttributeWidget> AttributeWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UIManager|Input", DisplayName="Open Backpack Input Action ID")
	FName OpenBackpackInputActionID = TEXT("背包");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UIManager|Input", DisplayName="Open Attribute Input Action ID")
	FName OpenCharacterInformationInputActionID = TEXT("角色属性");

private:
	/**
	 * @brief 创建管理器配置的界面实例。
	 *
	 * 使用当前玩家控制器和编辑器中配置的蓝图类创建背包与属性界面，并默认加入视口但隐藏。
	 */
	void CreateUIWidgets();

	/**
	 * @brief 将当前角色同步给所有已创建的界面对象。
	 *
	 * 背包界面和属性界面会基于该角色刷新各自的数据展示。
	 */
	void ApplyCharacterToWidgets();

	/**
	 * @brief 将当前玩家控制器同步给所有已创建的界面对象。
	 *
	 * 主要用于让界面持有控制器引用，便于后续 UI 与控制器交互。
	 */
	void ApplyControllerToWidgets();

	/**
	 * @brief 设置指定界面的可见性。
	 *
	 * @param InWidget 需要设置可见性的界面对象。
	 * @param bInVisible 为 true 时显示界面，为 false 时折叠隐藏界面。
	 */
	void SetWidgetVisible(UUserWidget* InWidget, bool bInVisible);

	/**
	 * @brief 判断指定界面当前是否可见。
	 *
	 * @param InWidget 需要检查的界面对象。
	 * @return 若界面存在且当前不是 Hidden/Collapsed，则返回 true；否则返回 false。
	 */
	bool IsWidgetVisible(const UUserWidget* InWidget) const;

	/**
	 * @brief 根据当前界面显示状态更新鼠标状态。
	 *
	 * 当任一 UI 界面打开时显示鼠标，否则恢复为纯游戏输入模式并隐藏鼠标。
	 */
	void UpdateCursorState() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxBackpackWidget> m_pBackpackWidget = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ULxAttributeWidget> m_pAttributeWidget = nullptr;

	bool m_bInputRegistered = false;
};
