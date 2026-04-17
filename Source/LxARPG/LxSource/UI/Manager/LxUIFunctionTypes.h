#pragma once

#include "CoreMinimal.h"
#include "LxUIFunctionBase.h"
#include "LxUIFunctionTypes.generated.h"

class ULxItemLogicBase;
class ULxItemTooltipWidget;

/** 主菜单相关UI功能类型。 */
UCLASS(DisplayName="主菜单UI功能")
class LXARPG_API ULxMainMenuUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	/** 判断主菜单功能当前是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const override;
};

/** 角色功能相关UI功能类型。 */
UCLASS(DisplayName="角色功能UI类型")
class LXARPG_API ULxCharacterFunctionUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	/** 判断角色功能当前是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const override;
};

/** 角色交互相关UI功能类型。 */
UCLASS(DisplayName="角色交互UI类型")
class LXARPG_API ULxCharacterInteractionUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	/** 判断角色交互功能当前是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const override;
};

/** 角色HUD相关UI功能类型。 */
UCLASS(DisplayName="角色HUD功能类型")
class LXARPG_API ULxCharacterHUDUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	/** 判断角色HUD功能当前是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const override;
};

/** 角色弹窗相关UI功能类型。 */
UCLASS(DisplayName="角色弹窗UI类型")
class LXARPG_API ULxCharacterPopupUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	/** 判断角色弹窗功能当前是否需要显示鼠标光标。 */
	virtual bool ShouldDisplayCursor() const override;

	/** 显示指定物品的悬浮信息。 */
	bool ShowItemTooltip(ULxItemLogicBase* InItemLogic, FVector2D InMouseScreenPosition);

	/** 更新物品悬浮窗位置。 */
	void UpdateItemTooltipPosition(FVector2D InMouseScreenPosition);

	/** 隐藏物品悬浮窗。 */
	void HideItemTooltip();

	virtual void AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID) override;

	virtual void RemoveManagedUIWidget(ULxUIBaseObject* InChildUIWidget) override;

	/** 获取当前缓存的物品悬浮信息控件。 */
	ULxItemTooltipWidget* GetItemTooltipWidget() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULxItemTooltipWidget> m_pItemTooltipWidget = nullptr;
};
