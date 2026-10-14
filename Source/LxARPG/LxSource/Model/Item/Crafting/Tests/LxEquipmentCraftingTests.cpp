#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Slate/SObjectWidget.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManager.h"
#include "LxARPG/LxSource/UI/Manager/LxUIManagerTypes.h"
#include "LxARPG/LxSource/UI/Manager/LxManufacturingUIManager.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "Kismet/GameplayStatics.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxEquipmentCraftingTypes.h"
#include "LxARPG/LxSource/Model/Item/Crafting/LxItemManufacturingComponent.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Item/DataType/Slot/LxItemSlotData.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Content/Logic/LxCharacterContentComponent.h"
#include "LxARPG/LxSource/Model/Item/Logic/LxCharacterBackpackComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractableComponent.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxInteractionNode.h"
#include "LxARPG/LxSource/Model/Interaction/Logic/LxFunctionPageInteractionComponent.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionTreeAsset.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerControlComponent.h"
#include "LxARPG/LxSource/Model/PlayerControl/Logic/LxPlayerInteractionModule.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxGameSaveData.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxInteractionSaveHelpers.h"
#include "LxARPG/LxSource/UI/Crafting/LxEquipmentCraftingWidget.h"
#include "UObject/UnrealType.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectComponent.h"
#include "LxARPG/LxSource/Model/Effect/Logic/LxCharacterEffectCacheComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"

/** 验证锻造曲线和随机规则，不依赖真实随机运气。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCraftRulesTest, "LxARPG.EquipmentCrafting.Rules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCraftRulesTest::RunTest(const FString& Parameters)
{
    using namespace LxEquipmentCrafting;
    const FLxEquipmentCraftService Default;
    auto Q = CalculateQuote(ELxEquipmentCraftOperation::Enhance, 0.f, 0, Default);
    TestEqual(TEXT("初始强化100%"), Q.SuccessChance, 1.f); TestEqual(TEXT("初始消耗1金币"), Q.GoldCost, 1);
    Q = CalculateQuote(ELxEquipmentCraftOperation::Enhance, .6f, 0, Default);
    TestTrue(TEXT("60%强化时低于10%成功率"), Q.SuccessChance < .1f && Q.SuccessChance > .01f);
    Q = CalculateQuote(ELxEquipmentCraftOperation::Enhance, 1.f, 0, Default);
    TestEqual(TEXT("100%强化时50金币"), Q.GoldCost, 50); TestEqual(TEXT("最低基础成功率1%"), Q.SuccessChance, .01f);
    Q = CalculateQuote(ELxEquipmentCraftOperation::Enhance, 2.f, 0, Default);
    TestEqual(TEXT("200%保持1%成功率"), Q.SuccessChance, .01f); TestEqual(TEXT("费用线性增长99"), Q.GoldCost, 99);
    Q = CalculateQuote(ELxEquipmentCraftOperation::AddEntry, 0.f, 10, Default);
    TestEqual(TEXT("10词条新增1%"), Q.SuccessChance, .01f); TestEqual(TEXT("10词条50金币"), Q.GoldCost, 50);
    FLxEquipmentCraftService Bonus; Bonus.SuccessMultiplier = 2.f; Bonus.GoldMultiplier = .5f;
    Q = CalculateQuote(ELxEquipmentCraftOperation::Enhance, 1.f, 0, Bonus);
    TestEqual(TEXT("NPC乘成功率"), Q.SuccessChance, .02f); TestEqual(TEXT("NPC乘金币费用"), Q.GoldCost, 25);
    TestEqual(TEXT("NPC不影响调整概率"), CalculateQuote(ELxEquipmentCraftOperation::AdjustEntries, 1.f, 5, Bonus).SuccessChance, .5f);
    TestEqual(TEXT("移除至少一条概率"), CalculateQuote(ELxEquipmentCraftOperation::RemoveEntry, 0.f, 2, Bonus).SuccessChance, .75f);
    FRandomStream Random(78123);
    for (int32 Index = 0; Index < 1000; ++Index)
    {
        TestEqual(TEXT("30%失败不扣值"), RollEnhancement(.3f, false, Random), .3f);
        const float Gain = RollEnhancement(1.f, true, Random) - 1.f;
        TestTrue(TEXT("可超过100%且增益1%-10%"), Gain > .009f && Gain < .101f);
        const float Loss = .31f - RollEnhancement(.31f, false, Random);
        TestTrue(TEXT("30%以上失败扣1%-10%"), Loss > .009f && Loss < .101f);
        const float Ratio = RollProportion(.5f, Random);
        TestTrue(TEXT("词条调整幅度不超过20个百分点"), Ratio >= .3f && Ratio <= .7f);
        TestTrue(TEXT("比例不会负数"), RollProportion(0.f, Random) >= 0.f);
        const float AtFull = RollProportion(1.f, Random);
        TestTrue(TEXT("100%附近仍按20个百分点调整"), AtFull >= .8f && AtFull <= 1.2f);
    }
    // 以固定种子验证达到及超过100%后均能双向调整，避免被上限截断。
    int32 Increases = 0, Decreases = 0, CrossedFull = 0;
    for (int32 Index = 0; Index < 20000; ++Index)
    {
        const float Ratio = RollProportion(1.5f, Random);
        TestTrue(TEXT("超过100%后调整幅度仍不超过20个百分点"), Ratio >= 1.3f && Ratio <= 1.7f);
        Increases += Ratio > 1.5f;
        Decreases += Ratio < 1.5f;
        CrossedFull += RollProportion(1.f, Random) > 1.f;
    }
    TestTrue(TEXT("超过100%的非零提高结果符合50%方向概率"), Increases > 9000 && Increases < 10000);
    TestTrue(TEXT("超过100%的非零降低结果符合50%方向概率"), Decreases > 9000 && Decreases < 10000);
    TestTrue(TEXT("100%词条能够被调整到100%以上"), CrossedFull > 9000 && CrossedFull < 10000);
    int32 Hits[3] = {0, 0, 0};
    for (int32 Index = 0; Index < 20000; ++Index) { const int32 Hit = RollRemoval(2, Random); ++Hits[Hit == INDEX_NONE ? 2 : Hit]; }
    TestTrue(TEXT("首词条约50%被移除"), Hits[0] > 9500 && Hits[0] < 10500);
    TestTrue(TEXT("第二词条总体约25%"), Hits[1] > 4500 && Hits[1] < 5500);
    TestTrue(TEXT("约25%没有移除"), Hits[2] > 4500 && Hits[2] < 5500);
    TestEqual(TEXT("1条普通"), ULxEquipment::CalculateRarity(false, 1), ELxItemRarityType::Ordinary);
    TestEqual(TEXT("2条高级"), ULxEquipment::CalculateRarity(false, 2), ELxItemRarityType::Advanced);
    TestEqual(TEXT("8条稀有"), ULxEquipment::CalculateRarity(false, 8), ELxItemRarityType::Rare);
    TestEqual(TEXT("12条神话"), ULxEquipment::CalculateRarity(false, 12), ELxItemRarityType::Mythic);
    TestEqual(TEXT("固定7条传说"), ULxEquipment::CalculateRarity(true, 7), ELxItemRarityType::Legend);
    TestEqual(TEXT("固定8条史诗"), ULxEquipment::CalculateRarity(true, 8), ELxItemRarityType::Epic);
    return true;
}

/** 验证最小装备存档、基础词条静态恢复、固定装备和两个相同装备ID的独立性。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCraftSaveTest, "LxARPG.EquipmentCrafting.SaveRoundTrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCraftSaveTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>(); Instance->Init();
    if (auto* Subsystem = Instance->GetSubsystem<ULxGameInstanceSubsystem>()) if (Subsystem->GetSaveManager()) Subsystem->GetSaveManager()->SetReadOnly(true);
    ON_SCOPE_EXIT { Instance->Shutdown(); };
    if (!TestFalse(TEXT("存在装备配置"), LxItemConfig::GetEquipmentItemMap().IsEmpty())) return false;
    const FLxEquipmentInformation Original = LxItemConfig::GetEquipmentItemMap().CreateConstIterator().Value();
    ON_SCOPE_EXIT { LxItemConfig::SetEquipmentItemData(Original); };
    FLxEquipmentInformation Config = Original;
    Config.bFixedNormalEntries = false;
    const auto IDs = LxEntryConfig::GetEquipmentForgeEntryIDs();
    if (!TestTrue(TEXT("存在锻造词条"), IDs.Num() > 1)) return false;
    FLxEntryQuote Base; Base.EntryID = IDs[0];
    Config.ItemEntryConfigs = { FLxItemEntryConfig(Base, ELxEntryLogicType::Base),
        FLxItemEntryConfig(FLxEntryQuote(), ELxEntryLogicType::Base), FLxItemEntryConfig(FLxEntryQuote(), ELxEntryLogicType::Normal) };
    LxItemConfig::SetEquipmentItemData(Config);
    auto* Item = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(Config.ItemIDTag, 1)));
    if (!TestNotNull(TEXT("创建独立装备"), Item)) return false;
    TestEqual(TEXT("未配置ID的静态占位不计入普通词条"), Item->GetNormalEntries().Num(), 0);
    FLxEquipmentEntry Entry; Entry.EntryID = IDs[1]; Entry.Proportion = 1.42f;
    TestTrue(TEXT("修改普通词条与强化度"), Item->ApplyCraftingState(1.25f, {Entry}));
    FLxItemSaveRecord Saved;
    TestTrue(TEXT("导出装备专用记录"), LxItemSaveData::CaptureItem(Item, Saved));
    TestTrue(TEXT("不保存通用词条CD或基础词条"), Saved.Entries.IsEmpty());
    TestEqual(TEXT("只保存一个动态普通词条"), Saved.Equipment.NormalEntries.Num(), 1);
    auto* Copy = Cast<ULxEquipment>(LxItemSaveData::CreateItem(Instance, Saved));
    if (!TestNotNull(TEXT("重新创建装备"), Copy)) return false;
    TestEqual(TEXT("原始生效比例未被强化污染"), Copy->GetNormalEntries()[0].Proportion, 1.42f);
    TestEqual(TEXT("基础词条缓存保留原始比例"), Copy->GetItemEntryList()[0]->GetEntryQuote().EntryProportion, 1.f);
    TestEqual(TEXT("普通词条缓存保留原始比例"), Copy->GetItemEntryList()[1]->GetEntryQuote().EntryProportion, 1.42f);
    TestEqual(TEXT("静态基础词条不变"), LxItemConfig::GetItemData(Config.ItemIDTag)->ItemEntryConfigs[0].EntryQuote.EntryProportion, 1.f);
    // 仓库、宝箱与商人使用各自的交互档，装备状态不得退化为静态配置。
    auto* ContainerSlot = NewObject<ULxItemSlotData>(Instance);
    ContainerSlot->InitItemSlot(ELxItemSlotType::Warehouse, LxTag_Item, Item);
    TArray<TObjectPtr<ULxItemSlotData>> ContainerSlots = {ContainerSlot};
    TArray<FLxInteractionItemSaveRecord> ContainerRecords;
    TestTrue(TEXT("容器采集动态装备"), LxInteractionSaveHelpers::CaptureSlots(ContainerSlots, ContainerRecords));
    TArray<TObjectPtr<ULxItemSlotData>> RestoredContainer;
    TestTrue(TEXT("容器独立恢复动态装备"), LxInteractionSaveHelpers::BuildRestoredSlots(Instance,
        ContainerRecords, 1, ELxItemSlotType::Warehouse, RestoredContainer));
    auto* ContainerCopy = RestoredContainer.IsEmpty() ? nullptr : Cast<ULxEquipment>(RestoredContainer[0]->GetItem());
    if (!TestNotNull(TEXT("容器中的装备存在"), ContainerCopy)) return false;
    TestEqual(TEXT("容器保留强化"), ContainerCopy->GetEnhancement(), 1.25f);
    TestEqual(TEXT("容器保留普通词条比例"), ContainerCopy->GetNormalEntries()[0].Proportion, 1.42f);
    TestEqual(TEXT("容器保留装备身份"), ContainerCopy->GetInstanceID(), Item->GetInstanceID());
    ULxGameSaveData* Save = NewObject<ULxGameSaveData>();
    auto& Character = Save->Players.Add(Config.ItemIDTag);
    Character.SaveID = Config.ItemIDTag;
    auto& Slot = Character.BackpackSlots.AddDefaulted_GetRef(); Slot.SlotIndex = 0; Slot.Item = Saved;
    auto* Second = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(Config.ItemIDTag, 1)));
    auto& Equipped = Character.EquipmentSlots.AddDefaulted_GetRef(); Equipped.SlotIndex = 0; LxItemSaveData::CaptureItem(Second, Equipped.Item);
    TestTrue(TEXT("提取两件同种装备"), Character.ExtractEquipmentInstances());
    TestEqual(TEXT("两件装备使用不同身份"), Character.EquipmentInstances.Num(), 2);
    TestFalse(TEXT("角色槽位只保存实例引用"), Character.BackpackSlots[0].Item.ItemIDTag.IsValid());
    TArray<uint8> Bytes; UGameplayStatics::SaveGameToMemory(Save, Bytes);
    auto* Loaded = Cast<ULxGameSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("真实序列化成功"), Loaded)) return false;
    auto& LoadedCharacter = Loaded->Players.FindChecked(Config.ItemIDTag);
    TArray<FLxItemSlotSaveRecord> Backpack, Equipment;
    TestTrue(TEXT("解析独立装备集合"), LoadedCharacter.ResolveEquipmentInstances(Backpack, Equipment));
    TestEqual(TEXT("两件装备强化度独立"), Backpack[0].Item.Equipment.Enhancement, 1.25f);
    TestEqual(TEXT("第二件未强化"), Equipment[0].Item.Equipment.Enhancement, 0.f);
    TestEqual(TEXT("角色序列化保留超过100%的普通词条"), Backpack[0].Item.Equipment.NormalEntries[0].Proportion, 1.42f);
    auto* LoadedEquipment = Cast<ULxEquipment>(LxItemSaveData::CreateItem(Instance, Backpack[0].Item));
    if (TestNotNull(TEXT("角色读档恢复超过100%的词条"), LoadedEquipment))
        TestEqual(TEXT("角色读档不截断比例"), LoadedEquipment->GetNormalEntries()[0].Proportion, 1.42f);
    FLxEquipmentEntry InvalidEntry = Entry; InvalidEntry.Proportion = -.01f;
    TestFalse(TEXT("比例下限仍为0%"), Item->ApplyCraftingState(1.25f, {InvalidEntry}));
    TestEqual(TEXT("无效调整不改变已有比例"), Item->GetNormalEntries()[0].Proportion, 1.42f);
    LoadedCharacter.EquipmentInstances.Remove(Saved.EquipmentInstanceID);
    TestFalse(TEXT("缺失引用不能静默生成默认装备"), LoadedCharacter.ResolveEquipmentInstances(Backpack, Equipment));
    Config.bFixedNormalEntries = true;
    Base.EntryProportion = 1.75f;
    Config.ItemEntryConfigs.Emplace(Base, ELxEntryLogicType::Normal);
    LxItemConfig::SetEquipmentItemData(Config);
    auto* Fixed = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(Config.ItemIDTag, 1)));
    TestTrue(TEXT("固定词条可以强化"), Fixed->ApplyCraftingState(.5f, Fixed->GetNormalEntries()));
    TestFalse(TEXT("固定词条不允许移除"), Fixed->ApplyCraftingState(.5f, {}));
    LxItemSaveData::CaptureItem(Fixed, Saved);
    TestTrue(TEXT("固定装备不存词条"), Saved.Equipment.NormalEntries.IsEmpty());
    auto* FixedCopy = Cast<ULxEquipment>(LxItemSaveData::CreateItem(Instance, Saved));
    TestNotNull(TEXT("固定装备由静态配置恢复"), FixedCopy);
    if (FixedCopy)
    {
        TestEqual(TEXT("恢复固定普通词条"), FixedCopy->GetNormalEntries().Num(), 1);
        TestEqual(TEXT("固定静态词条也允许超过100%"), FixedCopy->GetNormalEntries()[0].Proportion, 1.75f);
    }
    for (int32 Index = 0; Index < 24; ++Index) Config.ItemEntryConfigs.Emplace(Base, ELxEntryLogicType::Normal);
    LxItemConfig::SetEquipmentItemData(Config);
    auto* Capped = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(Config.ItemIDTag, 1)));
    TestEqual(TEXT("静态配置同样最多20条普通词条"), Capped->GetNormalEntries().Num(), 20);
    LxItemSaveData::CaptureItem(Capped, Saved);
    auto* CappedCopy = Cast<ULxEquipment>(LxItemSaveData::CreateItem(Instance, Saved));
    if (TestNotNull(TEXT("超过上限的静态配置仍可按上限读档"), CappedCopy)) TestEqual(TEXT("读档仍保持20条"), CappedCopy->GetNormalEntries().Num(), 20);
    return true;
}

/** 通过真实角色的数据中转及效果缓存检查强化只在生成效果时应用一次。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCraftEnhancementEffectsTest, "LxARPG.EquipmentCrafting.EnhancementEffects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCraftEnhancementEffectsTest::RunTest(const FString& Parameters)
{
    UGameInstance* Instance = NewObject<UGameInstance>(); Instance->Init();
    if (auto* Subsystem = Instance->GetSubsystem<ULxGameInstanceSubsystem>()) if (Subsystem->GetSaveManager()) Subsystem->GetSaveManager()->SetReadOnly(true);
    ON_SCOPE_EXIT { Instance->Shutdown(); };
    if (!TestTrue(TEXT("存在装备配置"), !LxItemConfig::GetEquipmentItemMap().IsEmpty())) return false;
    const FLxEquipmentInformation Original = LxItemConfig::GetEquipmentItemMap().CreateConstIterator().Value();
    ON_SCOPE_EXIT { LxItemConfig::SetEquipmentItemData(Original); };
    TArray<FLxEntryAttributeGain> OriginalEntries;
    for (const FGameplayTag EntryID : LxEntryConfig::GetEquipmentForgeEntryIDs())
    {
        const auto* Entry = LxEntryConfig::GetEntryData(EntryID);
        if (Entry && Entry->EntryType == ELxEntryType::AttributeGain)
        {
            OriginalEntries.Add(*static_cast<const FLxEntryAttributeGain*>(Entry));
            if (OriginalEntries.Num() == 2) break;
        }
    }
    if (!TestEqual(TEXT("存在两个真实属性增益词条"), OriginalEntries.Num(), 2)) return false;
    ON_SCOPE_EXIT { for (const auto& Entry : OriginalEntries) LxEntryConfig::SetAttributeGainEntryData(Entry); };
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FLxEntryAttributeGain Entry = OriginalEntries[Index];
        Entry.AttributeIDTag = LxTag_Attribute_Ability_Strength;
        Entry.EntryTarget = ELxEntryTarget::ToValue;
        Entry.EffectiveType = ELxEntryEffectiveType::BasicValue;
        Entry.EntryValue = Index == 0 ? 40.f : 20.f;
        Entry.TargetBusinessCategories.Reset();
        LxEntryConfig::SetAttributeGainEntryData(Entry);
    }
    FLxEquipmentInformation Config = Original;
    Config.bFixedNormalEntries = false;
    FLxEntryQuote Base; Base.EntryID = OriginalEntries[0].EntryID;
    Config.ItemEntryConfigs = {FLxItemEntryConfig(Base, ELxEntryLogicType::Base)};
    LxItemConfig::SetEquipmentItemData(Config);
    auto* Item = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(Config.ItemIDTag, 1)));
    if (!TestNotNull(TEXT("强化测试装备创建成功"), Item)) return false;
    FLxEquipmentEntry Normal; Normal.EntryID = OriginalEntries[1].EntryID; Normal.Proportion = 1.42f;
    if (!TestTrue(TEXT("设置125%强化和142%普通词条比例"), Item->ApplyCraftingState(1.25f, {Normal}))) return false;
    if (!TestEqual(TEXT("基础及普通词条均已建立"), Item->GetItemEntryList().Num(), 2)) return false;
    TestEqual(TEXT("装备效果倍率为2.25"), Item->GetItemEntryEffectMultiplier(), 2.25f);

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->SetGameInstance(Instance); World->InitializeActorsForPlay(FURL());
    ON_SCOPE_EXIT { World->SetGameInstance(nullptr); World->DestroyWorld(false); };
    auto* Player = World->SpawnActor<ALxPlayerCharacter>();
    if (!TestNotNull(TEXT("创建真实效果测试角色"), Player)) return false;
    Player->GetCharacterContentComponent()->BaseComponentInitialize();
    Player->GetCharacterAttributeComponent()->BaseComponentInitialize();
    Player->GetCharacterEffectComponent()->BaseComponentInitialize();
    auto* Transfer = Player->GetCharacterDataTransferComponent(); Transfer->BaseComponentInitialize();
    auto* Cache = Player->GetCharacterEffectCacheComponent();
    if (!TestNotNull(TEXT("真实角色效果缓存模块存在"), Cache)) return false;
    const auto* CacheProperty = FindFProperty<FMapProperty>(Cache->GetClass(), TEXT("CachedEffectPackages"));
    if (!TestNotNull(TEXT("真实持续效果缓存可读取"), CacheProperty)) return false;
    const auto* CachedPackages = CacheProperty->ContainerPtrToValuePtr<TMap<FName, FLxEffectPackage>>(Cache);
    /** 读取已由正式中转链提交的装备增益，不调用词条自身的效果转换代替生产链。 */
    const auto ReadEquipmentGain = [CachedPackages]()
    {
        float Gain = 0.f;
        for (const auto& Pair : *CachedPackages)
            if (Pair.Value.SourceContext.SourceType == ELxEffectPackageSource::Equipment)
                for (const auto& Effect : Pair.Value.AttributeModifierEffects)
                    if (Effect.AttributeIDTag == LxTag_Attribute_Ability_Strength) Gain += Effect.ModifierValue;
        return Gain;
    };
    FLxCharacterEntryPackage Package;
    Package.EntrySource = ELxCharacterEntrySource::Equipment;
    Package.EntryList = Item->GetItemEntryList();
    Transfer->ReceiveEntryPackage(Package);
    TestEqual(TEXT("实际效果为(40+20乘1.42)乘2.25"), ReadEquipmentGain(), 153.9f, .001f);
    Transfer->ReceiveEntryPackage(Package);
    TestEqual(TEXT("重复刷新替换同来源且不累乘强化"), ReadEquipmentGain(), 153.9f, .001f);
    TestEqual(TEXT("基础词条缓存仍是原始比例1"), Item->GetItemEntryList()[0]->GetEntryQuote().EntryProportion, 1.f);
    TestEqual(TEXT("普通词条缓存仍是原始比例1.42"), Item->GetItemEntryList()[1]->GetEntryQuote().EntryProportion, 1.42f);
    TestTrue(TEXT("强化改变为50%"), Item->ApplyCraftingState(.5f, {Normal}));
    Package.EntryList = Item->GetItemEntryList();
    Transfer->ReceiveEntryPackage(Package);
    TestEqual(TEXT("新的实际效果使用倍率1.5"), ReadEquipmentGain(), 102.6f, .001f);
    Package.EntryList.Reset();
    Transfer->ReceiveEntryPackage(Package);
    TestEqual(TEXT("撤回装备词条移除旧增益"), ReadEquipmentGain(), 0.f);
    return true;
}

/** 使用真实玩家、NPC交互模块和金币背包检查收费、防越权和操作效果。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxCraftTransactionTest, "LxARPG.EquipmentCrafting.Transactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxCraftTransactionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    UGameInstance* Instance = NewObject<UGameInstance>(); World->SetGameInstance(Instance); Instance->Init();
    if (auto* Subsystem = Instance->GetSubsystem<ULxGameInstanceSubsystem>()) if (Subsystem->GetSaveManager()) Subsystem->GetSaveManager()->SetReadOnly(true);
    ON_SCOPE_EXIT { Instance->Shutdown(); World->SetGameInstance(nullptr); };
    auto* Player = World->SpawnActor<ALxPlayerCharacter>();
    Player->GetCharacterContentComponent()->BaseComponentInitialize(); Player->GetPlayerControlComponent()->BaseComponentInitialize();
    auto* Backpack = Player->GetCharacterBackpackComponent(); auto* Craft = Player->GetItemManufacturingComponent();
    const FLxEquipmentInformation Original = LxItemConfig::GetEquipmentItemMap().CreateConstIterator().Value();
    ON_SCOPE_EXIT { LxItemConfig::SetEquipmentItemData(Original); };
    FLxEquipmentInformation Config = Original; Config.bFixedNormalEntries = false; Config.ItemEntryConfigs.Reset(); LxItemConfig::SetEquipmentItemData(Config);
    Backpack->AddItemList({ FLxItemQuote(Config.ItemIDTag, 1), FLxItemQuote(LxTag_Item_Material_Currency_Gold, 200) });
    ULxItemSlotData* Slot = Backpack->GetBackpackSlotAt(0);
    auto* Item = Cast<ULxEquipment>(Slot->GetItem());
    if (!TestNotNull(TEXT("玩家装备准备成功"), Item)) return false;
    auto* NPC = World->SpawnActor<AActor>(); auto* Provider = NewObject<ULxInteractableComponent>(NPC);
    Provider->FeatureConfig.bEnableFunctionPage = true;
    Provider->FeatureConfig.FunctionPageConfig.PageWidgetClass = ULxEquipmentCraftingWidget::StaticClass();
    auto* Tree = NewObject<ULxInteractionTreeAsset>(); Tree->Features.bEnableFunctionPage = true;
    auto* Node = NewObject<ULxInteractionTreeNodeData>(Tree); Node->NodeId = FGuid::NewGuid(); Node->Type = ELxInteractionActionType::FunctionPage;
    Tree->Nodes.Add(Node); Tree->Roots.Add(Node->NodeId);
    TestTrue(TEXT("NPC加载功能节点"), Provider->SetInteractionTreeAsset(Tree));
    auto* Feature = Cast<ULxFunctionPageInteractionComponent>(Provider->GetRootInteractionNodes()[0]->GetInteractionFeature());
    Player->GetPlayerInteractionComponent()->AddInteractableComponent(Provider);
    TestTrue(TEXT("打开玩家制造服务"), Craft->BeginEquipmentService(Feature));
    TestTrue(TEXT("允许拖入玩家装备"), Craft->SelectEquipment(Slot));
    auto* Foreign = NewObject<ULxItemSlotData>(Provider); Foreign->InitItemSlot(ELxItemSlotType::Transaction, LxTag_Item, Item);
    TestFalse(TEXT("相同物品的商店槽位也不允许越权拖入"), Craft->SelectEquipment(Foreign));
    // 使用实际蓝图和Slate拖放路径，验证放大格子接收拖入、按钮绑定与报价刷新。
    UClass* PageClass = LoadClass<ULxEquipmentCraftingWidget>(nullptr,
        TEXT("/Game/项目内容/UI/角色UI界面/制造界面/装备打造.装备打造_C"));
    if (!TestNotNull(TEXT("实际打造界面蓝图"), PageClass)) return false;
    auto* Page = CreateWidget<ULxEquipmentCraftingWidget>(World, PageClass);
    auto* UIManager = NewObject<ULxUIManager>(World); UIManager->Initialize();
    TestTrue(TEXT("主布局管理器接受制造层注册"), UIManager->RegisterManufacturingWidget(Page));
    TestTrue(TEXT("页面归属新增制造层"), UIManager->GetManufacturingUIManager()->ContainsWidget(Page));
    Page->TakeWidget(); Page->SetManufacturingComponent(Craft);
    ON_SCOPE_EXIT { Page->SetManufacturingComponent(nullptr); Page->ReleaseSlateResources(true); };
    auto* Grid = Cast<ULxCraftingGridWidget>(Page->WidgetTree->FindWidget(TEXT("EquipmentGrid")));
    auto* EnhanceButton = Cast<UButton>(Page->WidgetTree->FindWidget(TEXT("EnhanceButton")));
    if (!TestNotNull(TEXT("放大装备格子已绑定"), Grid) || !TestNotNull(TEXT("强化按钮已绑定"), EnhanceButton)) return false;
    Craft->ClearSelection();
    auto* Source = NewObject<ULxItemGridWidget>(World); Source->Initialize(); Source->SetItemSlotData(Slot);
    const auto SourceSlate = StaticCastSharedRef<SObjectWidget>(Source->TakeWidget());
    const auto TargetSlate = StaticCastSharedRef<SObjectWidget>(Grid->TakeWidget());
    const FGeometry Geometry = FGeometry::MakeRoot(FVector2D(128, 128), FSlateLayoutTransform());
    const FPointerEvent Press(0, FVector2D(10, 10), FVector2D(5, 5), TSet<FKey>{EKeys::LeftMouseButton}, EKeys::LeftMouseButton, 0, FModifierKeysState());
    const FReply DragReply = SourceSlate->OnDragDetected(Geometry, Press);
    if (!TestTrue(TEXT("原物品格子创建拖拽"), DragReply.GetDragDropContent().IsValid())) return false;
    const FDragDropEvent Drop(Press, DragReply.GetDragDropContent());
    TestTrue(TEXT("实际打造格子接收拖拽"), TargetSlate->OnDrop(Geometry, Drop).IsEventHandled());
    TestTrue(TEXT("拖入后仍持有原装备"), Craft->GetSelectedEquipment() == Item && Slot->GetItem() == Item);
    TestTrue(TEXT("合法报价启用强化按钮"), EnhanceButton->GetIsEnabled());
    EnhanceButton->OnClicked.Broadcast();
    TestTrue(TEXT("初始强化必成功"), Craft->GetLastResult().bSuccess);
    TestEqual(TEXT("扣除一个金币"), Craft->GetGoldCount(), 199);
    TestTrue(TEXT("强化真实修改装备"), Item->GetEnhancement() >= .01f && Item->GetEnhancement() <= .1f);
    Craft->RequestCraft(ELxEquipmentCraftOperation::AddEntry);
    TestEqual(TEXT("零词条必新增一条"), Item->GetNormalEntries().Num(), 1);
    TestEqual(TEXT("新增收费一个金币"), Craft->GetGoldCount(), 198);
    TArray<FLxEquipmentEntry> OverFull = Item->GetNormalEntries(); OverFull[0].Proportion = 1.5f;
    TestTrue(TEXT("准备150%生效比例词条"), Item->ApplyCraftingState(Item->GetEnhancement(), OverFull));
    auto* AdjustButton = Cast<UButton>(Page->WidgetTree->FindWidget(TEXT("AdjustButton")));
    if (!TestNotNull(TEXT("调整词条按钮已绑定"), AdjustButton)) return false;
    AdjustButton->OnClicked.Broadcast();
    TestTrue(TEXT("超过100%后仍可通过界面调整"), Craft->GetLastResult().bExecuted);
    TestEqual(TEXT("调整收费五金币"), Craft->GetGoldCount(), 193);
    const float AdjustedRatio = Item->GetNormalEntries()[0].Proportion;
    TestTrue(TEXT("调整结果不会被截回100%"), AdjustedRatio >= 1.3f && AdjustedRatio <= 1.7f);
    auto* EntriesText = Cast<UTextBlock>(Page->WidgetTree->FindWidget(TEXT("EntriesText")));
    TestTrue(TEXT("界面正确显示超过100%的比例"), EntriesText && EntriesText->GetText().ToString().Contains(
        FString::Printf(TEXT("生效比例 %.0f%%"), AdjustedRatio * 100.f)));
    TArray<FLxEquipmentEntry> Full = Item->GetNormalEntries(); Full.SetNum(20);
    for (auto& Entry : Full) Entry = Item->GetNormalEntries()[0];
    TestTrue(TEXT("准备上限装备"), Item->ApplyCraftingState(Item->GetEnhancement(), Full));
    TestFalse(TEXT("20条禁止新增"), Craft->GetQuote(ELxEquipmentCraftOperation::AddEntry).bAvailable);
    Craft->RequestCraft(ELxEquipmentCraftOperation::AddEntry); TestEqual(TEXT("拒绝操作不扣金币"), Craft->GetGoldCount(), 193);
    Player->GetPlayerInteractionComponent()->RemoveInteractableComponent(Provider);
    Craft->RequestCraft(ELxEquipmentCraftOperation::Enhance); TestEqual(TEXT("离开NPC不能继续扣费"), Craft->GetGoldCount(), 193);
    Craft->EndEquipmentService(); TestNull(TEXT("关闭时清空打造引用"), Craft->GetSelectedEquipment());
    TestTrue(TEXT("关闭后装备仍在背包"), Slot->GetItem() == Item);
    auto* QuoteText = Cast<UTextBlock>(Page->WidgetTree->FindWidget(TEXT("EnhanceQuote")));
    TestTrue(TEXT("关闭服务后按钮不可继续打造"), !EnhanceButton->GetIsEnabled());
    TestTrue(TEXT("关闭服务后显示重新交互提示"), QuoteText && QuoteText->GetText().ToString().Contains(TEXT("交互")));
    Page->CloseUIDisplay();
    TestFalse(TEXT("通用关闭入口释放制造层"), UIManager->GetManufacturingUIManager()->ContainsWidget(Page));
    TestNull(TEXT("通用关闭清除制造组件引用"), Page->GetManufacturingComponent());
    UClass* TooltipClass = LoadClass<ULxItemTooltipWidget>(nullptr,
        TEXT("/Game/项目内容/UI/角色UI界面/弹窗界面/物品信息显示界面.物品信息显示界面_C"));
    if (!TestNotNull(TEXT("真实装备悬浮界面"), TooltipClass)) return false;
    auto* Tooltip = CreateWidget<ULxItemTooltipWidget>(World, TooltipClass);
    Tooltip->SetDisplayItemLogic(Item);
    auto* StrengthText = Cast<UTextBlock>(Tooltip->WidgetTree->FindWidget(TEXT("强化强度")));
    TestTrue(TEXT("悬浮提示显示新的强化度"), StrengthText && StrengthText->GetText().ToString().Contains(TEXT("强化度")));
    if (UWidget* Potential = Tooltip->WidgetTree->FindWidget(TEXT("锻造潜能"))) TestEqual(TEXT("隐藏旧锻造潜能"), Potential->GetVisibility(), ESlateVisibility::Collapsed);
    return true;
}
#endif
