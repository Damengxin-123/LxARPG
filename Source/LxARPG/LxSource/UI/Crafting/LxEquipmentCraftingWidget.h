#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxUIBaseObject.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemGridWidget.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxEquipmentCraftingTypes.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxEquipmentCraftingWidget.generated.h"

class UButton;
class UTextBlock;
class UImage;
class ULxItemManufacturingComponent;

/** 放大的打造物品格子：继承现有物品提示，拖入只选择玩家已拥有的装备。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="打造装备格子")
class LXARPG_API ULxCraftingGridWidget : public ULxItemGridWidget
{
    GENERATED_BODY()
public:
    /** 绑定制造组件并刷新格子引用及图标。 */
    void SetManufacturingComponent(ULxItemManufacturingComponent* Component);
    /** 绑定蓝图布局后刷新初始显示。 */
    virtual void NativeConstruct() override;
protected:
    /** 接收背包或装备栏的现有拖拽操作。 */
    virtual bool NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation) override;
    /** 打造显示格子不能触发装备使用。 */
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
    /** 鼠标抬起也不执行装备使用，防止右键更换装备。 */
    virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
    /** 不允许从只读引用格子创建移动物品的拖拽。 */
    virtual void NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) override;
    /** 蓝图布局中用于显示装备图标的图片。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="装备图标", meta=(BindWidgetOptional))
    TObjectPtr<UImage> EquipmentIcon;
    /** 空格子提示，放入装备后隐藏。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="放入提示", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> EmptyHint;
private:
    /** 当前玩家的制造组件 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="当前玩家的制造组件")
    TObjectPtr<ULxItemManufacturingComponent> Manufacturing;
};

/** 装备打造界面逻辑；布局和美术全部由子类控件蓝图提供。 */
UCLASS(BlueprintType, Blueprintable, DisplayName="装备打造界面")
class LXARPG_API ULxEquipmentCraftingWidget : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
    GENERATED_BODY()
public:
    /** 打造台仅保存玩家装备引用，不作为物品提取来源。 */
    virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override;
    /** 拒绝从只读打造格子提取物品。 */
    virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override;
    /** 委托制造模块验证背包装备，来源物品保持原槽位和完整实例。 */
    virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override;
    /** 快速链接使用已有装备选择逻辑，不调用移动或扣费接口。 */
    virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request,
        ULxUIBaseObject* SourceWindow, FText& OutReason) override;
    /** 展示链接结果并通知子类蓝图。 */
    virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override;
    /** 最近一次装备快捷链接结果。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|物品操作", DisplayName="最近物品操作结果")
    FLxItemOperationResult LastItemOperationResult;
    /** 蓝图接收装备链接成功及失败的提示。 */
    UFUNCTION(BlueprintImplementableEvent, Category="装备打造|物品操作", DisplayName="物品快捷操作完成")
    void OnItemOperationCompleted(const FLxItemOperationResult& Result);
    /** 设置玩家制造组件并监听报价和装备更新。 */
    UFUNCTION(BlueprintCallable, Category="装备打造", DisplayName="设置制造组件")
    void SetManufacturingComponent(ULxItemManufacturingComponent* Component);
    /** 向蓝图提供制造组件及选择状态。 */
    UFUNCTION(BlueprintPure, Category="装备打造", DisplayName="获取制造组件")
    ULxItemManufacturingComponent* GetManufacturingComponent() const { return Manufacturing; }
    /** 刷新原生绑定控件并通知子类蓝图。 */
    UFUNCTION(BlueprintCallable, Category="装备打造", DisplayName="刷新打造界面")
    void RefreshCrafting();
    /** 蓝图可覆盖表现与反馈，逻辑仍由制造组件执行。 */
    UFUNCTION(BlueprintImplementableEvent, Category="装备打造", DisplayName="打造显示更新")
    void OnCraftingDisplayUpdated();
    /** 子类蓝图接收每次操作结果，用于播放音效或动画。 */
    UFUNCTION(BlueprintImplementableEvent, Category="装备打造", DisplayName="打造操作完成")
    void OnCraftingCompleted(const FLxEquipmentCraftResult& Result);
    /** 构造时绑定按钮，不在C++生成布局。 */
    virtual void NativeConstruct() override;
    /** 关闭时解除事件和格子引用。 */
    virtual void NativeDestruct() override;
protected:
    /** 拖入装备的放大格子。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="装备格子", meta=(BindWidgetOptional))
    TObjectPtr<ULxCraftingGridWidget> EquipmentGrid;
    /** 强化操作按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="强化按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> EnhanceButton;
    /** 调整全部普通词条按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="调整按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> AdjustButton;
    /** 随机新增词条按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="增加按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> AddButton;
    /** 按顺序随机移除词条按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="移除按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> RemoveButton;
    /** 清空格子选择的按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="取下按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> ClearButton;
    /** 通过交互体系关闭页面的按钮。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="关闭按钮", meta=(BindWidgetOptional))
    TObjectPtr<UButton> CloseButton;
    /** 当前装备名称、稀有度及强化度。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="装备摘要", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> EquipmentSummary;
    /** 玩家持有金币。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="金币文本", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> GoldText;
    /** 显示基础和普通词条实际生效效果。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="词条文本", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> EntriesText;
    /** 操作结果提示。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="结果文本", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> ResultText;
    /** 四个操作的实时费用和概率。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="强化报价", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> EnhanceQuote;
    /** 每条独立调整比例的说明。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="调整报价", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> AdjustQuote;
    /** 新增词条的成功率与费用。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="增加报价", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> AddQuote;
    /** 随机移除的成功率与费用。 */
    UPROPERTY(BlueprintReadOnly, Category="装备打造|布局", DisplayName="移除报价", meta=(BindWidgetOptional))
    TObjectPtr<UTextBlock> RemoveQuote;
private:
    /** 当前制造组件 */
    UPROPERTY(Transient, VisibleInstanceOnly, Category="装备打造|运行时", DisplayName="当前制造组件")
    TObjectPtr<ULxItemManufacturingComponent> Manufacturing;
    /** 点击强化。 */
    UFUNCTION(Category="装备打造", DisplayName="点击强化")
    void HandleEnhance();
    /** 点击调整。 */
    UFUNCTION(Category="装备打造", DisplayName="点击调整")
    void HandleAdjust();
    /** 点击增加。 */
    UFUNCTION(Category="装备打造", DisplayName="点击增加")
    void HandleAdd();
    /** 点击移除。 */
    UFUNCTION(Category="装备打造", DisplayName="点击移除")
    void HandleRemove();
    /** 点击取下。 */
    UFUNCTION(Category="装备打造", DisplayName="点击取下")
    void HandleClear();
    /** 点击关闭，交给交互管理器结束会话。 */
    UFUNCTION(Category="装备打造", DisplayName="点击关闭")
    void HandleClose();
    /** 将制造结果转发给蓝图。 */
    UFUNCTION(Category="装备打造", DisplayName="接收打造结果")
    void HandleCompleted(const FLxEquipmentCraftResult& Result);
};
