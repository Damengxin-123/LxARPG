#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxItemSplitWidget.generated.h"

class ULxItemOperationUIRouter;
class USpinBox;
class USlider;
class UButton;
class UTextBlock;

/** 数量选择窗口基类，布局由蓝图提供，数据操作仅通过路由器提交。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="物品拆分窗口"))
class LXARPG_API ULxItemSplitWidget : public ULxUIBaseObject
{
    GENERATED_BODY()
public:
    /** 绑定数量控件与按钮。 */
    virtual void NativeConstruct() override;
    /** 检查来源失效，避免确认过期数量。 */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    /** 使用逃脱键取消数量选择。 */
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    /** 初始化显示数据，不修改来源。 */
    void InitializeSplit(ULxItemOperationUIRouter* InRouter, const FText& ItemName, int32 TotalCount);
    /** 输入框与滑条共用的整数设置入口。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|拆分", meta=(DisplayName="设置物品拆分数量"))
    void SetSplitCount(int32 InCount);
    /** 将选择数量交给数据层操作入口。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|拆分", meta=(DisplayName="确认物品拆分"))
    void ConfirmSplit();
    /** 取消尚未提交的数量选择。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|拆分", meta=(DisplayName="取消物品拆分"))
    void CancelSplit();
    /** 显示失败原因并恢复输入。 */
    void ShowFailure(const FText& Reason);
    /** 显示等待状态，防止重复确认。 */
    void SetPending(bool bInPending);
    /** 当前待确认数量。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|拆分", meta=(DisplayName="拆分数量"))
    int32 SplitCount = 1;
    /** 允许拆出的最大整数数量。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|拆分", meta=(DisplayName="最大拆分数量"))
    int32 MaximumCount = 1;
    /** 通知蓝图扩展显示和失败表现。 */
    UFUNCTION(BlueprintImplementableEvent, Category="物品操作|拆分", meta=(DisplayName="拆分窗口数据更新"))
    void OnSplitDataUpdated(int32 Count, int32 Maximum, bool bInPending, const FText& FailureReason);
protected:
    /** 支持输入和拖动的数量框。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="拆分数量输入框"))
    TObjectPtr<USpinBox> CountInput;
    /** 与数量框同步的滑条。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="拆分数量滑条"))
    TObjectPtr<USlider> CountSlider;
    /** 确认按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="确认拆分按钮"))
    TObjectPtr<UButton> ConfirmButton;
    /** 取消按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="取消拆分按钮"))
    TObjectPtr<UButton> CancelButton;
    /** 当前物品名称。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="拆分物品名称"))
    TObjectPtr<UTextBlock> ItemNameText;
    /** 失败与等待状态提示。 */
    UPROPERTY(BlueprintReadOnly, Category="物品操作|控件", meta=(BindWidgetOptional, DisplayName="拆分状态提示"))
    TObjectPtr<UTextBlock> StatusText;
private:
    /** 将数值控件通知转换为整数。 */
    UFUNCTION(meta=(DisplayName="处理拆分输入数量", Category="物品操作|拆分"))
    void HandleInputChanged(float InValue);
    /** 将滑条数值映射到合法整数。 */
    UFUNCTION(meta=(DisplayName="处理拆分滑条数量", Category="物品操作|拆分"))
    void HandleSliderChanged(float InValue);
    /** 同步控件与蓝图通知，避免回调递归。 */
    void RefreshControls();
    /** 当前操作路由器。 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ULxItemOperationUIRouter> Router;
    /** 当前状态提示。 */
    UPROPERTY(Transient)
    FText StatusMessage;
    /** 等待权威结果期间禁止重复确认。 */
    bool bPending = false;
    /** 同步控件期间禁止重复回调。 */
    bool bRefreshing = false;
};
