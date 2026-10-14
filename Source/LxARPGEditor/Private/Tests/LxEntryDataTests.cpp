#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/Font.h"
#include "Engine/DataTable.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontCache.h"
#include "Fonts/CompositeFont.h"
#include "WidgetBlueprint.h"
#include "Components/RichTextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ListView.h"
#include "Components/SizeBox.h"
#include "../LxUIThemePreview.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Model/Entry/DataType/LxEntry.h"
#include "LxARPG/LxSource/Model/Style/RichText/LxRichTextStyleTypes.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxCharacterBaseAttributeConfig.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Buff/Logic/LxCharacterBuffComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/UI/ItemInfo/LxItemTooltipWidget.h"

/** 词条数据验证使用的实际项目配置与独立运行环境。 */
namespace LxEntryDataTest
{
    /** 本次新增 Buff 的中文行名，同时构成稳定标签的末段。 */
    const TCHAR* BuffNames[] = {TEXT("强健耐力"), TEXT("精准祝福"), TEXT("致命战意"), TEXT("盾卫誓约"),
        TEXT("剑锋祝福"), TEXT("迅捷攻势"), TEXT("疾风祝福"), TEXT("轻装行军"), TEXT("轻盈步伐"),
        TEXT("幸运祈愿"), TEXT("清泉共鸣"), TEXT("雷霆共鸣"), TEXT("晨光祝福"), TEXT("森林祝福")};

    /** 加载游戏设置中实际使用的管理器，并初始化运行时词条与物品缓存。 */
    ULxGameDataTablesManager* LoadManager()
    {
        UClass* Class = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
        auto* Manager = Class ? Class->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
        if (Manager) Manager->LoadDataTables();
        return Manager;
    }

    /** 使用标签创建真实词条，并保留引用倍率供显示和效果共同使用。 */
    ULxEntryObjectBase* MakeEntry(const FString& ID, const float Proportion = 1.f)
    {
        FLxEntryQuote Quote;
        Quote.EntryID = FGameplayTag::RequestGameplayTag(FName(*ID));
        Quote.EntryProportion = Proportion;
        return ULxEntryObjectBase::CreateEnterObject(GetTransientPackage(), Quote);
    }

    /** 构建自动清理的世界，允许 Buff 的真实计时器和撤回流程执行。 */
    struct FTestWorld
    {
        /** 本次验证唯一的临时世界。 */
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        /** 初始化角色模块所需的正常生命周期。 */
        FTestWorld()
        {
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            World->InitializeActorsForPlay(FURL());
            World->BeginPlay();
            World->GetWorldSettings()->NotifyBeginPlay();
        }
        /** 无论断言结果如何都结束计时器并释放世界。 */
        ~FTestWorld()
        {
            World->EndPlay(EEndPlayReason::Quit);
            GEngine->DestroyWorldContext(World);
            World->DestroyWorld(false);
        }
    };

    /** 在创建前设置实际角色属性表中的标签，避免原生测试角色使用空属性。 */
    ALxBaseCharacter* SpawnConfiguredCharacter(UWorld* World)
    {
        const UDataTable* Table = GetDefault<ULxGameSettings>()->CharacterBaseAttributeTable.LoadSynchronous();
        TArray<FLxCharacterBaseAttributeConfig*> Rows;
        if (Table) Table->GetAllRows(TEXT("词条数据验证"), Rows);
        if (Rows.IsEmpty()) return nullptr;
        ALxBaseCharacter* Character = World->SpawnActorDeferred<ALxBaseCharacter>(ALxBaseCharacter::StaticClass(),
            FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Character) return nullptr;
        FStructProperty* Property = FindFProperty<FStructProperty>(Character->GetClass(), TEXT("CharacterIDTag"));
        if (!Property) { Character->Destroy(); return nullptr; }
        *Property->ContainerPtrToValuePtr<FGameplayTag>(Character) = Rows[0]->CharacterTag;
        Character->FinishSpawning(FTransform::Identity);
        return Character;
    }

    /** 按数值结构读取词条实际作用的字段，资源增益读取上限而非当前资源。 */
    float AttributeValue(const ULxCharacterBaseAttributeSet* Set, const FGameplayTag Attribute)
    {
        FLxScalarAttributeData Scalar;
        if (Set->GetScalarAttribute(Attribute, Scalar)) return Scalar.Value;
        FLxResourceAttributeData Resource;
        if (Set->GetResourceAttribute(Attribute, Resource)) return Resource.ValueLimit;
        FLxRangeAttributeData Range;
        return Set->GetRangeAttribute(Attribute, Range) ? Range.Value : -1.f;
    }
}

/** 验证真实词条的样式行、百分号、百分点、小数与 Buff 时长文本。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEntryDataTextTest, "LxARPG.EntryData.DisplayText",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxEntryDataTextTest::RunTest(const FString& Parameters)
{
    using namespace LxEntryDataTest;
    auto* Manager = LoadManager();
    if (!TestNotNull(TEXT("实际数据管理器存在"), Manager)) return false;
    FLxRichStyledText Legacy;
    Legacy.TextStyleIDTag = FGameplayTag::RequestGameplayTag(TEXT("文本样式.词条样式.普通词条"));
    Legacy.Text = FText::FromString(TEXT("生命值回复 100"));
    TestEqual(TEXT("业务样式标签解析为真实普通样式行"), Legacy.GetStyledText().ToString(),
        FString(TEXT("<普通>生命值回复 100</>")));

    TStrongObjectPtr<ULxEntryObjectBase> Chance(MakeEntry(TEXT("词条.属性修改.标量.暴击率增加1"), 5.25f));
    if (TestNotNull(TEXT("比例属性的增加词条可创建"), Chance.Get()))
        TestTrue(TEXT("0.01比例按照百分点显示并保留小数"), Chance->GetDisplayName().ToString().Contains(TEXT("5.25个百分点")));
    TStrongObjectPtr<ULxEntryObjectBase> Speed(MakeEntry(TEXT("词条.属性修改.标量.基础移动速度增加1"), .5f));
    if (TestNotNull(TEXT("移动速度词条可创建"), Speed.Get()))
        TestTrue(TEXT("半米每秒增益不会显示成零"), Speed->GetDisplayName().ToString().Contains(TEXT("0.5米/秒")));
    TStrongObjectPtr<ULxEntryObjectBase> Recovery(MakeEntry(TEXT("词条.资源修改.恢复.生命百分比回复1")));
    if (TestNotNull(TEXT("已有比例回复词条仍可创建"), Recovery.Get()))
    {
        const FString Text = Recovery->GetDisplayName().ToString();
        TestTrue(TEXT("已有回复正确显示百分比"), Text.Contains(TEXT("2%")));
        TestFalse(TEXT("回复模板没有重复百分号"), Text.Contains(TEXT("%%")));
        TestTrue(TEXT("回复词条生成原生样式行名"), Text.StartsWith(TEXT("<稀有>")));
    }

    UDataTable* Tables[] = {Manager->m_pAttributeGainEntryTable, Manager->m_pAttributeInfluenceEntryTable,
        Manager->m_pAttributeRecoveryEntryTable, Manager->m_pCreateBuffEntryTable, Manager->m_pChangeStateEntryTable,
        Manager->m_pDisplayTextEntryTable, Manager->m_pGrantSkillEntryTable, Manager->m_pDamageEntryTable};
    FString Report;
    for (UDataTable* Table : Tables)
    {
        if (!TestNotNull(TEXT("实际词条表存在"), Table)) continue;
        for (const auto& Pair : Table->GetRowMap())
        {
            const auto* Data = reinterpret_cast<const FLxEntryBase*>(Pair.Value);
            // 无标签的空模板行不会加载到运行时词条缓存，也不参与可视化验证。
            if (!Data->EntryID.IsValid()) continue;
            TStrongObjectPtr<ULxEntryObjectBase> Entry(MakeEntry(Data->EntryID.ToString()));
            if (!TestNotNull(Pair.Key.ToString(), Entry.Get())) continue;
            const FString Text = Entry->GetDisplayName().ToString();
            TestFalse(Pair.Key.ToString() + TEXT("没有未替换占位符"), Text.Contains(TEXT("{0}")));
            TestFalse(Pair.Key.ToString() + TEXT("没有无效词条样式末段"), Text.Contains(TEXT("词条>")));
            TestFalse(Pair.Key.ToString() + TEXT("没有Unicode替换字符"), Text.Contains(TEXT("\ufffd")));
            FName Style;
            if (Data->EntryText.EntryDisplayName.TextStyleIDTag.IsValid())
            {
                TestTrue(Pair.Key.ToString() + TEXT("样式标签能解析"), Data->EntryText.EntryDisplayName.ResolveTextStyleTag(Style));
                TestTrue(Pair.Key.ToString() + TEXT("显示使用实际样式行"), Text.StartsWith(TEXT("<") + Style.ToString() + TEXT(">")));
            }
            Report += Pair.Key.ToString() + TEXT("：") + Text + TEXT("\n");
        }
    }
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("词条数据维护");
    TestTrue(TEXT("保存真实运行时显示文本"), FFileHelper::SaveStringToFile(Report,
        *(Directory / TEXT("运行时文本.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM));
    return true;
}

/** 使用创建 Buff 词条的完整链路验证14个增益实际作用于角色，移除和到期均能撤回。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEntryDataBuffTest, "LxARPG.EntryData.BuffLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxEntryDataBuffTest::RunTest(const FString& Parameters)
{
    using namespace LxEntryDataTest;
    auto* Manager = LoadManager();
    if (!TestNotNull(TEXT("实际数据管理器存在"), Manager)) return false;
    FTestWorld Context;
    auto* Character = SpawnConfiguredCharacter(Context.World);
    if (!TestNotNull(TEXT("使用真实角色属性配置"), Character)) return false;
    auto* BuffModule = Character->GetCharacterBuffComponent();
    auto* AttributeComponent = Character->GetCharacterAttributeComponent();
    auto* Transfer = Character->GetCharacterDataTransferComponent();
    if (!TestNotNull(TEXT("Buff模块已初始化"), BuffModule)
        || !TestNotNull(TEXT("属性组件已初始化"), AttributeComponent)
        || !TestNotNull(TEXT("数据传递组件已初始化"), Transfer)) return false;

    for (const TCHAR* Name : BuffNames)
    {
        const FLxBuffInformation* Definition = Manager->m_pBuffItemTable->FindRow<FLxBuffInformation>(Name, TEXT("词条Buff验证"));
        if (!TestNotNull(FString(Name) + TEXT("数据存在"), Definition)) continue;
        TestEqual(FString(Name) + TEXT("包含三种属性作用方式"), Definition->ItemEntryConfigs.Num(), 3);
        if (Definition->ItemEntryConfigs.Num() != 3) continue;
        TStrongObjectPtr<ULxEntryObjectBase> Gain(MakeEntry(Definition->ItemEntryConfigs[0].EntryQuote.EntryID.ToString()));
        const auto* GainObject = Cast<ULxEntryObjectAttributeGain>(Gain.Get());
        if (!TestNotNull(FString(Name) + TEXT("属性词条可解析"), GainObject)) continue;
        const FGameplayTag Attribute = GainObject->GetAttributeGainData().AttributeIDTag;
        const float Before = AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), Attribute);
        TStrongObjectPtr<ULxEntryObjectBase> Create(MakeEntry(TEXT("词条.创建Buff.获得") + FString(Name)));
        if (!TestNotNull(FString(Name) + TEXT("创建词条存在"), Create.Get())) continue;
        TestTrue(FString(Name) + TEXT("创建文本包含真实持续时间"), Create->GetDisplayName().ToString().Contains(TEXT("持续30秒")));
        FLxEffectPackage Package;
        Package.ApplyPolicy = ELxEffectPackageApplyPolicy::Instant;
        Package.SourceContext.SourceType = ELxEffectPackageSource::Skill;
        Package.SourceContext.SourceName = TEXT("词条数据验证");
        Create->AppendEffectsToPackage(Package);
        Transfer->ApplyEffectPackage(Package);
        TArray<ULxBuff*> Active;
        BuffModule->GetActiveBuffs(Active);
        if (!TestEqual(FString(Name) + TEXT("创建真实Buff"), Active.Num(), 1)) { BuffModule->ClearBuffs(); continue; }
        TestEqual(FString(Name) + TEXT("完整词条引用可创建"), Active[0]->GetItemEntryList().Num(), 3);
        TestTrue(FString(Name) + TEXT("持续时间为30秒"), FMath::IsNearlyEqual(Active[0]->GetRemainingDuration(), 30.f));
        const float After = AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), Attribute);
        TestTrue(FString(Name) + TEXT("属性确实增加"), After > Before);
        TestTrue(FString(Name) + TEXT("可正常撤回"), BuffModule->RemoveBuff(Active[0]));
        TestTrue(FString(Name) + TEXT("撤回后恢复原属性"), FMath::IsNearlyEqual(
            AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), Attribute), Before, .0001f));
    }

    const auto* SpeedDefinition = Manager->m_pBuffItemTable->FindRow<FLxBuffInformation>(TEXT("轻盈步伐"), TEXT("时效Buff验证"));
    if (!TestNotNull(TEXT("速度增益配置存在"), SpeedDefinition)) return false;
    const FGameplayTag SpeedTag = FGameplayTag::RequestGameplayTag(TEXT("属性.行动.基础移动速度"));
    const float BeforeSpeed = AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), SpeedTag);
    BuffModule->AddBuff(SpeedDefinition->ItemIDTag, 1.f, .5f);
    TestTrue(TEXT("小数速度增益不会在属性计算中取整"), FMath::IsNearlyEqual(
        AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), SpeedTag), (BeforeSpeed + .5f) * 1.1f * 1.05f, .0001f));
    for (int32 Index = 0; Index < 8; ++Index) { ++GFrameCounter; Context.World->Tick(LEVELTICK_All, .1f); }
    TArray<ULxBuff*> Remaining;
    BuffModule->GetActiveBuffs(Remaining);
    TestEqual(TEXT("时间到期自动移除Buff"), Remaining.Num(), 0);
    TestTrue(TEXT("到期后自动撤回属性"), FMath::IsNearlyEqual(
        AttributeValue(AttributeComponent->GetRuntimeAttributeSet(), SpeedTag), BeforeSpeed, .0001f));
    return true;
}

/** 通过实际物品悬浮界面离屏渲染三种 Buff，同时核对词条字体中的中文和数值字形。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxEntryDataPreviewTest, "LxARPG.EntryData.VisualPreview",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxEntryDataPreviewTest::RunTest(const FString& Parameters)
{
    using namespace LxEntryDataTest;
    if (!TestNotNull(TEXT("实际数据管理器存在"), LoadManager())) return false;
    auto* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
        TEXT("/Game/项目内容/UI/角色UI界面/弹窗界面/物品信息显示界面"));
    if (!TestNotNull(TEXT("实际物品悬浮界面存在"), Blueprint)) return false;
    FTestWorld Context;
    for (const TCHAR* Name : {TEXT("精准祝福"), TEXT("轻盈步伐"), TEXT("强健耐力")})
    {
        FLxItemQuote Quote(FGameplayTag::RequestGameplayTag(FName(*(TEXT("物品.buff.属性提升效果.") + FString(Name)))), 1);
        TStrongObjectPtr<ULxItemBase> Item(ULxItemBase::CreateItemObject(GetTransientPackage(), Quote));
        if (!TestNotNull(FString(Name) + TEXT("真实物品存在"), Item.Get())) continue;
        Item->ItemIcon().LoadSynchronous();
        TestTrue(FString(Name) + TEXT("实际悬浮信息可绘制"), LxUITheme::RenderPreview(Blueprint,
            FPaths::ProjectSavedDir() / TEXT("词条数据维护/Buff预览") / Name, false,
            [this, &Item](UUserWidget* Widget)
            {
                auto* Tooltip = Cast<ULxItemTooltipWidget>(Widget);
                TestTrue(TEXT("实际蓝图接收完整Buff显示数据"), Tooltip && Tooltip->SetDisplayItemLogic(Item.Get()));
                if (auto* Entries = Cast<UListView>(Widget->GetWidgetFromName(TEXT("基础词条显示"))))
                    TestEqual(TEXT("悬浮界面收到三条实际属性词条"), Entries->GetNumItems(), 3);
            }, Context.World));
    }

    // 使用项目实际富文本控件展示已格式化数据，避免虚拟列表首帧尚未生成行时遗漏视觉核对。
    auto* TextBlueprint = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/项目内容/UI/UI组件/富文本显示框"));
    if (!TestNotNull(TEXT("实际富文本控件存在"), TextBlueprint)) return false;
    FString Displayed;
    const TPair<const TCHAR*, float> Samples[] = {
        {TEXT("词条.属性修改.标量.暴击率增加1"), 5.f},
        {TEXT("词条.属性修改.标量.暴击率提高1"), 10.f},
        {TEXT("词条.属性修改.标量.暴击率总增1"), 5.f},
        {TEXT("词条.属性修改.标量.基础移动速度增加1"), .5f},
        {TEXT("词条.资源修改.上限.体力值上限增加1"), 20.f},
        {TEXT("词条.资源修改.恢复.生命百分比回复1"), 1.f},
        {TEXT("词条.创建Buff.获得精准祝福"), 1.f},
        {TEXT("词条.创建Buff.创建生命回复Buff1"), 1.f}};
    for (const auto& Sample : Samples)
    {
        TStrongObjectPtr<ULxEntryObjectBase> Entry(MakeEntry(Sample.Key, Sample.Value));
        if (Entry) Displayed += Entry->GetDisplayName().ToString() + TEXT("\n");
    }
    TestTrue(TEXT("实际富文本组件直接绘制修复后的词条"), LxUITheme::RenderPreview(TextBlueprint,
        FPaths::ProjectSavedDir() / TEXT("词条数据维护/文本预览"), false,
        [this, &Displayed](UUserWidget* Widget)
        {
            TArray<UWidget*> Widgets;
            Widget->WidgetTree->GetAllWidgets(Widgets);
            bool bFoundRichText = false;
            for (auto* Child : Widgets)
            {
                if (auto* Rich = Cast<URichTextBlock>(Child))
                {
                    Rich->SetText(FText::FromString(Displayed));
                    Rich->SetAutoWrapText(false);
                    Rich->SetMinDesiredWidth(640.f);
                    Rich->RefreshTextLayout();
                    bFoundRichText = true;
                }
                if (auto* Border = Cast<UBorder>(Child)) Border->SetBrushColor(FLinearColor(.94f, .93f, .87f, 1.f));
            }
            if (auto* Size = Cast<USizeBox>(Widget->WidgetTree->RootWidget))
            {
                Size->SetWidthOverride(680.f);
                Size->SetHeightOverride(260.f);
            }
            TestTrue(TEXT("富文本控件包含实际显示块"), bFoundRichText);
        }, Context.World));
    auto* StyleTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/项目内容/美术资产/界面主题/明亮文本样式集表"));
    if (!TestNotNull(TEXT("实际词条样式表存在"), StyleTable)) return false;
    const auto* Row = StyleTable->FindRow<FRichTextStyleRow>(TEXT("普通"), TEXT("字体字形验证"));
    if (!TestNotNull(TEXT("普通样式存在"), Row)) return false;
    auto* Font = Cast<UFont>(Row->TextStyle.Font.FontObject);
    if (!TestNotNull(TEXT("词条字体资产有效"), Font)) return false;
    const FCompositeFont* Composite = Font->GetCompositeFont();
    if (!TestTrue(TEXT("字体具有可加载的字形数据"), Composite && !Composite->DefaultTypeface.Fonts.IsEmpty())) return false;
    const FFontData& FontData = Composite->DefaultTypeface.Fonts[0].Font;
    const FString Sample = TEXT("0123456789.+-%{}[]力量智慧敏捷体质生命魔力体力护盾暴击率伤害格挡减免护甲攻击速度移动加成负重基础米秒幸运火水电元素亲和光明之神自然信仰增加提高总增恢复持续，");
    auto Cache = FSlateApplication::Get().GetRenderer()->GetFontCache();
    TSet<TCHAR> Checked;
    for (const TCHAR Character : Sample)
    {
        if (Checked.Contains(Character)) continue;
        Checked.Add(Character);
        TestTrue(FString::Printf(TEXT("字体包含字形：%c"), Character), Cache->CanLoadCodepoint(FontData, Character));
    }
    return true;
}
#endif
