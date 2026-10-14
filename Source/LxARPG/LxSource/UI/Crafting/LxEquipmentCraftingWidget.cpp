#include "LxEquipmentCraftingWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/UI/ItemGrid/LxItemDragInfo.h"
#include "UObject/StrongObjectPtr.h"

void ULxCraftingGridWidget::SetManufacturingComponent(ULxItemManufacturingComponent* Component)
{
    Manufacturing = Component;
    SetItemSlotData(Component ? Component->GetSelectedSlot() : nullptr);
    if (EquipmentIcon)
    {
        EquipmentIcon->SetBrushFromTexture(GetCurrentItem() ? GetDisplayIcon() : nullptr);
        EquipmentIcon->SetVisibility(GetCurrentItem() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
    }
    if (EmptyHint) EmptyHint->SetVisibility(GetCurrentItem() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void ULxCraftingGridWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetManufacturingComponent(Manufacturing);
}

bool ULxCraftingGridWidget::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
    SetDragHoverSelected(false);
    const ULxItemDragInfo* Drag = Cast<ULxItemDragInfo>(Operation);
    return Manufacturing && Drag && Manufacturing->SelectEquipment(Drag->SourceSlot);
}

FReply ULxCraftingGridWidget::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) { return FReply::Handled(); }
FReply ULxCraftingGridWidget::NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) { return FReply::Handled(); }
void ULxCraftingGridWidget::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation) { Operation = nullptr; }

void ULxEquipmentCraftingWidget::SetManufacturingComponent(ULxItemManufacturingComponent* Component)
{
    if (Manufacturing)
    {
        Manufacturing->OnManufacturingChanged.RemoveAll(this);
        Manufacturing->OnEquipmentCrafted.RemoveAll(this);
    }
    Manufacturing = Component;
    if (Manufacturing)
    {
        Manufacturing->OnManufacturingChanged.AddUObject(this, &ULxEquipmentCraftingWidget::RefreshCrafting);
        Manufacturing->OnEquipmentCrafted.AddDynamic(this, &ULxEquipmentCraftingWidget::HandleCompleted);
    }
    RefreshCrafting();
}

bool ULxEquipmentCraftingWidget::GetQuickItemContainer_Implementation(FLxItemContainerAddress& OutContainer) const
{
    OutContainer = FLxItemContainerAddress();
    return false;
}

ULxItemTransferTicket* ULxEquipmentCraftingWidget::PrepareQuickItemTransfer_Implementation(ULxItemSlotData* SourceSlot, FText& OutReason)
{
    OutReason = NSLOCTEXT("LxItemOperations", "CraftingExtractDenied", "打造格子仅显示装备链接，不能从这里提取物品");
    return nullptr;
}

bool ULxEquipmentCraftingWidget::CanReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket, FText& OutReason) const
{
    if (!IsVisible() || !GetIsEnabled() || !Manufacturing)
    {
        OutReason = NSLOCTEXT("LxItemOperations", "CraftingWindowInvalid", "装备打造窗口已失效");
        return false;
    }
    if (!IsValid(Ticket) || Ticket->IsCompleted() || !IsValid(Ticket->SourceSlot)
        || Ticket->SourceSlot->GetSlotType() != ELxItemSlotType::Backpack
        || Ticket->SourceSlot->GetItem() != Ticket->Item || Ticket->Count != 1
        || !Cast<ULxEquipment>(Ticket->Item)
        || !LxSafeItemOperations::MatchesExpectedRecord(Ticket->SourceSlot, Ticket->ExpectedItem))
    {
        OutReason = NSLOCTEXT("LxItemOperations", "CraftingLinkInvalid", "只能从背包快速链接完整装备，来源物品不能发生变化");
        return false;
    }
    return Manufacturing->CanSelectEquipment(Ticket->SourceSlot, OutReason);
}

bool ULxEquipmentCraftingWidget::ReceiveQuickItemTransfer_Implementation(ULxItemTransferTicket* Ticket,
    const FLxItemOperationRequest& Request, ULxUIBaseObject* SourceWindow, FText& OutReason)
{
    if (!CanReceiveQuickItemTransfer_Implementation(Ticket, OutReason)) return false;
    // 装备选择及蓝图结果事件可能关闭页面或触发回收，整个同步交接保持相关对象存活。
    TStrongObjectPtr<ULxItemTransferTicket> ProtectedTicket(Ticket);
    TStrongObjectPtr<ULxEquipmentCraftingWidget> ProtectedWindow(this);
    TStrongObjectPtr<ULxUIBaseObject> ProtectedSource(SourceWindow);
    if (Request.Kind != ELxItemOperationKind::QuickTransfer || Request.Source.Kind != ELxItemContainerKind::Backpack
        || Request.SourceSlotIndex != Ticket->SourceSlot->GetSlotIndex()
        || !LxSafeItemOperations::MatchesExpectedRecord(Ticket->SourceSlot, Request.ExpectedItem)
        || !Manufacturing->SelectEquipment(Ticket->SourceSlot))
    {
        OutReason = NSLOCTEXT("LxItemOperations", "CraftingLinkFailed", "装备来源或打造状态已变化，未修改物品");
        return false;
    }
    // 选择只保存原装备身份与槽位引用，终结凭证不移除来源，也不消耗金币。
    Ticket->Cancel();
    FLxItemOperationResult Result;
    Result.RequestId = Request.RequestId;
    Result.Kind = ELxItemOperationKind::QuickTransfer;
    Result.bSucceeded = true;
    Result.Message = NSLOCTEXT("LxItemOperations", "CraftingLinked", "装备已链接到打造格子");
    ILxItemQuickTransferInterface::Execute_NotifyQuickItemOperationResult(this, Result);
    if (IsValid(SourceWindow) && SourceWindow->GetClass()->ImplementsInterface(ULxItemQuickTransferInterface::StaticClass()))
        ILxItemQuickTransferInterface::Execute_NotifyQuickItemOperationResult(SourceWindow, Result);
    return true;
}

void ULxEquipmentCraftingWidget::NotifyQuickItemOperationResult_Implementation(const FLxItemOperationResult& Result)
{
    LastItemOperationResult = Result;
    if (ResultText) ResultText->SetText(Result.Message);
    OnItemOperationCompleted(Result);
}

void ULxEquipmentCraftingWidget::NativeConstruct()
{
    Super::NativeConstruct();
    if (EnhanceButton) EnhanceButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleEnhance);
    if (AdjustButton) AdjustButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleAdjust);
    if (AddButton) AddButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleAdd);
    if (RemoveButton) RemoveButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleRemove);
    if (ClearButton) ClearButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleClear);
    if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ULxEquipmentCraftingWidget::HandleClose);
    RefreshCrafting();
}

void ULxEquipmentCraftingWidget::NativeDestruct()
{
    SetManufacturingComponent(nullptr);
    Super::NativeDestruct();
}

void ULxEquipmentCraftingWidget::RefreshCrafting()
{
    if (EquipmentGrid) { EquipmentGrid->SetOwningUIManager(GetOwningUIManager()); EquipmentGrid->SetManufacturingComponent(Manufacturing); }
    ULxEquipment* Equipment = Manufacturing ? Manufacturing->GetSelectedEquipment() : nullptr;
    if (EquipmentSummary)
    {
        EquipmentSummary->SetText(Equipment ? FText::FromString(FString::Printf(TEXT("%s  ·  %s\n强化度 %.0f%%  ·  普通词条 %d / 20%s"),
            *Equipment->ItemDisplayName().ToString(), *StaticEnum<ELxItemRarityType>()->GetDisplayNameTextByValue(int64(Equipment->ItemRarity())).ToString(),
            Equipment->GetEnhancement() * 100.f, Equipment->GetNormalEntries().Num(), Equipment->HasFixedEntries() ? TEXT("  ·  固定词条") : TEXT("")))
            : FText::FromString(TEXT("拖入背包或装备栏中的装备，或在背包按Alt＋左键快速链接")));
    }
    if (GoldText) GoldText->SetText(FText::FromString(FString::Printf(TEXT("持有金币  %d"), Manufacturing ? Manufacturing->GetGoldCount() : 0)));
    if (ResultText) ResultText->SetText(Manufacturing && !Manufacturing->GetLastResult().Message.IsEmpty()
        ? Manufacturing->GetLastResult().Message : FText::FromString(TEXT("选择装备后，可查看每项操作的金币费用与成功率")));
    if (EntriesText)
    {
        FString Text;
        if (Equipment)
        {
            for (ELxEntryLogicType Type : { ELxEntryLogicType::Base, ELxEntryLogicType::Normal })
            {
                Text += Type == ELxEntryLogicType::Base ? TEXT("基础词条（静态）\n") : TEXT("\n普通词条\n");
                int32 NormalIndex = 0;
                const TArray<FLxEquipmentEntry> NormalEntries = Equipment->GetNormalEntries();
                for (const FLxItemEntryRuntimeInfo& Entry : Equipment->GetItemEntryRuntimeInfoList())
                {
                    if (Entry.EntryLogicType != Type || !Entry.EntryObject) continue;
                    // 打造页面与装备物品信息使用相同的强化显示倍率，原始生效比例仍独立展示。
                    Text += Entry.EntryObject->GetDisplayNameWithScale(Equipment->GetItemEntryEffectMultiplier()).ToString();
                    if (Type == ELxEntryLogicType::Normal && NormalEntries.IsValidIndex(NormalIndex))
                        Text += FString::Printf(TEXT("  [生效比例 %.0f%%]"), NormalEntries[NormalIndex++].Proportion * 100.f);
                    Text += TEXT("\n");
                }
            }
            // 普通文本控件去除富文本标记，保留本地化后的实际数值。
            int32 Begin = Text.Find(TEXT("<"));
            while (Begin != INDEX_NONE)
            {
                const int32 End = Text.Find(TEXT(">"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Begin);
                if (End == INDEX_NONE) break;
                Text.RemoveAt(Begin, End - Begin + 1);
                Begin = Text.Find(TEXT("<"));
            }
        }
        EntriesText->SetText(FText::FromString(Text));
    }
    UButton* Buttons[] = { EnhanceButton, AdjustButton, AddButton, RemoveButton };
    UTextBlock* Labels[] = { EnhanceQuote, AdjustQuote, AddQuote, RemoveQuote };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FLxEquipmentCraftQuote Quote = Manufacturing ? Manufacturing->GetQuote(ELxEquipmentCraftOperation(Index)) : FLxEquipmentCraftQuote();
        if (Buttons[Index]) Buttons[Index]->SetIsEnabled(Quote.bAvailable);
        if (Labels[Index])
        {
            FString Text = FString::Printf(TEXT("%d 金币  ·  %s %.2f%%"), Quote.GoldCost,
                Index == 1 ? TEXT("每条提高概率") : TEXT("成功率"), Quote.SuccessChance * 100.f);
            if (!Quote.Reason.IsEmpty()) Text += TEXT("\n") + Quote.Reason.ToString();
            Labels[Index]->SetText(FText::FromString(Text));
        }
    }
    if (ClearButton) ClearButton->SetIsEnabled(Equipment && Manufacturing && !Manufacturing->IsRequestPending());
    OnCraftingDisplayUpdated();
}

void ULxEquipmentCraftingWidget::HandleEnhance() { if (Manufacturing) Manufacturing->RequestCraft(ELxEquipmentCraftOperation::Enhance); }
void ULxEquipmentCraftingWidget::HandleAdjust() { if (Manufacturing) Manufacturing->RequestCraft(ELxEquipmentCraftOperation::AdjustEntries); }
void ULxEquipmentCraftingWidget::HandleAdd() { if (Manufacturing) Manufacturing->RequestCraft(ELxEquipmentCraftOperation::AddEntry); }
void ULxEquipmentCraftingWidget::HandleRemove() { if (Manufacturing) Manufacturing->RequestCraft(ELxEquipmentCraftOperation::RemoveEntry); }
void ULxEquipmentCraftingWidget::HandleClear() { if (Manufacturing) Manufacturing->ClearSelection(); }
void ULxEquipmentCraftingWidget::HandleClose() { CloseUIDisplay(); }
void ULxEquipmentCraftingWidget::HandleCompleted(const FLxEquipmentCraftResult& Result) { OnCraftingCompleted(Result); }
