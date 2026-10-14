#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/StrongObjectPtr.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "WidgetBlueprint.h"
#include "../LxUIThemePreview.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTreeAsset.h"
#include "LxARPG/LxSource/Model/Profession/DataType/LxProfessionTableConfig.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxCharacterProfessionComponent.h"
#include "LxARPG/LxSource/Model/Profession/Logic/LxProfessionDefinition.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/UI/Profession/LxProfessionWidget.h"
#include "LxARPG/LxSource/UI/Profession/LxProfessionUIData.h"

namespace LxProfessionRuntimeTest
{
	/** 读取项目实际使用的数据管理配置。 */
	ULxGameDataTablesManager* LoadManager()
	{
		UClass* Class = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
		auto* Manager = Class ? Class->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
		if (Manager) Manager->LoadDataTables(); return Manager;
	}
	/** 读取特定职业效果包中的指定属性加成。 */
	float EffectValue(ULxCharacterProfessionModule* Module, FGameplayTag Profession, const TCHAR* Attribute)
	{
		TArray<FLxEffectPackage> Packages; Module->BuildAllProfessionEffectPackages(Packages);
		float Value = 0.f;
		for (const auto& Package : Packages)
			if (Package.SourceContext.SourceIDTag == Profession)
				for (const auto& Modifier : Package.AttributeModifierEffects)
					if (Modifier.AttributeIDTag == FGameplayTag::RequestGameplayTag(Attribute)) Value += Modifier.ModifierValue;
		return Value;
	}
	/** 测试真实角色与UI事件使用的独立世界。 */
	struct FWorld
	{
		/** 随测试销毁的世界。 */
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		/** 初始化世界及角色生命周期。 */
		FWorld() { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); World->InitializeActorsForPlay(FURL()); World->BeginPlay(); World->GetWorldSettings()->NotifyBeginPlay(); }
		/** 清理计时器和世界引用。 */
		~FWorld() { World->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(World); World->DestroyWorld(false); }
	};
}

/** 使用项目实际职业资产验证学习、升级、入向倍率与存档数据兼容。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionRuntimeTest, "LxARPG.ProfessionTree.Runtime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxProfessionRuntimeTest::RunTest(const FString& Parameters)
{
	using namespace LxProfessionRuntimeTest;
	auto* Manager = LoadManager(); if (!TestNotNull(TEXT("实际数据管理配置存在"), Manager)) return false;
	auto* Tree = Manager->ProfessionTreeAsset.Get(); if (!TestNotNull(TEXT("数据管理配置已切换到职业体系"), Tree)) return false;
	TestTrue(TEXT("当前真实职业配置合法且ID唯一"), Tree->ValidateConfiguration().IsEmpty());
	TStrongObjectPtr<ULxProfessionTreeAsset> Copy(DuplicateObject<ULxProfessionTreeAsset>(Tree, GetTransientPackage()));
	LxProfessionConfig::SetProfessionTree(Copy.Get()); ON_SCOPE_EXIT { LxProfessionConfig::SetProfessionTree(Tree); };
	const FGameplayTag Novice = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.见习魔法师"));
	const FGameplayTag Junior = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.初级魔法师"));
	const FGameplayTag Middle = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.中级魔法师"));
	const FGameplayTag Senior = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.高级魔法师"));
	const FGameplayTag Fire = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.元素使.火焰使者"));
	TStrongObjectPtr<ULxCharacterProfessionModule> Module(NewObject<ULxCharacterProfessionModule>()); Module->InitializeModule(nullptr);
	TArray<ULxProfessionDefinition*> Definitions; Module->GetAllProfessionDefinitions(Definitions);
	TestEqual(TEXT("运行时只加载体系内的全部职业"), Definitions.Num(), Tree->Professions.Num());
	FLxProfessionLearnCheckResult Check;
	TestFalse(TEXT("初级魔法师需要前置"), Module->CanLearnProfession(Junior, Check));
	TestTrue(TEXT("实习魔法师可以学习"), Module->LearnProfession(Novice));
	TestEqual(TEXT("实习第一级提供智慧1"), EffectValue(Module.Get(), Novice, TEXT("属性.能力值.智慧")), 1.f);
	ULxProfessionDefinition* View = Module->GetProfessionDefinition(Novice);
	if (!TestNotNull(TEXT("存在资产兼容视图"), View)) return false;
	Module->AddProfessionExperienceByType(ELxProfessionType::Combat, View->GetRequiredExperienceForNextLevel(1) + View->GetRequiredExperienceForNextLevel(2) + 17.f);
	FLxProfessionRuntimeData Runtime; Module->GetProfessionRuntimeData(Novice, Runtime);
	TestEqual(TEXT("按照共享公式连续升级"), Runtime.Level, 3); TestEqual(TEXT("保留等级内剩余经验"), Runtime.Experience, 17.f);
	TestEqual(TEXT("三级累积三个等级的词条"), EffectValue(Module.Get(), Novice, TEXT("属性.能力值.智慧")), 3.f);
	TestTrue(TEXT("赋予满级实习职业"), Module->GrantProfession(Novice, 5, true));
	TestTrue(TEXT("前置满足后可学习初级"), Module->LearnProfession(Junior));
	TestTrue(TEXT("可学习火焰使者"), Module->LearnProfession(Fire));
	TestTrue(TEXT("初级职业只对火焰使者产生一次增益"), FMath::IsNearlyEqual(EffectValue(Module.Get(), Fire, TEXT("属性.能力值.智慧")), 4.8f));
	TestEqual(TEXT("单向输出不会反向增强初级魔法师"), EffectValue(Module.Get(), Junior, TEXT("属性.能力值.智慧")), 3.f);
	Module->LearnProfession(Middle); Module->LearnProfession(Senior);
	TestTrue(TEXT("多个入向倍率按乘积应用一次"), FMath::IsNearlyEqual(EffectValue(Module.Get(), Fire, TEXT("属性.元素.火元素亲和")), 25.2f));
	TArray<FLxProfessionSaveRecord> Records; Module->CaptureProfessionSaveData(Records);
	auto* Restored = NewObject<ULxCharacterProfessionModule>(); Restored->InitializeModule(nullptr);
	TestTrue(TEXT("现有按职业ID存档记录可恢复"), Restored->RestoreProfessionSaveData(Records));
	TestTrue(TEXT("恢复后效果不叠加"), FMath::IsNearlyEqual(EffectValue(Restored, Fire, TEXT("属性.元素.火元素亲和")), 25.2f));
	FLxProfessionSaveRecord Retired; Retired.ProfessionIDTag = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.战士.见习战士")); Retired.Level = 2; Retired.Experience = 15;
	Records.Add(Retired); TestTrue(TEXT("已移除职业不会阻止旧存档加载"), Restored->RestoreProfessionSaveData(Records));
	TArray<FLxProfessionSaveRecord> Export; Restored->CaptureProfessionSaveData(Export); TestEqual(TEXT("已移除职业进度仍被保留"), Export.Num(), Records.Num());
	for (ULxProfessionTreeInfluence* Influence : Copy->Influences)
		if (Influence->Kind == ELxProfessionTreeRelation::Benefit && Influence->Source->ProfessionID == Junior) Influence->Multiplier = 0.f;
	auto* Suppressed = NewObject<ULxCharacterProfessionModule>(); Suppressed->InitializeModule(nullptr); Suppressed->RestoreProfessionSaveData(Records);
	TestEqual(TEXT("零倍率完全抑制职业加成"), EffectValue(Suppressed, Fire, TEXT("属性.元素.火元素亲和")), 0.f);
	return true;
}

/** 通过真实角色更新属性，并驱动已保存职业蓝图的筛选、学习及详情显示。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxProfessionCharacterUITest, "LxARPG.ProfessionTree.CharacterUI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxProfessionCharacterUITest::RunTest(const FString& Parameters)
{
	using namespace LxProfessionRuntimeTest;
	if (!TestNotNull(TEXT("实际配置存在"), LoadManager())) return false;
	FWorld Context; auto* Character = Context.World->SpawnActor<ALxBaseCharacter>();
	if (!TestNotNull(TEXT("创建实际角色"), Character)) return false;
	Character->InitialCharacterInformation();
	auto* Transfer = Character->GetCharacterDataTransferComponent();
	const FGameplayTag Novice = FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.魔法师.见习魔法师"));
	const FGameplayTag Wisdom = FGameplayTag::RequestGameplayTag(TEXT("属性.能力值.智慧"));
	float Before = 0.f, After = 0.f; Transfer->QueryCharacterAttributeValue(Wisdom, Before);
	TestTrue(TEXT("角色学习新资产中的职业"), Transfer->LearnProfession(Novice));
	Transfer->QueryCharacterAttributeValue(Wisdom, After); TestEqual(TEXT("学习后实际角色属性立即变化"), After - Before, 1.f);
	auto* BP = LoadObject<UWidgetBlueprint>(nullptr, TEXT("/Game/项目内容/UI/角色UI界面/角色面板/角色职业界面"));
	if (!TestNotNull(TEXT("职业面板蓝图存在"), BP)) return false;
	TestTrue(TEXT("面板默认显示全部可学习职业"), !BP->GeneratedClass->GetDefaultObject<ULxProfessionWidget>()->IsOnlyShowLearnedProfessions());
	TestTrue(TEXT("真实职业面板可渲染"), LxUITheme::RenderPreview(BP, FPaths::ProjectSavedDir() / TEXT("ProfessionMigration/运行时"), false,
		[this, Transfer, Novice](UUserWidget* Widget)
		{
			auto* UI = CastChecked<ULxProfessionWidget>(Widget); UI->UpdateUIComponents(Transfer);
			TestEqual(TEXT("战斗分类显示五个职业"), UI->GetProfessionListUIData().Num(), 5);
			UI->SelectProfession(Novice);
			TestEqual(TEXT("实习职业显示五级成长配置"), UI->GetProfessionLevelListUIData().Num(), 5);
			UI->SetProfessionTypeFilter(ELxProfessionType::Gathering); TestEqual(TEXT("采集筛选生效"), UI->GetProfessionListUIData().Num(), 2);
			const auto Miner = FGameplayTag::RequestGameplayTag(TEXT("职业.采集.矿工")); UI->SelectProfession(Miner);
			if (auto* Button = Cast<UButton>(UI->GetWidgetFromName(TEXT("学习职业按钮")))) Button->OnClicked.Broadcast();
			FLxProfessionRuntimeData MinerData; TestTrue(TEXT("蓝图布局中的学习按钮调用真实模块"), Transfer->GetProfessionRuntimeData(Miner, MinerData));
			UI->SetProfessionTypeFilter(ELxProfessionType::Crafting); TestEqual(TEXT("生产筛选生效"), UI->GetProfessionListUIData().Num(), 2);
			UI->SetProfessionTypeFilter(ELxProfessionType::Combat); UI->SelectProfession(Novice);
			auto* Name = Cast<UTextBlock>(UI->GetWidgetFromName(TEXT("详情职业名称")));
			TestTrue(TEXT("详情标题使用资产职业名称"), Name && Name->GetText().ToString() == TEXT("实习魔法师"));
			auto* Learn = Cast<UButton>(UI->GetWidgetFromName(TEXT("学习职业按钮")));
			TestTrue(TEXT("已学习职业不可重复点击学习"), Learn && !Learn->GetIsEnabled());
			Transfer->AddProfessionExperienceByType(ELxProfessionType::Combat, 242.f);
		}, Context.World));
	TestTrue(TEXT("火焰专精的前置和入向关系可渲染"), LxUITheme::RenderPreview(BP, FPaths::ProjectSavedDir() / TEXT("ProfessionMigration/火焰使者"), false,
		[Transfer](UUserWidget* Widget)
		{
			auto* UI = CastChecked<ULxProfessionWidget>(Widget); UI->UpdateUIComponents(Transfer);
			UI->SelectProfession(FGameplayTag::RequestGameplayTag(TEXT("职业.战斗.元素使.火焰使者")));
		}, Context.World));
	return true;
}
#endif
