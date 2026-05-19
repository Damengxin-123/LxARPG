#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagementObject.h"
#include "LxPopupUIManager.generated.h"

class ULxUIBaseObject;

/** 弹窗 UI 管理器，负责确认框、提示框、系统弹窗等临时 UI 的显示与隐藏。 */
UCLASS(Blueprintable, BlueprintType, DisplayName="弹窗UI管理器")
class LXARPG_API ULxPopupUIManager : public ULxUIManagementObject
{
	GENERATED_BODY()

public:
	/** 注册弹窗 UI，可在注册时立即隐藏。 */
	UFUNCTION(BlueprintCallable, Category="弹窗UI", DisplayName="注册弹窗UI")
	void RegisterPopupWidget(ULxUIBaseObject* InWidget, bool bInHideOnRegister = true);

	/** 显示指定弹窗。 */
	UFUNCTION(BlueprintCallable, Category="弹窗UI", DisplayName="显示弹窗")
	bool ShowPopup(ULxUIBaseObject* InWidget);

	/** 隐藏指定弹窗。 */
	UFUNCTION(BlueprintCallable, Category="弹窗UI", DisplayName="隐藏弹窗")
	bool HidePopup(ULxUIBaseObject* InWidget);

	/** 隐藏该管理器持有的全部弹窗。 */
	UFUNCTION(BlueprintCallable, Category="弹窗UI", DisplayName="隐藏全部弹窗")
	void HideAllPopups();

	bool HasVisiblePopup() const;

	virtual void RefreshManagedUI() override;
	virtual bool ContainsWidget(const ULxUIBaseObject* InWidget) const override;

private:
	/** 当前注册的弹窗 UI 列表。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxUIBaseObject>> PopupWidgets;
};
