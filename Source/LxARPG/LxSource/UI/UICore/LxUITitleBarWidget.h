#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LxUITitleBarWidget.generated.h"

class ULxUIBaseObject;

/** 通用界面标题栏控件，负责把拖动和关闭操作转发给目标 UI。 */
UCLASS(Blueprintable, BlueprintType, meta=(DisplayName="界面标题栏控件"))
class LXARPG_API ULxUITitleBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 设置标题栏控制的目标 UI 界面；传入空值后自动查找最近的父级窗口。 */
	UFUNCTION(BlueprintCallable, Category="界面标题栏|目标", DisplayName="设置目标界面")
	void SetTargetUIObject(ULxUIBaseObject* InTargetUIObject);

	/** 获取显式指定的目标 UI，未指定时沿控件层级查找最近的父级窗口。 */
	UFUNCTION(BlueprintPure, Category="界面标题栏|目标", DisplayName="获取目标界面")
	ULxUIBaseObject* GetTargetUIObject() const;

	/** 关闭目标 UI 界面，并通过目标界面内部逻辑同步鼠标显示状态。 */
	UFUNCTION(BlueprintCallable, Category="界面标题栏|显示", DisplayName="关闭目标界面")
	bool CloseTargetUI();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual void NativeDestruct() override;

protected:
	/** 是否允许标题栏拖动目标 UI。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="界面标题栏|拖动", meta=(DisplayName="是否允许拖动"))
	bool bEnableDrag = true;

	/** 当前是否正在通过标题栏拖动目标 UI。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="界面标题栏|拖动", meta=(DisplayName="是否正在拖动标题栏"))
	bool bIsDraggingTitleBar = false;

	/** 显式指定的目标 UI；为空时根据当前控件层级自动解析，不缓存父窗口。 */
	UPROPERTY(Transient, BlueprintReadOnly, Category="界面标题栏|目标", meta=(DisplayName="目标界面"))
	TObjectPtr<ULxUIBaseObject> TargetUIObject = nullptr;

private:
	/** 结束本次标题栏拖动，并清理拖动开始时记录的窗口引用。 */
	void EndTitleBarDrag();

	/** 本次拖动开始时的目标窗口，避免重新挂载或修改目标后拖动其他窗口。 */
	UPROPERTY(Transient, VisibleInstanceOnly, Category="界面标题栏|拖动", meta=(DisplayName="当前拖动界面"))
	TWeakObjectPtr<ULxUIBaseObject> DragTargetUIObject;
};
