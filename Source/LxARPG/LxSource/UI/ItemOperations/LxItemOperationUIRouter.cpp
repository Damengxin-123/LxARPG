#include "LxItemOperationUIRouter.h"

#include "Blueprint/UserWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemQuickTransferInterface.h"
#include "LxARPG/LxSource/UI/ItemOperations/LxItemSplitWidget.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "LxItemOperationUI"

namespace
{
    /** 按完整上下文判断同一容器，避免不同仓库节点被混淆。 */
    bool SameContainer(const FLxItemContainerAddress& A, const FLxItemContainerAddress& B)
    {
        return A.Kind == B.Kind && A.Owner == B.Owner && A.RuntimeNodeIndex == B.RuntimeNodeIndex
            && A.InteractionTreeRevision == B.InteractionTreeRevision;
    }
    /** 只有实现统一接口的有效业务窗口可以参与交接。 */
    bool HasTransferInterface(const ULxUIBaseObject* Window)
    {
        return IsValid(Window) && Window->GetClass()->ImplementsInterface(ULxItemQuickTransferInterface::StaticClass());
    }
}

void ULxItemOperationUIRouter::Initialize(ULxUIManager* InManager)
{
    if (Manager.Get() != InManager) Shutdown();
    Manager = InManager;
    if (SplitWidgetClass.IsNull())
        SplitWidgetClass = TSoftClassPtr<ULxItemSplitWidget>(FSoftObjectPath(TEXT("/Game/项目内容/UI/角色UI界面/弹窗/物品拆分.物品拆分_C")));
    EnsureController();
}

void ULxItemOperationUIRouter::Shutdown()
{
    CancelSplit();
    for (const auto& Pair : PendingTickets) if (Pair.Value) Pair.Value->Cancel();
    PendingTickets.Reset();
    PendingWindows.Reset();
    PendingRequests.Reset();
    if (Controller.IsValid()) Controller->OnItemOperationCompleted.RemoveAll(this);
    Controller.Reset();
    Manager.Reset();
}

bool ULxItemOperationUIRouter::EnsureController()
{
    ALxPlayerController* NewController = Manager.IsValid() ? Cast<ALxPlayerController>(Manager->GetOwningPlayer()) : nullptr;
    if (NewController != Controller.Get())
    {
        if (Controller.IsValid()) Controller->OnItemOperationCompleted.RemoveAll(this);
        Controller = NewController;
    }
    if (!NewController || !NewController->IsLocalController()) return false;
    if (!NewController->OnItemOperationCompleted.IsBoundToObject(this))
        NewController->OnItemOperationCompleted.AddUObject(this, &ULxItemOperationUIRouter::HandleOperationCompleted);
    return true;
}

void ULxItemOperationUIRouter::ReportFailure(ULxUIBaseObject* SourceWindow, ELxItemOperationKind Kind, const FText& Reason)
{
    FLxItemOperationResult Result;
    Result.Kind = Kind;
    Result.Message = Reason;
    if (HasTransferInterface(SourceWindow))
        ILxItemQuickTransferInterface::Execute_NotifyQuickItemOperationResult(SourceWindow, Result);
}

bool ULxItemOperationUIRouter::HasPendingSource(const FLxItemContainerAddress& Source, int32 SlotIndex) const
{
    for (const auto& Pair : PendingRequests)
        if (Pair.Value.SourceSlotIndex == SlotIndex && SameContainer(Pair.Value.Source, Source)) return true;
    return false;
}

bool ULxItemOperationUIRouter::RequestQuickTransfer(ULxUIBaseObject* SourceWindow, ULxItemSlotData* SourceSlot)
{
    FText Reason;
    ULxUIManager* Layout = Manager.Get();
    if (!Layout || !Layout->IsMountedRoleWindow(SourceWindow) || !HasTransferInterface(SourceWindow)
        || !LxSafeItemOperations::IsQuickTransferSourceSlot(SourceSlot) || !LxSafeItemOperations::IsSupportedItem(SourceSlot ? SourceSlot->GetItem() : nullptr))
    {
        ReportFailure(SourceWindow, ELxItemOperationKind::QuickTransfer, LOCTEXT("InvalidSource", "此格子不支持快速存取"));
        return false;
    }
    if (Layout->HasBlockingModalWindow())
    {
        ReportFailure(SourceWindow, ELxItemOperationKind::QuickTransfer, LOCTEXT("ModalOpen", "请先关闭当前弹窗"));
        return false;
    }
    // 接口可以触发蓝图回调，临时保留来源窗口与票据直到本次同步交接结束。
    const TStrongObjectPtr<ULxUIBaseObject> SourceKeepAlive(SourceWindow);
    FLxItemOperationRequest Request;
    Request.RequestId = FGuid::NewGuid();
    if (!ILxItemQuickTransferInterface::Execute_GetQuickItemContainer(SourceWindow, Request.Source)
        || HasPendingSource(Request.Source, SourceSlot->GetSlotIndex()))
    {
        ReportFailure(SourceWindow, Request.Kind, LOCTEXT("SourceBusy", "来源窗口已失效或该格子正在处理中"));
        return false;
    }
    ULxItemTransferTicket* Ticket = ILxItemQuickTransferInterface::Execute_PrepareQuickItemTransfer(SourceWindow, SourceSlot, Reason);
    if (!Ticket)
    {
        ReportFailure(SourceWindow, Request.Kind, Reason);
        return false;
    }
    const TStrongObjectPtr<ULxItemTransferTicket> TicketKeepAlive(Ticket);
    // 凭据不移除原物品，任何目标查询失败都只需取消凭据。
    ULxUIBaseObject* TargetWindow = Layout->GetTopRoleWindow(SourceWindow);
    if (!HasTransferInterface(TargetWindow))
    {
        Ticket->Cancel();
        ReportFailure(SourceWindow, Request.Kind, LOCTEXT("NoReceiver", "最上层的其他窗口不支持快速存取"));
        return false;
    }
    const TStrongObjectPtr<ULxUIBaseObject> TargetKeepAlive(TargetWindow);
    Request.SourceSlotIndex = SourceSlot->GetSlotIndex();
    Request.ExpectedItem = Ticket->ExpectedItem;
    Request.Count = Ticket->Count;
    if (!ILxItemQuickTransferInterface::Execute_CanReceiveQuickItemTransfer(TargetWindow, Ticket, Reason)
        || !ILxItemQuickTransferInterface::Execute_ReceiveQuickItemTransfer(TargetWindow, Ticket, Request, SourceWindow, Reason))
    {
        Ticket->Cancel();
        ReportFailure(SourceWindow, Request.Kind, Reason.IsEmpty() ? LOCTEXT("ReceiverRejected", "目标窗口未接收物品") : Reason);
        return false;
    }
    return true;
}

bool ULxItemOperationUIRouter::SubmitOperation(const FLxItemOperationRequest& Request, ULxItemTransferTicket* Ticket,
    ULxUIBaseObject* SourceWindow, FText& OutReason)
{
    if (!Ticket || Ticket->IsCompleted() || !EnsureController() || !Manager.IsValid()
        || !Manager->IsMountedRoleWindow(SourceWindow) || HasPendingSource(Request.Source, Request.SourceSlotIndex))
    {
        OutReason = LOCTEXT("SubmitInvalid", "物品操作上下文已失效或正在处理中");
        return false;
    }
    PendingTickets.Add(Request.RequestId, Ticket);
    PendingWindows.Add(Request.RequestId, SourceWindow);
    PendingRequests.Add(Request.RequestId, Request);
    // 先登记请求再调用，单机中的结果可能同步返回。
    Controller->RequestItemOperation(Request);
    return true;
}

bool ULxItemOperationUIRouter::OpenSplitWindow(ULxUIBaseObject* SourceWindow, ULxItemSlotData* SourceSlot)
{
    FText Reason;
    if (!Manager.IsValid() || Manager->HasBlockingModalWindow() || !Manager->IsMountedRoleWindow(SourceWindow)
        || !HasTransferInterface(SourceWindow) || !LxSafeItemOperations::CanSplitItem(SourceSlot, Reason))
    {
        ReportFailure(SourceWindow, ELxItemOperationKind::SplitStack, Reason.IsEmpty() ? LOCTEXT("CannotOpenSplit", "当前无法拆分此物品") : Reason);
        return false;
    }
    FLxItemContainerAddress Source;
    if (!ILxItemQuickTransferInterface::Execute_GetQuickItemContainer(SourceWindow, Source)
        || HasPendingSource(Source, SourceSlot->GetSlotIndex()))
    {
        ReportFailure(SourceWindow, ELxItemOperationKind::SplitStack,
            LOCTEXT("SplitSourceBusy", "来源窗口已失效或该格子正在处理中"));
        return false;
    }
    ULxItemTransferTicket* Ticket = ILxItemQuickTransferInterface::Execute_PrepareQuickItemTransfer(SourceWindow, SourceSlot, Reason);
    if (!Ticket)
    {
        ReportFailure(SourceWindow, ELxItemOperationKind::SplitStack, Reason);
        return false;
    }
    // 同步加载布局期间也保留票据，来源物品仍由数据模块持有。
    const TStrongObjectPtr<ULxItemTransferTicket> TicketKeepAlive(Ticket);
    UClass* WidgetClass = SplitWidgetClass.LoadSynchronous();
    if (!WidgetClass || !EnsureController())
    {
        Ticket->Cancel();
        ReportFailure(SourceWindow, ELxItemOperationKind::SplitStack, LOCTEXT("SplitLayoutMissing", "拆分窗口布局尚未配置"));
        return false;
    }
    CancelSplit();
    SplitTicket = Ticket;
    SplitSourceWindow = SourceWindow;
    SplitRequest = FLxItemOperationRequest();
    SplitRequest.RequestId = FGuid::NewGuid();
    SplitRequest.Kind = ELxItemOperationKind::SplitStack;
    SplitRequest.Source = Source;
    SplitRequest.SourceSlotIndex = SourceSlot->GetSlotIndex();
    SplitRequest.ExpectedItem = Ticket->ExpectedItem;
    SplitWidget = CreateWidget<ULxItemSplitWidget>(Controller.Get(), WidgetClass);
    if (!SplitWidget || !Manager->MountRoleWindow(SplitWidget, true))
    {
        CancelSplit();
        ReportFailure(SourceWindow, ELxItemOperationKind::SplitStack, LOCTEXT("SplitMountFailed", "无法打开拆分窗口"));
        return false;
    }
    SplitWidget->InitializeSplit(this, Ticket->Item->ItemDisplayName(), Ticket->Count);
    SplitWidget->SetVisibility(ESlateVisibility::Visible);
    Manager->ActivateRoleWindow(SplitWidget);
    Manager->HideItemTooltip();
    Manager->RefreshCursorState();
    SplitWidget->SetKeyboardFocus();
    return true;
}

bool ULxItemOperationUIRouter::IsSplitSourceValid() const
{
    FLxItemContainerAddress Current;
    return SplitTicket && !SplitTicket->IsCompleted() && HasTransferInterface(SplitSourceWindow.Get())
        && SplitSourceWindow->IsVisible() && SplitSourceWindow->GetIsEnabled()
        && ILxItemQuickTransferInterface::Execute_GetQuickItemContainer(SplitSourceWindow.Get(), Current)
        && SameContainer(Current, SplitRequest.Source)
        && LxSafeItemOperations::MatchesExpectedRecord(SplitTicket->SourceSlot, SplitRequest.ExpectedItem);
}

bool ULxItemOperationUIRouter::ConfirmSplit(int32 Count)
{
    FText Reason;
    if (!IsSplitSourceValid())
    {
        if (SplitWidget) SplitWidget->ShowFailure(LOCTEXT("SplitSourceChanged", "来源物品已变化，请重新打开拆分窗口"));
        return false;
    }
    if (Count < 1 || Count >= SplitTicket->Count)
    {
        if (SplitWidget) SplitWidget->ShowFailure(LOCTEXT("SplitRange", "拆分数量超出允许范围"));
        return false;
    }
    SplitRequest.Count = Count;
    if (!SubmitOperation(SplitRequest, SplitTicket, SplitSourceWindow.Get(), Reason))
    {
        if (SplitWidget) SplitWidget->ShowFailure(Reason);
        return false;
    }
    return true;
}

void ULxItemOperationUIRouter::CancelSplit()
{
    ULxItemSplitWidget* Previous = SplitWidget;
    SplitWidget = nullptr;
    if (SplitTicket) SplitTicket->Cancel();
    SplitTicket = nullptr;
    SplitSourceWindow.Reset();
    if (Previous)
    {
        Previous->SetVisibility(ESlateVisibility::Collapsed);
        if (Manager.IsValid()) Manager->UnmountRoleWindow(Previous);
        else Previous->RemoveFromParent();
    }
    if (Manager.IsValid()) Manager->RefreshCursorState();
}

void ULxItemOperationUIRouter::HandleOperationCompleted(const FLxItemOperationResult& Result)
{
    if (!PendingRequests.Contains(Result.RequestId)) return;
    ULxUIBaseObject* SourceWindow = PendingWindows.FindRef(Result.RequestId).Get();
    const bool bIsOpenSplit = SplitWidget && SplitRequest.RequestId == Result.RequestId;
    if (ULxItemTransferTicket* Ticket = PendingTickets.FindRef(Result.RequestId))
        if (!bIsOpenSplit || Result.bSucceeded) Ticket->Cancel();
    PendingTickets.Remove(Result.RequestId);
    PendingWindows.Remove(Result.RequestId);
    PendingRequests.Remove(Result.RequestId);
    if (SplitWidget && SplitRequest.RequestId == Result.RequestId)
    {
        if (Result.bSucceeded) CancelSplit();
        else
        {
            // 重试采用新请求标识，重新打开窗口仍由来源记录校验保护。
            SplitRequest.RequestId = FGuid::NewGuid();
            SplitWidget->ShowFailure(Result.Message);
        }
    }
    if (HasTransferInterface(SourceWindow))
        ILxItemQuickTransferInterface::Execute_NotifyQuickItemOperationResult(SourceWindow, Result);
}

#undef LOCTEXT_NAMESPACE
