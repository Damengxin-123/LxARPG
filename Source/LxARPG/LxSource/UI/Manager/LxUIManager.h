#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxUIManager.generated.h"

class ALxBaseCharacter;
class ALxPlayerController;
class ULxItemBase;
class ULxItemTooltipWidget;

USTRUCT()
struct FLxManagedUIWidgetData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<ULxUIBaseObject> UIWidget = nullptr;

	UPROPERTY()
	ELxInputActionID InputActionID = ELxInputActionID::None;

	UPROPERTY()
	bool bShowCursorWhenVisible = true;
};

UCLASS(Blueprintable, BlueprintType, DisplayName="UI管理器")
class LXARPG_API ULxUIManager : public ULxUIBaseObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置玩家控制器")
	void SetPlayerController(ALxPlayerController* InPlayerController);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置受控角色")
	void SetControlledCharacter(ALxBaseCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="刷新UI")
	void RefreshUI();

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="注册子UI界面")
	void RegisterChildUIWidget(ULxUIBaseObject* InChildUIWidget, ELxInputActionID InInputActionID, bool bInShowCursorWhenVisible = true);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="设置子UI显示状态")
	void SetChildUIVisible(ULxUIBaseObject* InChildUIWidget, bool bInVisible);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="切换子UI显示状态")
	void ToggleChildUI(ULxUIBaseObject* InChildUIWidget);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="UIManager", DisplayName="更新子UI位置")
	void UpdateManagedUIPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InScreenPosition);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="显示物品提示")
	bool ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="更新物品提示位置")
	void UpdateItemTooltipPosition(FVector2D InMouseScreenPosition);

	UFUNCTION(BlueprintCallable, Category="UIManager", DisplayName="隐藏物品提示")
	void HideItemTooltip();

	virtual void HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue) override;

private:
	FLxManagedUIWidgetData* FindManagedUIDataByWidget(ULxUIBaseObject* InChildUIWidget);
	bool IsManagedUIVisible(const ULxUIBaseObject* InChildUIWidget) const;
	void UpdateTooltipPosition(ULxUIBaseObject* InChildUIWidget, FVector2D InAnchorScreenPosition);
	void UpdateCursorState() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ALxPlayerController> m_pPlayerController = nullptr;

	UPROPERTY(Transient)
	TArray<FLxManagedUIWidgetData> RegisteredChildWidgets;

	UPROPERTY(Transient)
	TMap<ELxInputActionID, TObjectPtr<ULxUIBaseObject>> m_mapInputActionToWidget;

	UPROPERTY(Transient)
	TObjectPtr<ULxItemTooltipWidget> m_pItemTooltipWidget = nullptr;
};
