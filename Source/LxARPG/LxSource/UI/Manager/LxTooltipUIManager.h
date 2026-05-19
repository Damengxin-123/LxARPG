#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxTooltipUIManager.generated.h"

class ULxItemBase;
class ULxItemTooltipWidget;
class ULxUIBaseObject;

/** 悬浮提示 UI 管理器，负责物品提示等跟随鼠标并进行视口边界修正的界面。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="悬浮提示UI管理器")
class LXARPG_API ULxTooltipUIManager : public ULxUIManagementObject
{
	GENERATED_BODY()

public:
	/** 设置物品提示 UI 控件。 */
	UFUNCTION(BlueprintCallable, Category="悬浮提示UI", DisplayName="设置物品提示UI")
	void SetItemTooltipWidget(ULxItemTooltipWidget* InItemTooltipWidget, bool bInShowCursorWhenVisible = true);

	/** 显示指定物品的提示信息。 */
	UFUNCTION(BlueprintCallable, Category="悬浮提示UI", DisplayName="显示物品提示")
	bool ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition);

	UFUNCTION(BlueprintCallable, Category="Tooltip UI", DisplayName="Show Item Tooltip With Value")
	bool ShowItemTooltipWithValue(ULxItemBase* InItem, int32 InItemValue, bool bInShowItemValue, FVector2D InMouseScreenPosition);

	/** 根据鼠标屏幕位置更新物品提示位置。 */
	UFUNCTION(BlueprintCallable, Category="悬浮提示UI", DisplayName="更新物品提示位置")
	void UpdateItemTooltipPosition(FVector2D InMouseScreenPosition);

	/** 隐藏当前物品提示。 */
	UFUNCTION(BlueprintCallable, Category="悬浮提示UI", DisplayName="隐藏物品提示")
	void HideItemTooltip();

	bool HasVisibleTooltip() const;
	bool ShouldShowCursorForTooltip() const;

	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	void UpdateTooltipPosition(ULxUIBaseObject* InWidget, FVector2D InAnchorScreenPosition);

private:
	/** 当前用于显示物品信息的提示 UI。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxItemTooltipWidget> ItemTooltipWidget = nullptr;

	/** 提示 UI 可见时是否要求主 UI 管理器显示鼠标。 */
	UPROPERTY(Transient)
	bool bShowCursorWhenVisible = true;
};
