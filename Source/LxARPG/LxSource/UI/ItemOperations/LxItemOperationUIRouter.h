#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxARPG/LxSource/Model/Item/DataType/LxItemOperationTypes.h"
#include "LxItemOperationUIRouter.generated.h"

class ULxUIManager;
class ULxUIBaseObject;
class ULxItemSlotData;
class ULxItemTransferTicket;
class ULxItemSplitWidget;
class ALxPlayerController;

/** 每个本地玩家独立的物品快捷操作路由器，保管凭据并管理拆分窗口。 */
UCLASS(BlueprintType, meta=(DisplayName="物品快捷操作路由器"))
class LXARPG_API ULxItemOperationUIRouter : public UObject
{
    GENERATED_BODY()
public:
    /** 绑定当前主布局器，支持控件重建后恢复绑定。 */
    void Initialize(ULxUIManager* InManager);
    /** 取消本地凭据并解除通知，未提交操作不会改变物品。 */
    void Shutdown();
    /** 由来源窗口提取凭据，再向最上层其他窗口的统一接口发送。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|快速存取", meta=(DisplayName="请求物品快速存取"))
    bool RequestQuickTransfer(ULxUIBaseObject* SourceWindow, ULxItemSlotData* SourceSlot);
    /** 打开数量选择窗口，仅确认后请求独立空格拆分。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|拆分", meta=(DisplayName="打开物品拆分窗口"))
    bool OpenSplitWindow(ULxUIBaseObject* SourceWindow, ULxItemSlotData* SourceSlot);
    /** 目标窗口使用的通用请求入口，不在窗口中修改物品。 */
    bool SubmitOperation(const FLxItemOperationRequest& Request, ULxItemTransferTicket* Ticket, ULxUIBaseObject* SourceWindow, FText& OutReason);
    /** 使用打开窗口时固定的来源记录提交拆分数量。 */
    bool ConfirmSplit(int32 Count);
    /** 关闭数量窗口，不执行物品修改。 */
    UFUNCTION(BlueprintCallable, Category="物品操作|拆分", meta=(DisplayName="取消物品拆分"))
    void CancelSplit();
    /** 检查拆分来源是否仍然有效。 */
    bool IsSplitSourceValid() const;
    /** 避免同一来源槽位连点重复提交。 */
    bool HasPendingSource(const FLxItemContainerAddress& Source, int32 SlotIndex) const;
    /** 可替换的拆分窗口蓝图，默认指向项目中文布局资产。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="物品操作|拆分", meta=(DisplayName="物品拆分窗口类型"))
    TSoftClassPtr<ULxItemSplitWidget> SplitWidgetClass;
private:
    /** 订阅当前玩家控制器的权威完成事件。 */
    bool EnsureController();
    /** 将本地拒绝交给来源窗口的统一结果接口。 */
    void ReportFailure(ULxUIBaseObject* SourceWindow, ELxItemOperationKind Kind, const FText& Reason);
    /** 终结凭据并把权威结果交给来源窗口。 */
    void HandleOperationCompleted(const FLxItemOperationResult& Result);
    /** 当前主布局器。 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ULxUIManager> Manager;
    /** 当前结果通知发送者。 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ALxPlayerController> Controller;
    /** 待完成请求的凭据，保留强引用防止提前回收。 */
    UPROPERTY(Transient)
    TMap<FGuid, TObjectPtr<ULxItemTransferTicket>> PendingTickets;
    /** 待完成请求的来源窗口。 */
    UPROPERTY(Transient)
    TMap<FGuid, TWeakObjectPtr<ULxUIBaseObject>> PendingWindows;
    /** 待完成请求，用于同一格的重复提交检查。 */
    UPROPERTY(Transient)
    TMap<FGuid, FLxItemOperationRequest> PendingRequests;
    /** 当前数量选择窗口。 */
    UPROPERTY(Transient)
    TObjectPtr<ULxItemSplitWidget> SplitWidget = nullptr;
    /** 数量窗口保留的待提交来源凭据。 */
    UPROPERTY(Transient)
    TObjectPtr<ULxItemTransferTicket> SplitTicket = nullptr;
    /** 数量窗口所属业务窗口。 */
    UPROPERTY(Transient)
    TWeakObjectPtr<ULxUIBaseObject> SplitSourceWindow;
    /** 打开数量窗口时固定的请求上下文。 */
    UPROPERTY(Transient)
    FLxItemOperationRequest SplitRequest;
};