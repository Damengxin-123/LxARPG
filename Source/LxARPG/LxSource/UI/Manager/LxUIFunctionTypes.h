#pragma once

#include "CoreMinimal.h"
#include "LxUIFunctionBase.h"
#include "LxUIFunctionTypes.generated.h"

class ULxItemGridWidget;
class ULxItemBase;
class ULxItemTooltipWidget;

UCLASS(DisplayName="主菜单UI功能")
class LXARPG_API ULxMainMenuUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	virtual bool ShouldDisplayCursor() const override;
};

UCLASS(DisplayName="角色功能UI类型")
class LXARPG_API ULxCharacterFunctionUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	virtual bool ShouldDisplayCursor() const override;
};

UCLASS(DisplayName="角色交互UI类型")
class LXARPG_API ULxCharacterInteractionUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	virtual bool ShouldDisplayCursor() const override;
};

UCLASS(DisplayName="角色HUD功能类型")
class LXARPG_API ULxCharacterHUDUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	virtual bool ShouldDisplayCursor() const override;

	virtual void SetControlledCharacter(ALxBaseCharacter* InControlledCharacter) override;

	virtual void AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID) override;

	virtual void HandlePlayerInputAction(FName InInputActionID, const FLxInputValue& InValue) override;

	bool BindShortcutInputAction(ULxItemGridWidget* InItemGridWidget, FName InInputActionID);

private:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<ULxItemGridWidget>> ShortcutInputToGridMap;
};

UCLASS(DisplayName="角色弹窗UI类型")
class LXARPG_API ULxCharacterPopupUIFunction : public ULxUIFunctionBase
{
	GENERATED_BODY()

public:
	virtual bool ShouldDisplayCursor() const override;

	bool ShowItemTooltip(ULxItemBase* InItem, FVector2D InMouseScreenPosition);

	void UpdateItemTooltipPosition(FVector2D InMouseScreenPosition);

	void HideItemTooltip();

	virtual void AddManagedUIWidget(ULxUIBaseObject* InChildUIWidget, FName InInputActionID) override;

	virtual void RemoveManagedUIWidget(ULxUIBaseObject* InChildUIWidget) override;

	ULxItemTooltipWidget* GetItemTooltipWidget() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ULxItemTooltipWidget> m_pItemTooltipWidget = nullptr;
};
