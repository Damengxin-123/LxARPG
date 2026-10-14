#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTradeContainerInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxTreasureChestInteractionComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxSafeItemOperations.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Player/Controllers/LxPlayerController.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxItemSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"

/** 验证真实控制器的服务器拒绝、结果回传和成功失败请求去重，不写入用户存档。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemOperationServerValidationTest,
    "LxARPG.UI.ItemOperations.ServerValidationAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxItemOperationServerValidationTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("创建服务器物品操作隔离世界"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
    World->SetGameInstance(Instance.Get());
    Instance->Init();
    ON_SCOPE_EXIT { Instance->Shutdown(); World->SetGameInstance(nullptr); };
    ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>();
    if (!TestTrue(TEXT("存在隔离存档管理器"), System && System->GetSaveManager())) return false;
    System->GetSaveManager()->SetReadOnly(true);
    TestTrue(TEXT("测试禁止用户存档写入"), System->GetSaveManager()->IsReadOnly());

    ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
    ALxPlayerController* Controller = World->SpawnActor<ALxPlayerController>();
    if (!TestNotNull(TEXT("创建真实玩家角色"), Player)
        || !TestNotNull(TEXT("创建真实玩家控制器"), Controller)) return false;
    Player->GetCharacterContentComponent()->BaseComponentInitialize();
    Controller->Possess(Player);
    if (!TestTrue(TEXT("服务器角色归属当前控制器"), Player->GetController() == Controller
        && Controller->GetPawn() == Player && Controller->HasAuthority())) return false;
    TestEqual(TEXT("隔离世界使用单机权威模式"), World->GetNetMode(), NM_Standalone);
    ULxCharacterBackpackModule* Backpack = Player->GetCharacterBackpackComponent();
    if (!TestNotNull(TEXT("真实角色背包模块已初始化"), Backpack)) return false;
    ULxItemBase* InitialItem = ULxItemBase::CreateItemObject(Backpack,
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 6));
    if (!TestNotNull(TEXT("创建真实可堆叠材料"), InitialItem)) return false;
    if (!TestTrue(TEXT("真实堆叠上限足够"), InitialItem->ItemInformation().ItemCountMax >= 6)) return false;
    FLxItemSlotSaveRecord Saved;
    Saved.SlotIndex = 0;
    Saved.SlotTag = LxTag_Item;
    if (!TestTrue(TEXT("完整采集物品记录"), LxItemSaveData::CaptureItem(InitialItem, Saved.Item))
        || !TestTrue(TEXT("恢复三格真实背包"), Backpack->RestoreBackpackSaveData(3, {Saved}))) return false;
    ULxItemSlotData* Source = Backpack->GetBackpackSlotAt(0);
    ULxItemBase* OriginalInstance = Source ? Source->GetItem() : nullptr;
    if (!TestNotNull(TEXT("恢复后的来源完整实例"), OriginalInstance)) return false;
    FLxItemSaveRecord OriginalRecord;
    if (!TestTrue(TEXT("保留操作前完整来源记录"), LxItemSaveData::CaptureItem(OriginalInstance, OriginalRecord))) return false;

    TArray<FLxItemOperationResult> Results;
    const FDelegateHandle ResultHandle = Controller->OnItemOperationCompleted.AddLambda(
        [&](const FLxItemOperationResult& Result) { Results.Add(Result); });
    ON_SCOPE_EXIT { Controller->OnItemOperationCompleted.Remove(ResultHandle); };
    int32 DataChanges = 0;
    const FDelegateHandle ChangeHandle = Backpack->OnDataChange.AddLambda([&]() { ++DataChanges; });
    ON_SCOPE_EXIT { Backpack->OnDataChange.Remove(ChangeHandle); };

    /** 直接进入服务器实现，仍经过单机客户端 RPC 返回最终结果。 */
    const auto Execute = [&](const FLxItemOperationRequest& Request)
    {
        const int32 PreviousResults = Results.Num();
        Controller->ServerExecuteItemOperation_Implementation(Request);
        if (!TestEqual(TEXT("每次执行均返回一个最终结果"), Results.Num(), PreviousResults + 1)) return false;
        TestEqual(TEXT("回传请求标识一致"), Results.Last().RequestId, Request.RequestId);
        TestEqual(TEXT("回传操作类型一致"), Results.Last().Kind, Request.Kind);
        TestFalse(TEXT("最终结果具有明确提示"), Results.Last().Message.IsEmpty());
        return true;
    };
    /** 来源和两个空格必须保持完整，拒绝不能产生数据变化通知。 */
    const auto ExpectUnchanged = [&](const TCHAR* Context)
    {
        TestTrue(*FString::Printf(TEXT("%s：来源仍为原实例"), Context), Source->GetItem() == OriginalInstance);
        TestEqual(*FString::Printf(TEXT("%s：来源数量未减少"), Context), OriginalInstance->ItemCount(), 6);
        TestTrue(*FString::Printf(TEXT("%s：完整属性未改变"), Context),
            LxSafeItemOperations::MatchesExpectedRecord(Source, OriginalRecord));
        TestNull(*FString::Printf(TEXT("%s：第二格保持空白"), Context), Backpack->GetBackpackSlotAt(1)->GetItem());
        TestNull(*FString::Printf(TEXT("%s：第三格保持空白"), Context), Backpack->GetBackpackSlotAt(2)->GetItem());
        TestEqual(*FString::Printf(TEXT("%s：拒绝不通知数据变化"), Context), DataChanges, 0);
    };
    /** 构建带真实容器归属和完整记录的拆分请求。 */
    const auto MakeRequest = [&]()
    {
        FLxItemOperationRequest Request;
        Request.RequestId = FGuid::NewGuid();
        Request.Kind = ELxItemOperationKind::SplitStack;
        Request.Source.Kind = ELxItemContainerKind::Backpack;
        Request.Source.Owner = Player;
        Request.SourceSlotIndex = 0;
        Request.Count = 2;
        Request.ExpectedItem = OriginalRecord;
        return Request;
    };

    FLxItemOperationRequest Stale = MakeRequest();
    ++Stale.ExpectedItem.ItemCount;
    if (!Execute(Stale)) return false;
    TestFalse(TEXT("过期来源记录被服务器拒绝"), Results.Last().bSucceeded);
    const FText OriginalFailure = Results.Last().Message;
    ExpectUnchanged(TEXT("过期请求"));

    FLxItemOperationRequest Invalid = MakeRequest();
    Invalid.SourceSlotIndex = INDEX_NONE;
    if (!Execute(Invalid)) return false;
    TestFalse(TEXT("负数来源槽位被拒绝"), Results.Last().bSucceeded);
    ExpectUnchanged(TEXT("负数索引"));
    Invalid.RequestId = FGuid::NewGuid();
    Invalid.SourceSlotIndex = 3;
    if (!Execute(Invalid)) return false;
    TestFalse(TEXT("超过容量的槽位被拒绝"), Results.Last().bSucceeded);
    ExpectUnchanged(TEXT("越界索引"));
    Invalid.RequestId = FGuid::NewGuid();
    Invalid.SourceSlotIndex = 1;
    if (!Execute(Invalid)) return false;
    TestFalse(TEXT("空槽位不能冒用有效记录"), Results.Last().bSucceeded);
    ExpectUnchanged(TEXT("空来源槽位"));

    AActor* ForeignOwner = World->SpawnActor<AActor>();
    if (!TestNotNull(TEXT("创建其他所属对象"), ForeignOwner)) return false;
    FLxItemOperationRequest Foreign = MakeRequest();
    Foreign.Source.Owner = ForeignOwner;
    if (!Execute(Foreign)) return false;
    TestFalse(TEXT("其他对象不能冒用当前背包"), Results.Last().bSucceeded);
    ExpectUnchanged(TEXT("来源越权"));
    FLxItemOperationRequest SameContainer = MakeRequest();
    SameContainer.Kind = ELxItemOperationKind::QuickTransfer;
    SameContainer.Count = 0;
    SameContainer.Target = SameContainer.Source;
    if (!Execute(SameContainer)) return false;
    TestFalse(TEXT("快速存取拒绝同源同目标"), Results.Last().bSucceeded);
    ExpectUnchanged(TEXT("同一容器"));

    FLxItemOperationRequest Valid = MakeRequest();
    if (!Execute(Valid)) return false;
    if (!TestTrue(TEXT("有效拆分成功"), Results.Last().bSucceeded)) return false;
    const FText SuccessMessage = Results.Last().Message;
    TestTrue(TEXT("拆分保留原来源实例"), Source->GetItem() == OriginalInstance);
    TestEqual(TEXT("来源保留四个"), OriginalInstance->ItemCount(), 4);
    ULxItemBase* SplitInstance = Backpack->GetBackpackSlotAt(1)->GetItem();
    if (!TestNotNull(TEXT("拆出物品进入独立空格"), SplitInstance)) return false;
    TestTrue(TEXT("拆出物品具有独立实例"), SplitInstance != OriginalInstance);
    TestEqual(TEXT("新格子数量为两个"), SplitInstance->ItemCount(), 2);
    TestEqual(TEXT("拆分数量守恒"), OriginalInstance->ItemCount() + SplitInstance->ItemCount(), 6);
    TestNull(TEXT("第三格仍为空白"), Backpack->GetBackpackSlotAt(2)->GetItem());
    const int32 ChangesAfterSuccess = DataChanges;

    // 精确重发返回缓存成功；原期望记录已过期，也不能再执行拆分。
    if (!Execute(Valid)) return false;
    TestTrue(TEXT("成功重发返回原成功结果"), Results.Last().bSucceeded);
    TestEqual(TEXT("成功重发返回原提示"), Results.Last().Message.ToString(), SuccessMessage.ToString());
    TestEqual(TEXT("成功重发不再减少来源"), OriginalInstance->ItemCount(), 4);
    TestEqual(TEXT("成功重发不增加目标数量"), SplitInstance->ItemCount(), 2);
    TestNull(TEXT("成功重发不创建第三格物品"), Backpack->GetBackpackSlotAt(2)->GetItem());
    TestEqual(TEXT("成功重发不产生新通知"), DataChanges, ChangesAfterSuccess);

    // 同一失败标识即使换成有效的新记录，也必须继续返回原失败。
    Stale.Count = 1;
    if (!TestTrue(TEXT("采集有效的新来源记录"), LxItemSaveData::CaptureItem(OriginalInstance, Stale.ExpectedItem))) return false;
    if (!Execute(Stale)) return false;
    TestFalse(TEXT("失败标识重发仍失败"), Results.Last().bSucceeded);
    TestEqual(TEXT("失败重发返回原原因"), Results.Last().Message.ToString(), OriginalFailure.ToString());
    TestTrue(TEXT("失败重发保留来源对象"), Source->GetItem() == OriginalInstance);
    TestEqual(TEXT("失败重发不能执行新拆分"), OriginalInstance->ItemCount(), 4);
    TestTrue(TEXT("失败重发保持新格子对象"), Backpack->GetBackpackSlotAt(1)->GetItem() == SplitInstance);
    TestEqual(TEXT("失败重发仍然数量守恒"), OriginalInstance->ItemCount() + SplitInstance->ItemCount(), 6);
    TestNull(TEXT("失败重发不创建第三格物品"), Backpack->GetBackpackSlotAt(2)->GetItem());
    TestEqual(TEXT("失败重发不产生通知"), DataChanges, ChangesAfterSuccess);
    return true;
}

/** 验证宝箱与交易的通用请求路由、交互权限及金币与物品的请求去重。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxItemOperationInteractionRoutesTest,
    "LxARPG.UI.ItemOperations.InteractionRoutesAndReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxItemOperationInteractionRoutesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("创建扩展快捷操作隔离世界"), World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    TStrongObjectPtr<UGameInstance> Instance(NewObject<UGameInstance>());
    World->SetGameInstance(Instance.Get());
    Instance->Init();
    ON_SCOPE_EXIT { Instance->Shutdown(); World->SetGameInstance(nullptr); };
    ULxGameInstanceSubsystem* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>();
    if (!TestTrue(TEXT("隔离世界具备只读存档管理器"), System && System->GetSaveManager())) return false;
    System->GetSaveManager()->SetReadOnly(true);

    ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
    ALxPlayerController* Controller = World->SpawnActor<ALxPlayerController>();
    AActor* Owner = World->SpawnActor<AActor>();
    if (!TestTrue(TEXT("创建实际玩家、控制器及交互对象"), Player && Controller && Owner)) return false;
    Player->GetCharacterContentComponent()->BaseComponentInitialize();
    Controller->Possess(Player);
    ULxCharacterBackpackModule* Backpack = Player->GetCharacterBackpackComponent();
    ULxPlayerInteractionModule* Interaction = Player->GetPlayerInteractionComponent();
    if (!TestTrue(TEXT("玩家提供真实背包与交互模块"), Backpack && Interaction)) return false;
    Interaction->InitializeModule(Player->GetPlayerControlComponent());

    FGameplayTag CommodityTag;
    int32 CommodityUnitPrice = 0;
    for (const auto& Pair : LxItemConfig::GetConsumableItemMap())
    {
        if (Pair.Value.ItemSellPrice > 0 && Pair.Value.ItemCountMax >= 4)
        {
            CommodityTag = Pair.Key;
            CommodityUnitPrice = Pair.Value.ItemSellPrice;
            break;
        }
    }
    if (!TestTrue(TEXT("真实商品配置包含可堆叠的正价消耗品"), CommodityTag.IsValid())) return false;
    const FLxItemInformationBase* GoldConfig = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
    if (!TestTrue(TEXT("金币堆叠容量允许测试完整买卖"), GoldConfig && GoldConfig->ItemCountMax >= 6)) return false;
    const int32 InitialGold = FMath::Min(100, GoldConfig->ItemCountMax - 4);
    ULxItemBase* Gold = ULxItemBase::CreateItemObject(Backpack, FLxItemQuote(LxTag_Item_Material_Currency_Gold, InitialGold));
    FLxItemSlotSaveRecord GoldRecord;
    GoldRecord.SlotIndex = 0;
    GoldRecord.SlotTag = LxTag_Item;
    if (!TestTrue(TEXT("采集初始金币并恢复真实背包"), LxItemSaveData::CaptureItem(Gold, GoldRecord.Item)
        && Backpack->RestoreBackpackSaveData(5, {GoldRecord}))) return false;
    /** 读取最终槽位，避免使用已被清理或合并的临时金币对象。 */
    const auto CountGold = [&]()
    {
        int32 Count = 0;
        for (ULxItemSlotData* Slot : Backpack->GetAllItems())
            if (Slot && Slot->GetItem() && Slot->GetItem()->ItemIDTag() == LxTag_Item_Material_Currency_Gold)
                Count += Slot->GetItem()->ItemCount();
        return Count;
    };

    ULxInteractableComponent* Provider = NewObject<ULxInteractableComponent>(Owner);
    Provider->bCreateAssetInteractionRange = false;
    Provider->FeatureConfig.bEnableTreasureChest = true;
    Provider->FeatureConfig.bEnableTradeContainer = true;
    Provider->FeatureConfig.TreasureChestConfig.ItemList = {FLxItemQuote(CommodityTag, 2)};
    FLxTradeItemConfig Commodity;
    Commodity.ItemIDTag = CommodityTag;
    Commodity.ItemCount = 2;
    Commodity.bLimitedStock = true;
    Provider->FeatureConfig.TradeContainerConfig.TradeItems = {Commodity};
    Provider->FeatureConfig.TradeContainerConfig.SellValueRate = 1.0f / static_cast<float>(CommodityUnitPrice);
    Provider->FeatureConfig.TradeContainerConfig.PurchaseValueRate = 1.0f / static_cast<float>(CommodityUnitPrice);
    Provider->RegisterComponentWithWorld(World);
    ULxInteractionTreeAsset* Asset = NewObject<ULxInteractionTreeAsset>(Provider);
    Asset->Features.bEnableTreasureChest = true;
    Asset->Features.bEnableTradeContainer = true;
    /** 创建实际资产节点，经项目正常实例化路径生成权限绑定与功能模块。 */
    const auto AddNode = [&](ELxInteractionActionType Type)
    {
        ULxInteractionTreeNodeData* Data = NewObject<ULxInteractionTreeNodeData>(Asset);
        Data->NodeId = FGuid::NewGuid();
        Data->Type = Type;
        Data->PromptText = FText::FromString(TEXT("快捷操作测试"));
        Asset->Nodes.Add(Data);
        Asset->Roots.Add(Data->NodeId);
    };
    AddNode(ELxInteractionActionType::TreasureChest);
    AddNode(ELxInteractionActionType::TradeContainer);
    if (!TestTrue(TEXT("加载实际宝箱与交易交互树"), Provider->SetInteractionTreeAsset(Asset))) return false;
    ULxTreasureChestInteractionComponent* Chest = nullptr;
    ULxTradeContainerInteractionComponent* Trade = nullptr;
    for (ULxInteractionNode* Node : Provider->GetRootInteractionNodes())
    {
        if (ULxTreasureChestInteractionComponent* Feature = Cast<ULxTreasureChestInteractionComponent>(Node->GetInteractionFeature())) Chest = Feature;
        if (ULxTradeContainerInteractionComponent* Feature = Cast<ULxTradeContainerInteractionComponent>(Node->GetInteractionFeature())) Trade = Feature;
    }
    if (!TestTrue(TEXT("正常创建宝箱及商品数据模块"), Chest && Trade)) return false;
    Provider->HandleInteractionRangeBeginOverlap(Player);
    ON_SCOPE_EXIT { Provider->HandleInteractionRangeEndOverlap(Player); };
    if (!TestTrue(TEXT("玩家进入允许操作的交互范围"), Interaction->IsInteractableComponentInRange(Provider))) return false;

    FLxItemContainerAddress BackpackAddress;
    BackpackAddress.Owner = Player;
    /** 捕获实际功能节点地址与当前树版本。 */
    const auto FeatureAddress = [&](ELxItemContainerKind Kind, ULxInteractionActionComponentBase* Feature)
    {
        FLxItemContainerAddress Address;
        Address.Kind = Kind;
        Address.Owner = Owner;
        Address.RuntimeNodeIndex = Feature->GetRuntimeNodeIndex();
        Address.InteractionTreeRevision = Provider->GetInteractionTreeRevision();
        return Address;
    };
    const FLxItemContainerAddress ChestAddress = FeatureAddress(ELxItemContainerKind::TreasureChest, Chest);
    const FLxItemContainerAddress TradeAddress = FeatureAddress(ELxItemContainerKind::Trade, Trade);
    /** 各请求携带完整来源快照；数量为零表示本次操作处理整格。 */
    const auto MakeRequest = [&](const FLxItemContainerAddress& SourceAddress,
        const FLxItemContainerAddress& TargetAddress, ULxItemSlotData* Slot)
    {
        FLxItemOperationRequest Request;
        Request.RequestId = FGuid::NewGuid();
        Request.Source = SourceAddress;
        Request.Target = TargetAddress;
        Request.SourceSlotIndex = Slot ? Slot->GetSlotIndex() : INDEX_NONE;
        LxItemSaveData::CaptureItem(Slot ? Slot->GetItem() : nullptr, Request.ExpectedItem);
        return Request;
    };
    TArray<FLxItemOperationResult> Results;
    const FDelegateHandle Handle = Controller->OnItemOperationCompleted.AddLambda(
        [&](const FLxItemOperationResult& Result) { Results.Add(Result); });
    ON_SCOPE_EXIT { Controller->OnItemOperationCompleted.Remove(Handle); };
    /** 使用真实服务器执行入口并验证最终结果确实回到玩家。 */
    const auto Execute = [&](const FLxItemOperationRequest& Request)
    {
        const int32 Before = Results.Num();
        Controller->ServerExecuteItemOperation_Implementation(Request);
        return TestEqual(TEXT("扩展请求回传唯一最终结果"), Results.Num(), Before + 1);
    };
    ULxItemSlotData* ChestSlot = Chest->GetTreasureChestSlotAt(0);
    ULxItemSlotData* TradeSlot = Trade->GetTradeSlotAt(0);
    if (!TestTrue(TEXT("实际宝箱和交易商品槽位有效"), ChestSlot && ChestSlot->GetItem() && TradeSlot && TradeSlot->GetItem())) return false;
    FText Reason;
    TestNull(TEXT("普通存取不能免费提取交易商品"), LxSafeItemOperations::BeginSafeExtract(TradeSlot, Reason));
    ULxItemBase* ChestInstance = ChestSlot->GetItem();
    FLxItemOperationRequest ChestRequest = MakeRequest(ChestAddress, BackpackAddress, ChestSlot);
    FLxItemOperationRequest StaleChest = ChestRequest;
    StaleChest.RequestId = FGuid::NewGuid();
    ++StaleChest.Source.InteractionTreeRevision;
    if (!Execute(StaleChest)) return false;
    TestFalse(TEXT("宝箱旧树版本请求被拒绝"), Results.Last().bSucceeded);
    TestTrue(TEXT("拒绝保留原宝箱实例与数量"), ChestSlot->GetItem() == ChestInstance && ChestInstance->ItemCount() == 2);
    TestEqual(TEXT("拒绝不影响金币"), CountGold(), InitialGold);
    Provider->HandleInteractionRangeEndOverlap(Player);
    FLxItemOperationRequest OutsideRange = ChestRequest;
    OutsideRange.RequestId = FGuid::NewGuid();
    if (!Execute(OutsideRange)) return false;
    TestFalse(TEXT("离开交互范围后无法快速取出"), Results.Last().bSucceeded);
    TestTrue(TEXT("范围拒绝仍保留完整宝箱物品"), ChestSlot->GetItem() == ChestInstance && ChestInstance->ItemCount() == 2);
    Provider->HandleInteractionRangeBeginOverlap(Player);

    FLxItemOperationRequest WrongTarget = MakeRequest(ChestAddress, TradeAddress, ChestSlot);
    if (!Execute(WrongTarget)) return false;
    TestFalse(TEXT("宝箱不能直接向交易界面出售"), Results.Last().bSucceeded);
    FLxItemOperationRequest ChestSplit = ChestRequest;
    ChestSplit.RequestId = FGuid::NewGuid();
    ChestSplit.Kind = ELxItemOperationKind::SplitStack;
    ChestSplit.Count = 1;
    if (!Execute(ChestSplit)) return false;
    TestFalse(TEXT("宝箱来源仍排除拆分"), Results.Last().bSucceeded);
    if (!Execute(ChestRequest) || !TestTrue(TEXT("宝箱快速取出进入背包"), Results.Last().bSucceeded)) return false;
    TestNull(TEXT("宝箱原槽位已清空"), ChestSlot->GetItem());
    TestTrue(TEXT("宝箱取出保留原实例并迁移归属"), Backpack->GetBackpackSlotAt(1)->GetItem() == ChestInstance && ChestInstance->GetOuter() == Backpack);
    TestEqual(TEXT("取出不扣金币"), CountGold(), InitialGold);
    if (!Execute(ChestRequest)) return false;
    TestTrue(TEXT("宝箱成功请求重发返回成功"), Results.Last().bSucceeded);
    TestEqual(TEXT("宝箱重发不重复添加"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 2);

    FLxItemOperationRequest Buy = MakeRequest(TradeAddress, BackpackAddress, TradeSlot);
    const int32 BuyPrice = TradeSlot->GetItemValue();
    if (!TestTrue(TEXT("实际商品报价有效且测试金币足够"), BuyPrice > 0 && BuyPrice <= InitialGold)) return false;
    FLxItemOperationRequest StaleBuy = Buy;
    StaleBuy.RequestId = FGuid::NewGuid();
    ++StaleBuy.ExpectedItem.ItemCount;
    if (!Execute(StaleBuy)) return false;
    TestFalse(TEXT("过期商品快照拒绝购买"), Results.Last().bSucceeded);
    TestEqual(TEXT("购买拒绝保留金币"), CountGold(), InitialGold);
    TestEqual(TEXT("购买拒绝保留商品库存"), TradeSlot->GetItem()->ItemCount(), 2);
    bool bTriedInFlightReplay = false;
    bool bInFlightReturnedPrematureResult = false;
    const FDelegateHandle InFlightHandle = Backpack->OnDataChange.AddLambda([&]()
    {
        if (bTriedInFlightReplay) return;
        bTriedInFlightReplay = true;
        const int32 Before = Results.Num();
        Controller->ServerExecuteItemOperation_Implementation(Buy);
        bInFlightReturnedPrematureResult = Results.Num() != Before;
    });
    ON_SCOPE_EXIT { Backpack->OnDataChange.Remove(InFlightHandle); };
    if (!Execute(Buy) || !TestTrue(TEXT("商品通过原购买逻辑进入背包"), Results.Last().bSucceeded)) return false;
    TestTrue(TEXT("购买最终通知确实重发同一在途请求"), bTriedInFlightReplay);
    TestFalse(TEXT("在途重发等待原结果，不抢先缓存失败"), bInFlightReturnedPrematureResult);
    TestEqual(TEXT("购买整组按当前报价扣金币"), CountGold(), InitialGold - BuyPrice);
    TestEqual(TEXT("购买后背包商品数量为四"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 4);
    TestNull(TEXT("有限库存购买完成后售罄"), TradeSlot->GetItem());
    if (!Execute(Buy)) return false;
    TestTrue(TEXT("购买成功请求重发返回成功"), Results.Last().bSucceeded);
    TestEqual(TEXT("购买重发不重复扣金币"), CountGold(), InitialGold - BuyPrice);
    TestEqual(TEXT("购买重发不重复添加商品"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 4);

    ULxItemSlotData* SaleSlot = Backpack->GetBackpackSlotAt(1);
    const int32 SellPrice = Trade->GetBackpackSlotSellPrice(SaleSlot);
    FLxItemOperationRequest Sell = MakeRequest(BackpackAddress, TradeAddress, SaleSlot);
    FLxItemOperationRequest DepositChest = MakeRequest(BackpackAddress, ChestAddress, SaleSlot);
    if (!Execute(DepositChest)) return false;
    TestFalse(TEXT("宝箱仍然禁止快速存入"), Results.Last().bSucceeded);
    TestEqual(TEXT("宝箱拒收保留待售物品"), SaleSlot->GetItem()->ItemCount(), 4);
    if (!Execute(Sell) || !TestTrue(TEXT("背包物品通过原出售逻辑完成"), Results.Last().bSucceeded)) return false;
    TestNull(TEXT("出售成功后清空来源槽位"), SaleSlot->GetItem());
    TestEqual(TEXT("出售按当前收购比例增加金币"), CountGold(), InitialGold - BuyPrice + SellPrice);
    if (!Execute(Sell)) return false;
    TestTrue(TEXT("出售成功请求重发返回成功"), Results.Last().bSucceeded);
    TestEqual(TEXT("出售重发不重复奖励金币"), CountGold(), InitialGold - BuyPrice + SellPrice);
    return true;
}

#endif
