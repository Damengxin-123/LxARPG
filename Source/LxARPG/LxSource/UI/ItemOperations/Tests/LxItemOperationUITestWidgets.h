#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemSplitWidget.h"
#include "LxARPG/LxSource/Model/Item/DataType/Material/LxMaterial.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxItemOperationUITestWidgets.generated.h"

/** 隔离物品窗口，通过正式接口接收并调用数据层，不连接角色或存档。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="物品操作测试窗口", Category="自动化测试|物品操作"))
class ULxItemOperationUITestWindow : public ULxUIBaseObject, public ILxItemQuickTransferInterface
{
	GENERATED_BODY()
public:
	/** 本窗口隔离的真实槽位数据。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULxItemSlotData>> Slots;
	/** 本窗口代表的容器类型。 */
	ELxItemContainerKind ContainerKind = ELxItemContainerKind::Backpack;
	/** 最后一次安全提取凭据。 */
	UPROPERTY(Transient)
	TObjectPtr<ULxItemTransferTicket> LastTicket;
	/** 接收接口实际收到的来源窗口。 */
	UPROPERTY(Transient)
	TWeakObjectPtr<ULxUIBaseObject> LastSourceWindow;
	/** 接收接口实际收到的真实槽位索引。 */
	int32 LastSourceSlotIndex = INDEX_NONE;
	/** 接收接口的调用次数。 */
	int32 ReceiveCount = 0;
	/** 来源结果通知的调用次数。 */
	int32 ResultCount = 0;
	/** 最近一次失败提示。 */
	FText LastFailure;

	/** 返回隔离容器上下文，供路由固定来源。 */
	virtual bool GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const override
	{
		OutContainer = FLxItemContainerAddress();
		OutContainer.Kind = ContainerKind;
		return IsVisible();
	}
	/** 只允许从本窗口槽位提取，完全采用正式数据层凭据。 */
	virtual ULxItemTransferTicket* PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason) override
	{
		if (!Slots.Contains(SourceSlot)) return nullptr;
		LastTicket = LxSafeItemOperations::BeginSafeExtract(SourceSlot, OutReason);
		return LastTicket;
	}
	/** 数据层只读校验目标空间。 */
	virtual bool CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const override
	{
		return LxSafeItemOperations::CanAccept(Ticket, Slots, OutReason);
	}
	/** 测试接收端用真实原子放置验证窗口路由，同时记录来源。 */
	virtual bool ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, const FLxItemOperationRequest& Request,
		ULxUIBaseObject* SourceWindow, FText& OutReason) override
	{
		++ReceiveCount;
		LastSourceWindow = SourceWindow;
		LastSourceSlotIndex = Request.SourceSlotIndex;
		return LxSafeItemOperations::Accept(Ticket, Slots, OutReason);
	}
	/** 接收路由失败，不在窗口中修改任何物品。 */
	virtual void NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result) override
	{
		++ResultCount;
		LastFailure = Result.Message;
	}
};

/** 普通材料使用探针，识别错误抬起和取消路径是否使用了物品。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="物品使用测试探针", Category="自动化测试|物品操作"))
class ULxItemOperationUseProbe : public ULxMaterial
{
	GENERATED_BODY()
public:
	/** 已发生的一次性使用次数。 */
	int32 UseCount = 0;
	/** 已发生的真实抬起使用次数。 */
	int32 EndCount = 0;
	/** 记录使用，避免测试触发真实物品效果。 */
	virtual ELxItemUseState ItemUse() override { ++UseCount; return ELxItemUseState::ActivateEntry; }
	/** 保留普通物品在抬起时使用的语义。 */
	virtual ELxItemUseState ItemUseEnd() override { ++EndCount; return ItemUse(); }
};

/** 提供原生输入与滑条控件的拆分窗口，测试数量通知与整数夹取。 */
UCLASS(Transient, NotBlueprintable, meta=(DisplayName="拆分数量测试窗口", Category="自动化测试|物品操作"))
class ULxItemSplitUITestWidget : public ULxItemSplitWidget
{
	GENERATED_BODY()
public:
	/** 创建只存在于内存中的控件树，并由正常构造流程绑定控件事件。 */
	void BuildControls()
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("数量测试根"));
		WidgetTree->RootWidget = Root;
		CountInput = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("CountInput"));
		CountSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("CountSlider"));
		Root->AddChildToCanvas(CountInput);
		Root->AddChildToCanvas(CountSlider);
	}
	/** 获取真实数量输入控件。 */
	USpinBox* GetInputForTest() const { return CountInput; }
	/** 获取真实数量滑条控件。 */
	USlider* GetSliderForTest() const { return CountSlider; }
};
