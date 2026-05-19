#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagerTypes.h"
#include "LxTogglePanelUIManager.generated.h"

class ULxUIBaseObject;

/** 按键开关面板的运行时注册数据。 */
USTRUCT(BlueprintType, meta=(DisplayName="按键面板UI数据"))
struct FLxTogglePanelWidgetData
{
	GENERATED_BODY()

	/** 被管理的面板 UI。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="按键面板", DisplayName="UI控件")
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	/** 触发面板开关的输入行为。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="按键面板", DisplayName="输入行为ID")
	ELxInputActionID InputActionID = ELxInputActionID::None;

	/** 面板显示时是否显示鼠标。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="按键面板", DisplayName="显示时开启鼠标")
	bool bShowCursorWhenVisible = true;

	/** 打开该面板时是否关闭其他面板。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="按键面板", DisplayName="打开时关闭其他面板")
	bool bCloseOtherPanelsWhenOpened = false;

	/** 是否跟随角色数据刷新。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="按键面板", DisplayName="同步角色数据")
	bool bUpdateWithCharacterData = true;
};

/** 按键开关 UI 管理器，负责背包、属性、装备等由输入打开/关闭的面板。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="按键面板UI管理器")
class LXARPG_API ULxTogglePanelUIManager : public ULxUIManagementObject
{
	GENERATED_BODY()

public:
	/** 注册一个可由输入切换显示状态的面板 UI。 */
	UFUNCTION(BlueprintCallable, Category="按键面板", DisplayName="注册面板UI")
	void RegisterPanelWidget(ULxUIBaseObject* InWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true, bool bInCloseOtherPanelsWhenOpened = false, bool bInUpdateWithCharacterData = true);

	/** 设置指定面板 UI 的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="按键面板", DisplayName="设置面板显示状态")
	bool SetPanelVisible(ULxUIBaseObject* InWidget, bool bInVisible);

	/** 切换指定面板 UI 的显示状态。 */
	UFUNCTION(BlueprintCallable, Category="按键面板", DisplayName="切换面板显示状态")
	bool TogglePanelWidget(ULxUIBaseObject* InWidget);

	bool HandleInputValue(ELxInputActionID InInputActionID, const FLxInputValue& InValue);
	bool HasVisibleCursorPanel() const;

	virtual void RefreshManagedUI() override;
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	FLxTogglePanelWidgetData* FindPanelDataByWidget(const ULxUIBaseObject* InWidget);
	const FLxTogglePanelWidgetData* FindPanelDataByWidget(const ULxUIBaseObject* InWidget) const;
	void CloseOtherPanels(const ULxUIBaseObject* InKeepWidget);
	void UpdateInputRegistration(FLxTogglePanelWidgetData& InPanelData, ELxInputActionID InNewInputActionID);

private:
	/** 当前注册的按键面板列表。 */
	UPROPERTY(Transient)
	TArray<FLxTogglePanelWidgetData> PanelWidgets;

	/** 输入行为到面板 UI 的快速查询表。 */
	UPROPERTY(Transient)
	TMap<ELxInputActionID, TObjectPtr<ULxUIBaseObject>> InputActionToWidget;
};
