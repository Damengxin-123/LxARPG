#include "LxItemManufacturingComponent.h"

#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterEquipmentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Model/Item/DataType/Equipment/LxEquipment.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"

ULxItemManufacturingComponent::ULxItemManufacturingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    Random.GenerateNewSeed();
}

void ULxItemManufacturingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndEquipmentService();
    Super::EndPlay(EndPlayReason);
}

bool ULxItemManufacturingComponent::IsServiceValid(ULxFunctionPageInteractionComponent* Feature) const
{
    const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(GetOwner());
    ULxPlayerInteractionModule* Interaction = Player ? Player->GetPlayerInteractionComponent() : nullptr;
    ULxInteractableComponent* NPC = IsValid(Feature) ? Feature->GetInteractableComponent() : nullptr;
    ULxInteractionNode* Node = IsValid(Feature) ? Feature->GetOwnerInteractionNode() : nullptr;
    return Interaction && NPC && IsValid(NPC->GetOwner()) && Node && Feature->IsEquipmentCraftingPage()
        && NPC->OwnsInteractionNode(Node) && Node->GetInteractionFeature() == Feature
        && NPC->GetInteractionTreeRevision() == Feature->GetInteractionTreeRevision()
        && Interaction->IsInteractableComponentInRange(NPC) && Node->CheckCommonRequirement(Interaction)
        && (Feature->GetInteractionState() == ELxInteractionDataState::Interactable
            || Feature->GetInteractionState() == ELxInteractionDataState::Interacting);
}

bool ULxItemManufacturingComponent::BeginEquipmentService(ULxFunctionPageInteractionComponent* Feature)
{
    EndEquipmentService();
    if (!IsServiceValid(Feature)) return false;
    ActiveService = Feature;
    if (const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(GetOwner()))
    {
        Player->GetCharacterBackpackComponent()->OnDataChange.AddUObject(this, &ULxItemManufacturingComponent::HandleInventoryChanged);
        Player->GetCharacterContentComponent()->GetEquipmentModule()->OnDataChange.AddUObject(this, &ULxItemManufacturingComponent::HandleInventoryChanged);
    }
    OnManufacturingChanged.Broadcast();
    return true;
}

void ULxItemManufacturingComponent::EndEquipmentService()
{
    if (const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(GetOwner()))
    {
        if (Player->GetCharacterBackpackComponent()) Player->GetCharacterBackpackComponent()->OnDataChange.RemoveAll(this);
        if (Player->GetCharacterContentComponent() && Player->GetCharacterContentComponent()->GetEquipmentModule())
            Player->GetCharacterContentComponent()->GetEquipmentModule()->OnDataChange.RemoveAll(this);
    }
    ActiveService.Reset();
    SelectedID.Invalidate();
    LastResult = FLxEquipmentCraftResult();
    OnManufacturingChanged.Broadcast();
}

ULxItemSlotData* ULxItemManufacturingComponent::FindOwnedEquipment(const FGuid& ID) const
{
    const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(GetOwner());
    if (!Player || !ID.IsValid() || !Player->GetCharacterBackpackComponent() || !Player->GetCharacterContentComponent()) return nullptr;
    TArray<TObjectPtr<ULxItemSlotData>> Slots = Player->GetCharacterBackpackComponent()->GetAllItems();
    if (auto* Module = Player->GetCharacterContentComponent()->GetEquipmentModule()) Slots.Append(Module->GetEquipmentSlots());
    for (ULxItemSlotData* Slot : Slots)
        if (ULxEquipment* Equipment = Slot ? Cast<ULxEquipment>(Slot->GetItem()) : nullptr;
            Equipment && Equipment->ItemIsValid() && Equipment->GetInstanceID() == ID) return Slot;
    return nullptr;
}

bool ULxItemManufacturingComponent::SelectEquipment(ULxItemSlotData* Slot)
{
    FText Reason;
    if (!CanSelectEquipment(Slot, Reason)) return false;
    ULxEquipment* Equipment = CastChecked<ULxEquipment>(Slot->GetItem());
    SelectedID = Equipment->GetInstanceID();
    LastResult = FLxEquipmentCraftResult();
    OnManufacturingChanged.Broadcast();
    return true;
}

bool ULxItemManufacturingComponent::CanSelectEquipment(ULxItemSlotData* Slot, FText& OutReason) const
{
    OutReason = FText::GetEmpty();
    if (bRequestPending || bExecuting || LxSafeItemOperations::IsSlotBusy(Slot))
    {
        OutReason = NSLOCTEXT("LxEquipmentCrafting", "SelectionBusy", "装备或打造操作正在处理中");
        return false;
    }
    if (!IsServiceValid(ActiveService.Get()))
    {
        OutReason = NSLOCTEXT("LxEquipmentCrafting", "SelectionServiceInvalid", "装备打造服务已失效，请重新与NPC交互");
        return false;
    }
    ULxEquipment* Equipment = Slot ? Cast<ULxEquipment>(Slot->GetItem()) : nullptr;
    if (!Equipment || !Equipment->ItemIsValid() || Equipment->ItemCount() != 1
        || FindOwnedEquipment(Equipment->GetInstanceID()) != Slot)
    {
        OutReason = NSLOCTEXT("LxEquipmentCrafting", "SelectionEquipmentInvalid", "只能链接当前玩家持有的完整装备实例");
        return false;
    }
    return true;
}

void ULxItemManufacturingComponent::ClearSelection()
{
    if (bRequestPending) return;
    SelectedID.Invalidate();
    OnManufacturingChanged.Broadcast();
}

ULxItemSlotData* ULxItemManufacturingComponent::GetSelectedSlot() const { return FindOwnedEquipment(SelectedID); }

ULxEquipment* ULxItemManufacturingComponent::GetSelectedEquipment() const
{
    ULxItemSlotData* Slot = GetSelectedSlot();
    return Slot ? Cast<ULxEquipment>(Slot->GetItem()) : nullptr;
}

int32 ULxItemManufacturingComponent::GetGoldCount() const
{
    const ALxPlayerCharacter* Player = Cast<ALxPlayerCharacter>(GetOwner());
    if (!Player || !Player->GetCharacterBackpackComponent()) return 0;
    int64 Count = 0;
    for (ULxItemSlotData* Slot : Player->GetCharacterBackpackComponent()->GetAllItems())
        if (Slot && Slot->IsValid() && Slot->GetItem()->ItemIDTag() == LxTag_Item_Material_Currency_Gold) Count += Slot->GetItem()->ItemCount();
    return int32(FMath::Min<int64>(Count, MAX_int32));
}

TArray<FGameplayTag> ULxItemManufacturingComponent::GetEntryCandidates(ULxEquipment* Equipment) const
{
    TArray<FGameplayTag> Result;
    if (!Equipment) return Result;
    TArray<FGameplayTag> Pool = Equipment->EquipmentInformation().ForgeEntryPool;
    if (Pool.IsEmpty()) Pool = LxEntryConfig::GetEquipmentForgeEntryIDs();
    for (FGameplayTag ID : Pool)
    {
        if (!LxEntryConfig::GetEntryData(ID)) continue;
        const bool bExisting = Equipment->ItemInformation().ItemEntryConfigs.ContainsByPredicate(
            [ID](const FLxItemEntryConfig& Entry) { return Entry.EntryQuote.EntryID == ID; });
        if (!bExisting) Result.AddUnique(ID);
    }
    Result.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.ToString() < B.ToString(); });
    return Result;
}

FLxEquipmentCraftQuote ULxItemManufacturingComponent::BuildQuote(ELxEquipmentCraftOperation Operation,
    ULxEquipment* Equipment, ULxFunctionPageInteractionComponent* Feature) const
{
    FLxEquipmentCraftQuote Quote;
    if (!IsServiceValid(Feature)) { Quote.Reason = FText::FromString(TEXT("请先与NPC交互打开装备打造")); return Quote; }
    if (!Equipment) { Quote.Reason = FText::FromString(TEXT("请将装备拖入或快速链接到打造格子")); return Quote; }
    const int32 Count = Equipment->GetNormalEntries().Num();
    Quote = LxEquipmentCrafting::CalculateQuote(Operation, Equipment->GetEnhancement(), Count, Feature->GetCraftService());
    if (!Quote.bAvailable) return Quote;
    FString Reason;
    if (Equipment->HasFixedEntries() && Operation != ELxEquipmentCraftOperation::Enhance) Reason = TEXT("固定词条装备仅可强化");
    else if (Operation == ELxEquipmentCraftOperation::AddEntry && Count >= ULxEquipment::MaxNormalEntries) Reason = TEXT("普通词条已达20条上限");
    else if (Operation == ELxEquipmentCraftOperation::AddEntry && GetEntryCandidates(Equipment).IsEmpty()) Reason = TEXT("没有可添加的新词条");
    else if ((Operation == ELxEquipmentCraftOperation::AdjustEntries || Operation == ELxEquipmentCraftOperation::RemoveEntry) && Count == 0) Reason = TEXT("没有可操作的普通词条");
    else if (GetGoldCount() < Quote.GoldCost) Reason = TEXT("金币不足");
    Quote.bAvailable = Reason.IsEmpty();
    Quote.Reason = FText::FromString(Reason);
    return Quote;
}

FLxEquipmentCraftQuote ULxItemManufacturingComponent::GetQuote(ELxEquipmentCraftOperation Operation) const
{
    FLxEquipmentCraftQuote Quote = BuildQuote(Operation, GetSelectedEquipment(), ActiveService.Get());
    if (bRequestPending) { Quote.bAvailable = false; Quote.Reason = FText::FromString(TEXT("正在打造，请稍候")); }
    return Quote;
}

void ULxItemManufacturingComponent::RequestCraft(ELxEquipmentCraftOperation Operation)
{
    if (bRequestPending || bExecuting) return;
    const FLxEquipmentCraftQuote Quote = GetQuote(Operation);
    if (!Quote.bAvailable)
    {
        FLxEquipmentCraftResult Result; Result.Operation = Operation; Result.Message = Quote.Reason;
        ClientCraftResult_Implementation(Result); return;
    }
    ULxFunctionPageInteractionComponent* Service = ActiveService.Get();
    bRequestPending = true;
    ServerCraft(Operation, SelectedID, Service->GetInteractableComponent(), Service->GetRuntimeNodeIndex(), Service->GetInteractionTreeRevision());
    OnManufacturingChanged.Broadcast();
}

void ULxItemManufacturingComponent::ServerCraft_Implementation(ELxEquipmentCraftOperation Operation, FGuid ID,
    ULxInteractableComponent* NPC, int32 NodeIndex, int32 Revision)
{
    ULxInteractionNode* Node = IsValid(NPC) && NPC->GetInteractionTreeRevision() == Revision ? NPC->FindInteractionNodeByRuntimeIndex(NodeIndex) : nullptr;
    auto* Feature = Node ? Cast<ULxFunctionPageInteractionComponent>(Node->GetInteractionFeature()) : nullptr;
    ClientCraftResult(ExecuteCraft(Operation, ID, Feature));
}

FLxEquipmentCraftResult ULxItemManufacturingComponent::ExecuteCraft(ELxEquipmentCraftOperation Operation,
    const FGuid& ID, ULxFunctionPageInteractionComponent* Feature)
{
    FLxEquipmentCraftResult Result; Result.Operation = Operation;
    if (bExecuting || !GetOwner() || !GetOwner()->HasAuthority()) return Result;
    TGuardValue<bool> Guard(bExecuting, true);
    ULxItemSlotData* Slot = FindOwnedEquipment(ID);
    ULxEquipment* Equipment = Slot ? Cast<ULxEquipment>(Slot->GetItem()) : nullptr;
    const FLxEquipmentCraftQuote Quote = BuildQuote(Operation, Equipment, Feature);
    if (!Quote.bAvailable) { Result.Message = Quote.Reason; return Result; }
    TArray<FLxEquipmentEntry> Entries = Equipment->GetNormalEntries();
    const FLxEquipmentSaveRecord Before = Equipment->CaptureEquipmentState();
    float Enhancement = Equipment->GetEnhancement();
    Result.bSuccess = Random.FRand() < Quote.SuccessChance;
    switch (Operation)
    {
    case ELxEquipmentCraftOperation::Enhance:
        Enhancement = LxEquipmentCrafting::RollEnhancement(Enhancement, Result.bSuccess, Random);
        Result.EnhancementDelta = Enhancement - Equipment->GetEnhancement();
        break;
    case ELxEquipmentCraftOperation::AdjustEntries:
        Result.bSuccess = true;
        for (FLxEquipmentEntry& Entry : Entries)
        {
            const float Next = LxEquipmentCrafting::RollProportion(Entry.Proportion, Random);
            Result.ChangedEntries += !FMath::IsNearlyEqual(Next, Entry.Proportion);
            Entry.Proportion = Next;
        }
        break;
    case ELxEquipmentCraftOperation::AddEntry:
        if (Result.bSuccess)
        {
            const TArray<FGameplayTag> Pool = GetEntryCandidates(Equipment);
            FLxEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
            NewEntry.EntryID = Pool[Random.RandRange(0, Pool.Num() - 1)];
            NewEntry.Proportion = Random.RandRange(0, 100) / 100.f;
            Result.ChangedEntries = 1;
        }
        break;
    case ELxEquipmentCraftOperation::RemoveEntry:
        {
            const int32 RemovedIndex = LxEquipmentCrafting::RollRemoval(Entries.Num(), Random);
            Result.bSuccess = RemovedIndex != INDEX_NONE;
            if (Result.bSuccess) { Entries.RemoveAt(RemovedIndex); Result.ChangedEntries = 1; }
        }
        break;
    default: return Result;
    }
    // 先验证并准备实例状态；资金检查失败时恢复，只有完整结算才广播变化。
    if (!Equipment->ApplyCraftingState(Enhancement, Entries)) { Result.Message = FText::FromString(TEXT("装备词条配置无效，未扣除金币")); return Result; }
    auto* Backpack = CastChecked<ALxPlayerCharacter>(GetOwner())->GetCharacterBackpackComponent();
    if (Quote.GoldCost > 0 && !Backpack->RemoveItemList({ FLxItemQuote(LxTag_Item_Material_Currency_Gold, Quote.GoldCost) }))
    {
        Equipment->RestoreEquipmentState(Before, ID);
        Result.Message = FText::FromString(TEXT("金币不足，未执行打造")); return Result;
    }
    Result.bExecuted = true;
    Result.GoldSpent = Quote.GoldCost;
    Result.Message = FText::FromString(Result.bSuccess ? TEXT("打造成功") : TEXT("本次打造未成功，金币已消耗"));
    if (Operation == ELxEquipmentCraftOperation::AdjustEntries) Result.Message = FText::FromString(TEXT("普通词条已分别随机调整"));
    if (Operation == ELxEquipmentCraftOperation::Enhance)
        Result.Message = FText::FromString(FString::Printf(TEXT("%s：强化度%s %.0f%%"), Result.bSuccess ? TEXT("强化成功") : TEXT("强化失败"), Result.bSuccess ? TEXT("增加") : TEXT("减少"), FMath::Abs(Result.EnhancementDelta) * 100.f));
    Slot->OnItemDataChanged.Broadcast(Equipment);
    Backpack->SyncReplicatedBackpackSlots();
    return Result;
}

void ULxItemManufacturingComponent::ClientCraftResult_Implementation(const FLxEquipmentCraftResult& Result)
{
    bRequestPending = false;
    LastResult = Result;
    OnManufacturingChanged.Broadcast();
    OnEquipmentCrafted.Broadcast(Result);
}

void ULxItemManufacturingComponent::HandleInventoryChanged()
{
    OnManufacturingChanged.Broadcast();
}
