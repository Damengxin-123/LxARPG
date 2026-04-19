#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputReceiveInterface.h"
#include "LxUIManagerEnum.h"
#include "LxUIManager.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxLocalPlayerSubsystem;
class ULxCharacterHUDUIFunction;
class ULxUIFunctionBase;
class ULxCharacterPopupUIFunction;

USTRUCT(BlueprintType, DisplayName="已注册子UI数据")
struct FLxManagedUIWidgetData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="子UI对象")
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="输入行为ID")
	FName InputActionID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UIManager", DisplayName="UI功能类型")
	ELxUIFunctionType UIType = ELxUIFunctionType::CharacterFunction;
};

UCLASS(Blueprintable, BlueprintType, DisplayName="UI管理器")
class LXARPG_API ULxUIManager : public ULxUIBaseObject, public ILxInputReceiveInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="初始化UI管理器")
	void InitializeManager(ULxLocalPlayerSubsystem* InLocalPlayerSubsystem);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置玩家控制器")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置受控角色")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="刷新UI")
	void RefreshUI();

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="注册子UI界面")
	void RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID, ELxUIFunctionType InUIType);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="通知子UI状态变化")
	void NotifyChildUIVisibilityChanged(ULxUIBaseObject* InChildUIWidget);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置子UI显示状态")
	void SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="切换子UI显示状态")
	void ToggleChildUI(ULxUIBaseObject* InChildUIWidget);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="UIManager", DisplayName="更新子UI位置")
	void UpdateManagedUIPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InScreenPosition);

	UFUNCTION(BlueprintPure, Category="UIManager", DisplayName="获取已注册子UI列表")
	TArray<FLxManagedUIWidgetData> GetRegisteredChildWidgets() const { return RegisteredChildWidgets; }

	UFUNCTION(BlueprintPure, Category="UIManager", DisplayName="获取角色HUD功能对象")
	ULxCharacterHUDUIFunction* GetCharacterHUDUIFunction() const;

	UFUNCTION(BlueprintPure, Category="UIManager", DisplayName="获取角色弹窗功能对象")
	ULxCharacterPopupUIFunction* GetCharacterPopupUIFunction() const;

	void RegisterUIFunctionInputAction(ELxUIFunctionType InUIType, FName InInputActionID);

	virtual void HandleInputValue(FName InName, FLxInputValue InValue) override;

	virtual void InitMonitorRegistration() override;

private:
	FLxManagedUIWidgetData* FindManagedUIDataByWidget(ULxUIBaseObject* InChildUIWidget);

	ULxUIFunctionBase* GetOrCreateUIFunction(ELxUIFunctionType InUIType);

	void InitializeFunctionObjects();

	void RegisterInputAction(FName InInputActionID);

	void UpdateCursorState() const;

private:
	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="本地玩家子系统", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ULxLocalPlayerSubsystem> m_pLocalPlayerSubsystem = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="受控角色", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ALxBaseCharacter> m_pControlledCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="已注册子UI列表", meta=(AllowPrivateAccess="true"))
	TArray<FLxManagedUIWidgetData> RegisteredChildWidgets;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="输入行为映射表", meta=(AllowPrivateAccess="true"))
	TMap<FName, TObjectPtr<ULxUIFunctionBase>> m_mapInputActionToFunction;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="功能类型映射表", meta=(AllowPrivateAccess="true"))
	TMap<ELxUIFunctionType, TObjectPtr<ULxUIFunctionBase>> m_mapUITypeToFunction;

	UPROPERTY(Transient, BlueprintReadOnly, Category="UIManager", DisplayName="已注册输入行为ID集合", meta=(AllowPrivateAccess="true"))
	TSet<FName> RegisteredInputActionIDs;
};
