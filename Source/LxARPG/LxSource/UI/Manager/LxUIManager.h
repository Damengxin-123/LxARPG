#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxUIManagerEnum.h"
#include "LxUIManager.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxLocalPlayerSubsystem;
class ULxUIFunctionBase;
class ULxCharacterPopupUIFunction;

/**
 * @brief UI管理器中的子UI注册数据。
 *
 * 用于保存UI对象、对应输入行为ID以及所属功能类型。
 */
USTRUCT(BlueprintType, DisplayName="已注册子UI数据")
struct FLxManagedUIWidgetData
{
	GENERATED_BODY()

	/** 已注册的UI对象指针。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="子UI对象")
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 控制该UI的输入行为ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="输入行为ID")
	FName InputActionID = NAME_None;

	/** 该UI所属的功能类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="UI功能类型")
	ELxUIFunctionType UIType = ELxUIFunctionType::CharacterFunction;
};

/**
 * @brief 顶层UI管理器。
 *
 * 负责统一管理各个UI功能对象，保留UI注册与输入接收入口，
 * 再将逻辑转发给按ELxUIFunctionType拆分后的子功能对象。
 */
UCLASS(Blueprintable, BlueprintType, DisplayName="UI管理器")
class LXARPG_API ULxUIManager : public ULxUIBaseObject, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	/** 初始化UI管理器。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="初始化UI管理器")
	void InitializeManager(ULxLocalPlayerSubsystem* InLocalPlayerSubsystem);

	/** 设置当前玩家控制器。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置玩家控制器")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	/** 设置当前受控角色。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置受控角色")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	/** 刷新全部已注册功能对象和UI状态。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="刷新UI")
	void RefreshUI();

	/** 注册子UI，并按功能类型转交给对应子功能对象管理。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="注册子UI界面")
	void RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID, ELxUIFunctionType InUIType);

	/** 通知指定子UI显隐状态已发生变化。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="通知子UI状态变化")
	void NotifyChildUIVisibilityChanged(ULxUIBaseObject* InChildUIWidget);

	/** 设置指定子UI的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置子UI显示状态")
	void SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible);

	/** 切换指定子UI的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="切换子UI显示状态")
	void ToggleChildUI(ULxUIBaseObject* InChildUIWidget);

	/** 获取当前全部已注册子UI数据。 */
	UFUNCTION(BlueprintPure, Category="UIManager", DisplayName="获取已注册子UI列表")
	TArray<FLxManagedUIWidgetData> GetRegisteredChildWidgets() const { return RegisteredChildWidgets; }

	/** 获取角色弹窗功能对象。 */
	UFUNCTION(BlueprintPure, Category="UIManager", DisplayName="获取角色弹窗功能对象")
	ULxCharacterPopupUIFunction* GetCharacterPopupUIFunction() const;

	/** 处理输入系统转发给UI管理器的输入事件。 */
	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;

	/** 初始化UI管理器需要监听的输入。 */
	virtual void InitMonitorRegistration() override;

private:
	/** 根据UI对象查找其对应的注册数据。 */
	FLxManagedUIWidgetData* FindManagedUIDataByWidget(ULxUIBaseObject* InChildUIWidget);

	/** 按功能类型获取或创建对应的子功能对象。 */
	ULxUIFunctionBase* GetOrCreateUIFunction(ELxUIFunctionType InUIType);

	/** 初始化全部基础功能类型对象。 */
	void InitializeFunctionObjects();

	/** 向本地玩家子系统注册指定输入行为监听。 */
	void RegisterInputAction(FName InInputActionID);

	/** 汇总全部子功能对象的鼠标需求并更新鼠标状态。 */
	void UpdateCursorState() const;

private:
	/** 本地玩家子系统引用。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="本地玩家子系统", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem = nullptr;

	/** 当前受控角色引用。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="受控角色", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	/** 当前已注册的子UI数据列表。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="已注册子UI列表", meta=(AllowPrivateAccess="true"))
	TArray<FLxManagedUIWidgetData> RegisteredChildWidgets;

	/** 输入行为ID到子功能对象的映射表。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="输入行为映射表", meta=(AllowPrivateAccess="true"))
	TMap<FName, TObjectPtr<ULxUIFunctionBase>> m_mapInputActionToFunction;

	/** UI功能类型到子功能对象的映射表。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="功能类型映射表", meta=(AllowPrivateAccess="true"))
	TMap<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>> m_mapUITypeToFunction;

	/** 已注册到输入系统的输入行为ID集合。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="已注册输入行为ID集合", meta=(AllowPrivateAccess="true"))
	TSet<FName> RegisteredInputActionIDs;
};
