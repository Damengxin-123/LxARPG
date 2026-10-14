#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ListView.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntryTableConfig.h"
#include "LxARPG/LxSource/Model/Item/DataType/ConstData/LxItemConstData.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxItemSaveData.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"
#include "LxARPG/LxSource/UI/UICore/LxUITextData.h"

namespace LxEquipmentTooltipTest
{
    /** 读取真实蓝图词条列表的数据文本，无需启动视口或构造Slate界面。 */
    TArray<FString> ReadEntryTexts(ULxItemTooltipWidget* Tooltip, const TCHAR* ListName)
    {
        TArray<FString> Result;
        if (const auto* List = Cast<UListView>(Tooltip->WidgetTree->FindWidget(FName(ListName))))
            for (UObject* Object : List->GetListItems())
                if (const auto* Data = Cast<ULxUITextData>(Object)) Result.Add(Data->DisplayText.ToString());
        return Result;
    }
}

/** 真实物品信息蓝图显示强化后的数值，同时保证原始词条缓存和存档不会被显示操作污染。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEquipmentTooltipEnhancementTest, "LxARPG.UI.ItemTooltip.EnhancementText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxEquipmentTooltipEnhancementTest::RunTest(const FString& Parameters)
{
    using namespace LxEquipmentTooltipTest;
    UGameInstance* Instance = NewObject<UGameInstance>(); Instance->Init();
    if (auto* Subsystem = Instance->GetSubsystem<ULxGameInstanceSubsystem>()) if (Subsystem->GetSaveManager()) Subsystem->GetSaveManager()->SetReadOnly(true);
    ON_SCOPE_EXIT { Instance->Shutdown(); };
    if (!TestTrue(TEXT("存在真实装备配置"), !LxItemConfig::GetEquipmentItemMap().IsEmpty())) return false;
    if (!TestTrue(TEXT("存在真实材料配置"), !LxItemConfig::GetMaterialItemMap().IsEmpty())) return false;
    const FLxEquipmentInformation OriginalEquipment = LxItemConfig::GetEquipmentItemMap().CreateConstIterator()->Value;
    const FLxMaterialInformation OriginalMaterial = LxItemConfig::GetMaterialItemMap().CreateConstIterator()->Value;
    ON_SCOPE_EXIT { LxItemConfig::SetEquipmentItemData(OriginalEquipment); LxItemConfig::SetMaterialItemData(OriginalMaterial); };

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
    if (!TestEqual(TEXT("存在两个真实增益词条用于基础和普通列表"), OriginalEntries.Num(), 2)) return false;
    ON_SCOPE_EXIT { for (const auto& Entry : OriginalEntries) LxEntryConfig::SetAttributeGainEntryData(Entry); };
    for (int32 Index = 0; Index < 2; ++Index)
    {
        FLxEntryAttributeGain Entry = OriginalEntries[Index];
        Entry.AttributeIDTag = LxTag_Attribute_Ability_Strength;
        Entry.EntryTarget = ELxEntryTarget::ToValue;
        Entry.EffectiveType = ELxEntryEffectiveType::BasicValue;
        Entry.EntryValue = Index == 0 ? 40.f : 20.f;
        Entry.EntryText.EntryDisplayName.TextStyleIDTag = FGameplayTag();
        Entry.EntryText.EntryDisplayName.Text = FText::FromString(Index == 0 ? TEXT("基础增加 {0}") : TEXT("普通增加 {0}"));
        LxEntryConfig::SetAttributeGainEntryData(Entry);
    }
    FLxEntryQuote Base; Base.EntryID = OriginalEntries[0].EntryID;
    FLxEntryQuote NormalQuote; NormalQuote.EntryID = OriginalEntries[1].EntryID; NormalQuote.EntryProportion = 1.42f;
    FLxEquipmentInformation EquipmentConfig = OriginalEquipment;
    EquipmentConfig.bFixedNormalEntries = false;
    EquipmentConfig.ItemEntryConfigs = {FLxItemEntryConfig(Base, ELxEntryLogicType::Base)};
    LxItemConfig::SetEquipmentItemData(EquipmentConfig);
    auto* Equipment = Cast<ULxEquipment>(ULxItemBase::CreateItemObject(Instance, FLxItemQuote(EquipmentConfig.ItemIDTag, 1)));
    if (!TestNotNull(TEXT("创建真实装备实例"), Equipment)) return false;
    FLxEquipmentEntry Normal; Normal.EntryID = NormalQuote.EntryID; Normal.Proportion = NormalQuote.EntryProportion;
    if (!TestTrue(TEXT("设置125%强化和142%普通比例"), Equipment->ApplyCraftingState(1.25f, {Normal}))) return false;
    if (!TestEqual(TEXT("基础及普通词条均已建立"), Equipment->GetItemEntryList().Num(), 2)) return false;
    FLxItemSaveRecord OriginalSave;
    if (!TestTrue(TEXT("显示前采集原始装备存档"), LxItemSaveData::CaptureItem(Equipment, OriginalSave))) return false;

    FLxMaterialInformation MaterialConfig = OriginalMaterial;
    MaterialConfig.ItemCountMax = 99;
    MaterialConfig.ItemEntryConfigs = {FLxItemEntryConfig(Base, ELxEntryLogicType::Base), FLxItemEntryConfig(NormalQuote, ELxEntryLogicType::Normal)};
    LxItemConfig::SetMaterialItemData(MaterialConfig);
    ULxItemBase* Material = ULxItemBase::CreateItemObject(Instance, FLxItemQuote(MaterialConfig.ItemIDTag, 2));
    if (!TestNotNull(TEXT("创建使用相同原始词条的非装备实例"), Material)) return false;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    World->SetGameInstance(Instance);
    ON_SCOPE_EXIT { World->SetGameInstance(nullptr); World->DestroyWorld(false); };
    UClass* TooltipClass = LoadClass<ULxItemTooltipWidget>(nullptr,
        TEXT("/Game/项目内容/UI/角色UI界面/弹窗界面/物品信息显示界面.物品信息显示界面_C"));
    if (!TestNotNull(TEXT("加载真实物品信息显示界面蓝图"), TooltipClass)) return false;
    auto* Tooltip = CreateWidget<ULxItemTooltipWidget>(World, TooltipClass);
    if (!TestNotNull(TEXT("创建真实蓝图及控件树"), Tooltip)) return false;
    if (!TestNotNull(TEXT("真实基础词条列表存在"), Cast<UListView>(Tooltip->WidgetTree->FindWidget(TEXT("基础词条显示"))))) return false;
    if (!TestNotNull(TEXT("真实普通词条列表存在"), Cast<UListView>(Tooltip->WidgetTree->FindWidget(TEXT("普通扩展词条显示"))))) return false;

    /** 核对实际蓝图分类列表的最终文本，同时发现旧兼容事件和分类事件重复添加。 */
    const auto CheckLists = [this, Tooltip](const FString& Context, const FString& ExpectedBase, const FString& ExpectedNormal)
    {
        const auto BaseTexts = ReadEntryTexts(Tooltip, TEXT("基础词条显示"));
        const auto NormalTexts = ReadEntryTexts(Tooltip, TEXT("普通扩展词条显示"));
        if (TestEqual(Context + TEXT("基础词条恰好一条"), BaseTexts.Num(), 1)) TestEqual(Context + TEXT("基础数值文本"), BaseTexts[0], ExpectedBase);
        if (TestEqual(Context + TEXT("普通词条恰好一条"), NormalTexts.Num(), 1)) TestEqual(Context + TEXT("普通数值文本"), NormalTexts[0], ExpectedNormal);
    };
    for (int32 Index = 0; Index < 3; ++Index)
    {
        TestTrue(TEXT("强化装备重复显示成功"), Tooltip->SetDisplayItemLogic(Equipment));
        CheckLists(TEXT("125%强化重复显示 / "), TEXT("基础增加 90"), TEXT("普通增加 63.9"));
    }
    TestEqual(TEXT("无倍率接口始终展示基础原值"), Equipment->GetItemEntryList()[0]->GetDisplayName().ToString(), FString(TEXT("基础增加 40")));
    TestEqual(TEXT("无倍率接口始终展示普通原值及自身比例"), Equipment->GetItemEntryList()[1]->GetDisplayName().ToString(), FString(TEXT("普通增加 28.4")));
    TestEqual(TEXT("基础词条原始缓存未被显示倍率修改"), Equipment->GetItemEntryList()[0]->GetEntryQuote().EntryProportion, 1.f);
    TestEqual(TEXT("普通词条原始缓存未被显示倍率修改"), Equipment->GetItemEntryList()[1]->GetEntryQuote().EntryProportion, 1.42f);
    FLxItemSaveRecord AfterDisplay;
    TestTrue(TEXT("重复显示后可采集装备存档"), LxItemSaveData::CaptureItem(Equipment, AfterDisplay));
    TestEqual(TEXT("显示不改变装备身份"), AfterDisplay.EquipmentInstanceID, OriginalSave.EquipmentInstanceID);
    TestEqual(TEXT("显示不改变存档强化度"), AfterDisplay.Equipment.Enhancement, 1.25f);
    if (TestEqual(TEXT("显示后存档仍只有一条动态普通词条"), AfterDisplay.Equipment.NormalEntries.Num(), 1))
        TestEqual(TEXT("存档比例保留1.42而不是强化后比例"), AfterDisplay.Equipment.NormalEntries[0].Proportion, 1.42f);
    TestTrue(TEXT("显示不把基础词条写入通用存档"), AfterDisplay.Entries.IsEmpty());

    TestTrue(TEXT("同一蓝图切换到非装备"), Tooltip->SetDisplayItemLogic(Material));
    CheckLists(TEXT("切换非装备 / "), TEXT("基础增加 40"), TEXT("普通增加 28.4"));
    TestEqual(TEXT("非装备倍率始终为一"), Material->GetItemEntryEffectMultiplier(), 1.f);
    TestTrue(TEXT("同一蓝图切回装备"), Tooltip->SetDisplayItemLogic(Equipment));
    CheckLists(TEXT("切回装备 / "), TEXT("基础增加 90"), TEXT("普通增加 63.9"));
    TestTrue(TEXT("更新强化为50%"), Equipment->ApplyCraftingState(.5f, {Normal}));
    TestTrue(TEXT("刷新更新后的装备"), Tooltip->SetDisplayItemLogic(Equipment));
    CheckLists(TEXT("50%强化 / "), TEXT("基础增加 60"), TEXT("普通增加 42.6"));
    auto* Restored = Cast<ULxEquipment>(LxItemSaveData::CreateItem(Instance, OriginalSave));
    if (TestNotNull(TEXT("读档恢复显示前的装备"), Restored))
    {
        if (!TestEqual(TEXT("读档基础及普通词条均存在"), Restored->GetItemEntryList().Num(), 2)) return false;
        TestEqual(TEXT("读档基础缓存保持原始比例"), Restored->GetItemEntryList()[0]->GetEntryQuote().EntryProportion, 1.f);
        TestEqual(TEXT("读档普通缓存保持原始比例"), Restored->GetItemEntryList()[1]->GetEntryQuote().EntryProportion, 1.42f);
        TestTrue(TEXT("读档装备显示成功"), Tooltip->SetDisplayItemLogic(Restored));
        CheckLists(TEXT("读档125%强化 / "), TEXT("基础增加 90"), TEXT("普通增加 63.9"));
    }
    return true;
}

#endif
