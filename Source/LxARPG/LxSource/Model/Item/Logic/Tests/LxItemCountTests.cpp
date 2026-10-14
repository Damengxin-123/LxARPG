#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/ShowInfoConfig/LxItemShowInfoConfigID.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveHelpers.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

/** 大堆叠数量验证的隔离配置，所有修改只发生在运行时缓存。 */
namespace LxItemCountTest
{
    /** 加载真实物品数据，临时调整测试堆叠上限，退出时恢复原配置并禁止写玩家存档。 */
    struct FFixture
    {
        /** 持有测试物品及背包模块的独立游戏实例。 */
        TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>()};
        /** 测试前实际使用的金币静态配置。 */
        FLxMaterialInformation OriginalGold;
        /** 是否已成功读取并替换运行时金币配置。 */
        bool bReady = false;
        /** 网络快照验证使用的临时权威世界，仅在需要真实角色时创建。 */
        UWorld* World = nullptr;
        /** 设置本次验证的堆叠上限，保持资产文件和用户存档不变。 */
        explicit FFixture(const int32 Maximum)
        {
            Instance->Init();
            if (auto* System = Instance->GetSubsystem<ULxGameInstanceSubsystem>())
                if (System->GetSaveManager()) System->GetSaveManager()->SetReadOnly(true);
            const auto* Data = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
            if (!Data || Data->ItemType != ELxItemType::Material) return;
            OriginalGold = *static_cast<const FLxMaterialInformation*>(Data);
            FLxMaterialInformation Gold = OriginalGold;
            Gold.ItemCountMax = Maximum;
            LxItemConfig::SetMaterialItemData(Gold);
            bReady = true;
        }
        /** 恢复静态缓存并结束测试实例。 */
        ~FFixture()
        {
            if (bReady) LxItemConfig::SetMaterialItemData(OriginalGold);
            Instance->Shutdown();
            if (World) { World->SetGameInstance(nullptr); World->DestroyWorld(false); }
        }
        /** 通过真实物品创建路径构建指定数量金币。 */
        ULxItemBase* MakeGold(const int32 Count)
        {
            return ULxItemBase::CreateItemObject(Instance.Get(), FLxItemQuote(LxTag_Item_Material_Currency_Gold, Count));
        }
        /** 创建真实权威角色的背包，使数量变更触发生产代码的网络快照。 */
        ULxCharacterBackpackModule* MakeCharacterBackpack()
        {
            World = UWorld::CreateWorld(EWorldType::Game, false);
            World->SetGameInstance(Instance.Get());
            World->InitializeActorsForPlay(FURL());
            auto* Player = World->SpawnActor<ALxPlayerCharacter>();
            if (!Player) return nullptr;
            Player->GetCharacterContentComponent()->BaseComponentInitialize();
            Player->GetCharacterDataTransferComponent()->BaseComponentInitialize();
            return Player->GetCharacterBackpackComponent();
        }
    };
}

/** 数量保持32位类型并限制在0到9999，创建、读取、安全写入与存档恢复遵守同一边界。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxWideItemCountTest, "LxARPG.Items.CountWidth.RuntimeAndSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxWideItemCountTest::RunTest(const FString& Parameters)
{
    using namespace LxItemCountTest;
    FFixture Fixture(LxItemQuantity::Maximum);
    if (!TestTrue(TEXT("真实金币配置存在"), Fixture.bReady)) return false;
    TestEqual(TEXT("运行时数量保持32位整数"), sizeof(FLxItemCount), sizeof(int32));
    TestTrue(TEXT("数据表堆叠上限保持32位整数"), FindFProperty<FIntProperty>(FLxItemInformationBase::StaticStruct(), TEXT("ItemCountMax")) != nullptr);
    TestTrue(TEXT("数据表当前数量保持32位整数"), FindFProperty<FIntProperty>(FLxItemInformationBase::StaticStruct(), TEXT("ItemCount")) != nullptr);
    for (const int32 Count : {0, 1, 255, 256, LxItemQuantity::Maximum})
    {
        TestTrue(FString::Printf(TEXT("%d处于全局合法范围"), Count), LxItemQuantity::IsInRange(Count));
        TStrongObjectPtr<ULxItemBase> Gold(Fixture.MakeGold(Count));
        if (!TestNotNull(FString::Printf(TEXT("创建%d枚金币"), Count), Gold.Get())) continue;
        TestEqual(TEXT("数量接口没有截断"), Gold->ItemCount(), Count);
        TestEqual(TEXT("多件显示实际数量，零件及单件隐藏数量"), Gold->ItemCountText().ToFString(), Count > 1 ? FString::FromInt(Count) : FString());
        TestEqual(TEXT("完整信息与数量接口一致"), Gold->ItemInformation().ItemCount, Count);
        TestTrue(TEXT("合法数量能够安全写入"), Gold->CanSetItemCountSafely(Count));
        TestTrue(TEXT("实际数量能够安全清零"), Gold->SetItemCountSafely(0));
        TestTrue(TEXT("清零后能够安全写回合法数量"), Gold->SetItemCountSafely(Count));
        TestEqual(TEXT("安全写入后数量完整"), Gold->ItemCount(), Count);
        if (Count == 0)
        {
            TestFalse(TEXT("零数量实例表示已经耗尽"), Gold->ItemIsValid());
            continue;
        }
        FLxItemSaveRecord Record;
        if (!TestTrue(TEXT("采集完整数量"), LxItemSaveData::CaptureItem(Gold.Get(), Record))) continue;
        TestEqual(TEXT("存档记录保留完整数量"), Record.ItemCount, Count);
        TStrongObjectPtr<ULxItemBase> Restored(LxItemSaveData::CreateItem(Fixture.Instance.Get(), Record));
        if (TestNotNull(TEXT("大堆叠能够恢复"), Restored.Get())) TestEqual(TEXT("恢复数量无损"), Restored->ItemCount(), Count);
    }

    // 超范围输入必须被拒绝，不能截断、钳制或重建词条后覆盖已有运行时物品。
    TStrongObjectPtr<ULxItemBase> Existing(Fixture.MakeGold(256));
    if (!TestNotNull(TEXT("无效输入验证使用的原物品存在"), Existing.Get())) return false;
    const TArray<TObjectPtr<ULxEntryObjectBase>> OriginalEntries = Existing->GetItemEntryList();
    for (const int32 Count : {-1, LxItemQuantity::Maximum + 1, MAX_int32})
    {
        TestFalse(FString::Printf(TEXT("%d超出全局合法范围"), Count), LxItemQuantity::IsInRange(Count));
        TestNull(FString::Printf(TEXT("拒绝创建%d枚金币"), Count), Fixture.MakeGold(Count));
        TestFalse(TEXT("超范围数量不能安全写入"), Existing->CanSetItemCountSafely(Count));
        TestFalse(TEXT("拒绝超范围安全写入"), Existing->SetItemCountSafely(Count));
        TestEqual(TEXT("拒绝写入后原数量保留"), Existing->ItemCount(), 256);
        Existing->InitItemObject(FLxItemQuote(LxTag_Item_Material_Currency_Gold, Count));
        TestEqual(TEXT("拒绝重新初始化后原数量保留"), Existing->ItemCount(), 256);
        TestTrue(TEXT("拒绝重新初始化后原词条对象保留"), Existing->GetItemEntryList() == OriginalEntries);
        FLxItemSaveRecord Record;
        Record.ItemIDTag = LxTag_Item_Material_Currency_Gold;
        Record.ItemCount = Count;
        TestNull(TEXT("读档拒绝超范围数量"), LxItemSaveData::CreateItem(Fixture.Instance.Get(), Record));
    }

    // 编辑器外的静态配置加载同样限制数量与堆叠上限，零上限允许保留耗尽状态。
    FLxMaterialInformation ModifiedGold = Fixture.OriginalGold;
    ModifiedGold.ItemCountMax = MAX_int32;
    ModifiedGold.ItemCount = MAX_int32;
    LxItemConfig::SetMaterialItemData(ModifiedGold);
    const auto* ClampedGold = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
    if (!TestNotNull(TEXT("钳制后的金币配置存在"), ClampedGold)) return false;
    TestEqual(TEXT("静态配置堆叠上限钳制到9999"), ClampedGold->ItemCountMax, LxItemQuantity::Maximum);
    TestEqual(TEXT("静态配置数量钳制到9999"), ClampedGold->ItemCount, LxItemQuantity::Maximum);
    ModifiedGold.ItemCountMax = -1;
    ModifiedGold.ItemCount = -1;
    LxItemConfig::SetMaterialItemData(ModifiedGold);
    ClampedGold = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
    TestEqual(TEXT("静态配置负堆叠上限钳制到零"), ClampedGold->ItemCountMax, 0);
    TestEqual(TEXT("静态配置负数量钳制到零"), ClampedGold->ItemCount, 0);
    TStrongObjectPtr<ULxItemBase> EmptyGold(Fixture.MakeGold(0));
    if (TestNotNull(TEXT("零堆叠上限可创建零数量实例"), EmptyGold.Get()))
    {
        TestTrue(TEXT("零堆叠上限允许安全写入零"), EmptyGold->SetItemCountSafely(0));
        TestFalse(TEXT("零堆叠上限拒绝安全写入一"), EmptyGold->SetItemCountSafely(1));
        TestEqual(TEXT("拒绝写入后仍是零数量"), EmptyGold->ItemCount(), 0);
    }
    ModifiedGold.ItemCountMax = 256;
    ModifiedGold.ItemCount = LxItemQuantity::Maximum;
    LxItemConfig::SetMaterialItemData(ModifiedGold);
    ClampedGold = LxItemConfig::GetItemData(LxTag_Item_Material_Currency_Gold);
    TestEqual(TEXT("静态配置数量不超过物品自己的堆叠上限"), ClampedGold->ItemCount, 256);
    TStrongObjectPtr<ULxItemBase> SmallerStack(Fixture.MakeGold(255));
    if (TestNotNull(TEXT("小堆叠上限物品存在"), SmallerStack.Get()))
    {
        TestTrue(TEXT("可以安全写入物品的堆叠上限256"), SmallerStack->SetItemCountSafely(256));
        TestFalse(TEXT("257仍在全局范围内但超过本物品上限"), SmallerStack->SetItemCountSafely(257));
        TestEqual(TEXT("超过物品上限后原数量保留"), SmallerStack->ItemCount(), 256);
    }
    ModifiedGold.ItemCountMax = LxItemQuantity::Maximum;
    ModifiedGold.ItemCount = 0;
    LxItemConfig::SetMaterialItemData(ModifiedGold);

    // 消耗品经过相同数量参数初始化，使用一次应准确扣除一个，而不是在255处绕回。
    const auto& Consumables = LxItemConfig::GetConsumableItemMap();
    if (!TestTrue(TEXT("存在真实消耗品配置"), !Consumables.IsEmpty())) return false;
    const FLxConsumableInformation Original = Consumables.CreateConstIterator()->Value;
    FLxConsumableInformation Modified = Original;
    Modified.ItemCountMax = 9999;
    LxItemConfig::SetConsumableItemData(Modified);
    TStrongObjectPtr<ULxItemBase> Consumable(ULxItemBase::CreateItemObject(Fixture.Instance.Get(), FLxItemQuote(Original.ItemIDTag, 9999)));
    if (TestNotNull(TEXT("9999个消耗品实例存在"), Consumable.Get()))
    {
        TestEqual(TEXT("使用前数量完整"), Consumable->ItemCount(), 9999);
        TestEqual(TEXT("物品能够正常使用"), Consumable->ItemUse(), ELxItemUseState::ActivateEntry);
        TestEqual(TEXT("使用一次准确剩余9998"), Consumable->ItemCount(), 9998);
    }
    LxItemConfig::SetConsumableItemData(Original);

    /** 验证不可堆叠物品仍使用固定的一件配置，退出验证后立即恢复缓存。 */
    const auto VerifyNonStackable = [this](const auto& OriginalData, const auto SetData)
    {
        auto ModifiedData = OriginalData;
        ModifiedData.ItemCountMax = MAX_int32;
        ModifiedData.ItemCount = MAX_int32;
        SetData(ModifiedData);
        const auto* Normalized = LxItemConfig::GetItemData(OriginalData.ItemIDTag);
        if (TestNotNull(TEXT("不可堆叠物品配置存在"), Normalized))
        {
            TestEqual(TEXT("不可堆叠物品的最大堆叠数量仍为一"), Normalized->ItemCountMax, 1);
            TestEqual(TEXT("不可堆叠物品的配置数量仍为一"), Normalized->ItemCount, 1);
        }
        SetData(OriginalData);
    };
    const auto& Equipment = LxItemConfig::GetEquipmentItemMap();
    if (TestTrue(TEXT("存在真实装备配置"), !Equipment.IsEmpty()))
    {
        const FLxEquipmentInformation OriginalData = Equipment.CreateConstIterator()->Value;
        VerifyNonStackable(OriginalData, LxItemConfig::SetEquipmentItemData);
    }
    const auto& Buffs = LxItemConfig::GetBuffItemMap();
    if (TestTrue(TEXT("存在真实Buff配置"), !Buffs.IsEmpty()))
    {
        const FLxBuffInformation OriginalData = Buffs.CreateConstIterator()->Value;
        VerifyNonStackable(OriginalData, LxItemConfig::SetBuffItemData);
    }
    const auto& Skills = LxItemConfig::GetSkillItemMap();
    if (TestTrue(TEXT("存在真实技能配置"), !Skills.IsEmpty()))
    {
        const FLxSkillItemInformation OriginalData = Skills.CreateConstIterator()->Value;
        VerifyNonStackable(OriginalData, LxItemConfig::SetSkillItemData);
    }
    return true;
}

/** 覆盖9999枚金币的补满、余额检查、支付、复制快照、背包及交互容器恢复。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLargeGoldBackpackTest, "LxARPG.Items.CountWidth.Gold9999",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxLargeGoldBackpackTest::RunTest(const FString& Parameters)
{
    using namespace LxItemCountTest;
    FFixture Fixture(9999);
    if (!TestTrue(TEXT("真实金币配置存在"), Fixture.bReady)) return false;
    TStrongObjectPtr<ULxItemBase> Existing(Fixture.MakeGold(9000));
    TStrongObjectPtr<ULxItemBase> Incoming(Fixture.MakeGold(1234));
    if (!TestNotNull(TEXT("原堆叠存在"), Existing.Get()) || !TestNotNull(TEXT("新增堆叠存在"), Incoming.Get())) return false;
    TestTrue(TEXT("大堆叠正常合并"), Existing->ItemStack(Incoming.Get()));
    TestEqual(TEXT("原堆叠补满9999"), Existing->ItemCount(), 9999);
    TestEqual(TEXT("超出部分保留235"), Incoming->ItemCount(), 235);
    TStrongObjectPtr<ULxCharacterBackpackModule> Backpack(Fixture.MakeCharacterBackpack());
    if (!TestNotNull(TEXT("真实角色背包存在"), Backpack.Get())) return false;
    if (!TestTrue(TEXT("建立独立背包"), Backpack->RestoreBackpackSaveData(3, {}))) return false;
    TestTrue(TEXT("添加9999枚金币"), Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 9999)}));
    auto* GoldSlot = Backpack->GetBackpackSlotAt(0);
    if (!TestNotNull(TEXT("金币槽位存在"), GoldSlot) || !TestNotNull(TEXT("金币物品存在"), GoldSlot->GetItem())) return false;
    TestEqual(TEXT("背包数量读取9999"), GoldSlot->GetItem()->ItemCount(), 9999);
    TestTrue(TEXT("实际余额足够支付9999"), Backpack->CheckHaveItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 9999)}));
    TestFalse(TEXT("实际余额不足支付10000"), Backpack->CheckHaveItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 10000)}));
    TestTrue(TEXT("支付2500金币成功"), Backpack->RemoveItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 2500)}));
    TestEqual(TEXT("支付后余额7499"), GoldSlot->GetItem()->ItemCount(), 7499);
    TestTrue(TEXT("再次添加4000正确补满并拆堆"), Backpack->AddItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 4000)}));
    TestEqual(TEXT("第一堆金币9999"), GoldSlot->GetItem()->ItemCount(), 9999);
    TestEqual(TEXT("第二堆金币1500"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 1500);

    const auto* Property = FindFProperty<FArrayProperty>(Backpack->GetClass(), TEXT("ReplicatedBackpackSlots"));
    if (!TestNotNull(TEXT("数量复制快照属性存在"), Property)) return false;
    const auto* Replicated = Property->ContainerPtrToValuePtr<TArray<FLxItemSaveRecord>>(Backpack.Get());
    if (TestEqual(TEXT("复制快照包含全部槽位"), Replicated->Num(), 3))
    {
        TestEqual(TEXT("第一堆网络数量完整"), (*Replicated)[0].ItemCount, 9999);
        TestEqual(TEXT("第二堆网络数量完整"), (*Replicated)[1].ItemCount, 1500);
    }
    TArray<FLxItemSlotSaveRecord> Saved;
    TestTrue(TEXT("采集背包数量"), LxItemSaveData::CaptureSlots(Backpack->GetAllItems(), Saved));
    TStrongObjectPtr<ULxCharacterBackpackModule> Restored(NewObject<ULxCharacterBackpackModule>(Fixture.Instance.Get()));
    TestTrue(TEXT("恢复完整背包"), Restored->RestoreBackpackSaveData(3, Saved));
    TestTrue(TEXT("恢复后11499金币实际可用"), Restored->CheckHaveItemList({FLxItemQuote(LxTag_Item_Material_Currency_Gold, 11499)}));
    TArray<FLxInteractionItemSaveRecord> ContainerRecords;
    TestTrue(TEXT("采集交互容器数量"), LxInteractionSaveHelpers::CaptureSlots(Backpack->GetAllItems(), ContainerRecords));
    TArray<TObjectPtr<ULxItemSlotData>> ContainerSlots;
    TestTrue(TEXT("仓库及宝箱能够恢复超过255的堆叠"), LxInteractionSaveHelpers::BuildRestoredSlots(
        Fixture.Instance.Get(), ContainerRecords, 3, ELxItemSlotType::Warehouse, ContainerSlots));
    if (TestEqual(TEXT("交互容器槽位完整"), ContainerSlots.Num(), 3))
        TestEqual(TEXT("交互容器金币数量9999"), ContainerSlots[0]->GetItem()->ItemCount(), 9999);
    return true;
}

/** 批量请求可以超过单堆上限，分堆、容量不足回退、累计余额和扣除均保持数量完整。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxLargeCountCapacityTest, "LxARPG.Items.CountWidth.AggregatedCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxLargeCountCapacityTest::RunTest(const FString& Parameters)
{
    using namespace LxItemCountTest;
    FFixture Fixture(LxItemQuantity::Maximum);
    if (!TestTrue(TEXT("真实金币配置存在"), Fixture.bReady)) return false;
    TStrongObjectPtr<ULxCharacterBackpackModule> Backpack(NewObject<ULxCharacterBackpackModule>(Fixture.Instance.Get()));
    if (!TestTrue(TEXT("建立三个空槽位"), Backpack->RestoreBackpackSaveData(3, {}))) return false;
    const FLxItemQuote TwentyThousand(LxTag_Item_Material_Currency_Gold, 20000);
    TestTrue(TEXT("20000批量请求能够分为三堆"), Backpack->CanAddItemList({TwentyThousand}));
    if (!TestTrue(TEXT("添加20000金币不丢量"), Backpack->AddItemList({TwentyThousand}))) return false;
    for (int32 Index = 0; Index < 3; ++Index)
        if (!TestNotNull(FString::Printf(TEXT("第%d堆金币存在"), Index + 1), Backpack->GetBackpackSlotAt(Index)->GetItem())) return false;
    TestEqual(TEXT("第一堆9999金币"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 9999);
    TestEqual(TEXT("第二堆9999金币"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 9999);
    TestEqual(TEXT("第三堆保留2金币"), Backpack->GetBackpackSlotAt(2)->GetItem()->ItemCount(), 2);

    // 剩余容量只有9997，累计请求达到9998时应整体失败并保留全部已有堆叠。
    const TArray<FLxItemQuote> Excess{
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 5000),
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 4998)};
    TestFalse(TEXT("累计请求超过剩余容量"), Backpack->CanAddItemList(Excess));
    TestFalse(TEXT("容量不足时添加整体失败"), Backpack->AddItemList(Excess));
    TestEqual(TEXT("添加失败后第一堆保留9999"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 9999);
    TestEqual(TEXT("添加失败后第二堆保留9999"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 9999);
    TestEqual(TEXT("添加失败后第三堆保留2"), Backpack->GetBackpackSlotAt(2)->GetItem()->ItemCount(), 2);

    // 单条非法存档也不能替换已存在的合法背包数据。
    for (const int32 Count : {-1, LxItemQuantity::Maximum + 1, MAX_int32})
    {
        FLxItemSlotSaveRecord Invalid;
        Invalid.SlotIndex = 0;
        Invalid.SlotTag = LxTag_Item;
        Invalid.Item.ItemIDTag = LxTag_Item_Material_Currency_Gold;
        Invalid.Item.ItemCount = Count;
        TestFalse(TEXT("拒绝包含越界数量的背包存档"), Backpack->RestoreBackpackSaveData(3, {Invalid}));
        TestEqual(TEXT("恢复失败后第一堆保留9999"), Backpack->GetBackpackSlotAt(0)->GetItem()->ItemCount(), 9999);
        TestEqual(TEXT("恢复失败后第二堆保留9999"), Backpack->GetBackpackSlotAt(1)->GetItem()->ItemCount(), 9999);
        TestEqual(TEXT("恢复失败后第三堆保留2"), Backpack->GetBackpackSlotAt(2)->GetItem()->ItemCount(), 2);
    }

    const TArray<FLxItemQuote> TooMuch{
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 10000),
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 10001)};
    TestFalse(TEXT("累计扣除需求20001超过余额20000"), Backpack->CheckHaveItemList(TooMuch));
    TestFalse(TEXT("余额不足时扣除整体失败"), Backpack->RemoveItemList(TooMuch));
    TestTrue(TEXT("扣除失败后20000金币仍完整可用"), Backpack->CheckHaveItemList({TwentyThousand}));
    const TArray<FLxItemQuote> All{
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 10000),
        FLxItemQuote(LxTag_Item_Material_Currency_Gold, 10000)};
    TestTrue(TEXT("跨堆累计20000余额判断准确"), Backpack->CheckHaveItemList(All));
    TestTrue(TEXT("批次能够完整扣除跨堆总量20000"), Backpack->RemoveItemList(All));
    TestFalse(TEXT("第一堆耗尽"), Backpack->GetBackpackSlotAt(0)->IsValid());
    TestFalse(TEXT("第二堆耗尽"), Backpack->GetBackpackSlotAt(1)->IsValid());
    TestFalse(TEXT("第三堆耗尽"), Backpack->GetBackpackSlotAt(2)->IsValid());
    return true;
}
#endif
