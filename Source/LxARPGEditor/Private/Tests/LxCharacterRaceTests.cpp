#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "NativeGameplayTags.h"
#include "Components/ComboBoxString.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LxARPG/LxSource/Systems/LxGameInstanceSubsystem.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveManager.h"
#include "LxARPG/LxSource/UI/MainMenu/LxMainMenuWidget.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterPresentation.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfiles.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"

namespace
{
	/** 测试专用配置标签，不依赖项目资产的具体配置。 */
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TestConfigA, "测试.角色配置.甲");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TestConfigB, "测试.角色配置.乙");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TestMissingConfig, "测试.角色配置.未配置");

	/** 创建不依赖项目资产的人类配置，方便验证缺失、重复与错误类边界。 */
	FLxCharacterRaceConfig MakeHumanRace()
	{
		FLxCharacterRaceConfig Row;
		Row.CharacterConfigID = TestConfigA;
		Row.Race = ELxCharacterRaceType::Human;
		Row.RaceName = FText::FromString(TEXT("人类"));
		Row.PlayerCharacterClass = ALxPlayerCharacter::StaticClass();
		return Row;
	}
}

/** 配置错误必须阻止查询，不能随机取重复行或静默退回默认角色。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRaceValidationTest, "LxARPG.Menu.Race.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRaceValidationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<ULxGameDataTablesManager> Manager(NewObject<ULxGameDataTablesManager>());
	TStrongObjectPtr<UDataTable> Table(NewObject<UDataTable>());
	TArray<FLxCharacterRaceConfig> Rows;
	FString Error;
	TestFalse(TEXT("缺少表时拒绝查询"), Manager->GetCharacterRaceConfigs(Rows, Error));
	Manager->CharacterRaceTable = Table.Get();
	TestFalse(TEXT("行结构不符时拒绝查询"), Manager->GetCharacterRaceConfigs(Rows, Error));
	Table->RowStruct = FLxCharacterRaceConfig::StaticStruct();
	TestFalse(TEXT("空表不能创建角色"), Manager->GetCharacterRaceConfigs(Rows, Error));
	Table->AddRow(TEXT("人类"), MakeHumanRace());
	TestTrue(TEXT("有效的人类配置可用"), Manager->GetCharacterRaceConfigs(Rows, Error));
	TestEqual(TEXT("只返回一个可玩种族"), Rows.Num(), 1);
	FLxCharacterRaceConfig Found;
	TestFalse(TEXT("未配置标签时拒绝查询"), Manager->GetCharacterRaceConfig(TestMissingConfig, Found, Error));
	Table->AddRow(TEXT("重复人类"), MakeHumanRace());
	TestFalse(TEXT("重复配置标签不能任取一行"), Manager->GetCharacterRaceConfigs(Rows, Error));
	TestTrue(TEXT("失败不返回部分选项"), Rows.IsEmpty());
	FLxCharacterRaceConfig Second = MakeHumanRace();
	Second.CharacterConfigID = TestConfigB;
	Table->AddRow(TEXT("重复人类"), Second);
	TestTrue(TEXT("标签不同允许种族名称和角色类全部相同"), Manager->GetCharacterRaceConfigs(Rows, Error));
	TestEqual(TEXT("同种族同名配置均保留"), Rows.Num(), 2);
	TestTrue(TEXT("精确标签选中第二配置"), Manager->GetCharacterRaceConfig(TestConfigB, Found, Error));
	TestTrue(TEXT("查询没有退回首个人类"), Found.CharacterConfigID == TestConfigB);
	Second.CharacterConfigID = FGameplayTag();
	Table->AddRow(TEXT("重复人类"), Second);
	TestFalse(TEXT("未配置ID的行被拒绝"), Manager->GetCharacterRaceConfigs(Rows, Error));
	// UE 5.6 的 RemoveRow 通知会查询已删除的行；清空测试表后重建下一种边界数据。
	Table->EmptyTable();
	FLxCharacterRaceConfig Invalid = MakeHumanRace();
	Invalid.PlayerCharacterClass = FSoftObjectPath(APawn::StaticClass());
	Table->AddRow(TEXT("人类"), Invalid);
	TestFalse(TEXT("普通Pawn不能冒充玩家角色"), Manager->GetCharacterRaceConfigs(Rows, Error));
	Invalid.PlayerCharacterClass.Reset();
	Table->AddRow(TEXT("人类"), Invalid);
	TestFalse(TEXT("角色类为空时拒绝查询"), Manager->GetCharacterRaceConfigs(Rows, Error));
	return true;
}

/** 种族决定角色类；旧档迁移保留进度，配置失败不修改输入记录。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRaceResolutionTest, "LxARPG.Menu.Race.SaveResolution", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRaceResolutionTest::RunTest(const FString& Parameters)
{
	ULxGameSettings* Settings = GetMutableDefault<ULxGameSettings>();
	ULxGameDataTablesManager* Manager = GetMutableDefault<ULxGameDataTablesManager>();
	const TSoftClassPtr<ULxGameDataTablesManager> OriginalManager = Settings->GameDataTablesManagerClass;
	TStrongObjectPtr<UDataTable> OriginalTable(Manager->CharacterRaceTable);
	ON_SCOPE_EXIT { Settings->GameDataTablesManagerClass = OriginalManager; Manager->CharacterRaceTable = OriginalTable.Get(); };
	TStrongObjectPtr<UDataTable> Table(NewObject<UDataTable>());
	Table->RowStruct = FLxCharacterRaceConfig::StaticStruct();
	Table->AddRow(TEXT("人类"), MakeHumanRace());
	Settings->GameDataTablesManagerClass = ULxGameDataTablesManager::StaticClass();
	Manager->CharacterRaceTable = Table.Get();
	FString Error;
	FLxCharacterSaveRecord Record;
	TestTrue(TEXT("人类新角色可建立"), LxCharacterRace::CreateCharacterRecord(TestConfigA, Record, Error));
	TestTrue(TEXT("新档保存配置标签"), Record.CharacterConfigID == TestConfigA);
	TestTrue(TEXT("新档显式保存人类"), Record.CharacterRace == ELxCharacterRaceType::Human);
	TestFalse(TEXT("首次进入保留蓝图初始内容"), Record.bHasGameplayData);
	Record.BackpackSlotCount = 37;
	Record.bHasGameplayData = true;
	Record.CharacterClass = APawn::StaticClass();
	TestTrue(TEXT("已存标签覆盖旧的角色类缓存"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("角色类型来自人类配置表"), Record.CharacterClass.Get() == ALxPlayerCharacter::StaticClass());
	Record.CharacterRace = ELxCharacterRaceType::None;
	Record.CharacterConfigID = FGameplayTag();
	TestTrue(TEXT("缺少种族的旧档可从唯一类映射迁移"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("迁移得到人类种族"), Record.CharacterRace == ELxCharacterRaceType::Human);
	TestEqual(TEXT("旧档背包容量不丢失"), Record.BackpackSlotCount, 37);
	TestTrue(TEXT("旧档进度标记不丢失"), Record.bHasGameplayData);
	Record.CharacterRace = ELxCharacterRaceType::Elves;
	Record.CharacterConfigID = FGameplayTag();
	TestFalse(TEXT("没有精灵配置时保留错误"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("失败不把精灵覆盖成人类"), Record.CharacterRace == ELxCharacterRaceType::Elves);
	TestEqual(TEXT("失败保留原进度"), Record.BackpackSlotCount, 37);
	TestFalse(TEXT("新建不接受空标签"), LxCharacterRace::CreateCharacterRecord(FGameplayTag(), Record, Error));
	FLxCharacterRaceConfig Other = MakeHumanRace();
	Other.CharacterConfigID = TestConfigB;
	Table->AddRow(TEXT("同种族同名"), Other);
	Record.CharacterRace = ELxCharacterRaceType::None;
	TestFalse(TEXT("旧档类对应多个配置时禁止猜测"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestFalse(TEXT("迁移失败不写入配置标签"), Record.CharacterConfigID.IsValid());
	Record.CharacterClass.Reset();
	Record.CharacterRace = ELxCharacterRaceType::Human;
	TestFalse(TEXT("仅有种族且有多个配置时禁止猜测"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("同种族第二个配置可建立"), LxCharacterRace::CreateCharacterRecord(TestConfigB, Record, Error));
	TestTrue(TEXT("新建保留第二个配置身份"), Record.CharacterConfigID == TestConfigB);
	Record.CharacterConfigID = TestMissingConfig;
	TestFalse(TEXT("已存标签不存在时不回退到同种族配置"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("失败保留原标签"), Record.CharacterConfigID == TestMissingConfig);
	return true;
}

/** 从磁盘加载真实管理蓝图与人类表，验证新档的种族可序列化并再次解析。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRaceProjectAssetsTest, "LxARPG.Menu.Race.ProjectAssets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRaceProjectAssetsTest::RunTest(const FString& Parameters)
{
	const ULxGameDataTablesManager* Manager = LxCharacterRace::GetConfiguredManager();
	if (!TestNotNull(TEXT("项目已配置数据表管理类型"), Manager)) return false;
	TArray<FLxCharacterRaceConfig> Rows;
	FString Error;
	if (!TestTrue(TEXT("实际种族表可加载"), Manager->GetCharacterRaceConfigs(Rows, Error))) { AddError(Error); return false; }
	FLxCharacterRaceConfig DefaultConfig;
	if (!TestTrue(TEXT("默认角色标签可查询"), Manager->GetCharacterRaceConfig(GetDefault<ULxMainMenuSettings>()->DefaultCharacterConfigID, DefaultConfig, Error))) return false;
	// 比较解析后的类型，资产整理或重定向不应让正确的角色配置被误判为失败。
	UClass* ConfiguredPlayer = GetDefault<ULxMainMenuSettings>()->DefaultCharacter.LoadSynchronous();
	TestNotNull(TEXT("默认玩家角色可加载"), ConfiguredPlayer);
	TestEqual(TEXT("默认配置使用配置的玩家角色"), DefaultConfig.PlayerCharacterClass.LoadSynchronous(), ConfiguredPlayer);
	for (const FLxCharacterRaceConfig& Row : Rows)
	{
		TStrongObjectPtr<ULxCharacterProfileSave> Save(NewObject<ULxCharacterProfileSave>());
		if (!TestTrue(TEXT("每个实际角色配置创建成功"), LxCharacterRace::CreateCharacterRecord(Row.CharacterConfigID, Save->Record, Error))) return false;
		TArray<uint8> Bytes;
		TestTrue(TEXT("序列化角色档"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes));
		TStrongObjectPtr<ULxCharacterProfileSave> Loaded(Cast<ULxCharacterProfileSave>(UGameplayStatics::LoadGameFromMemory(Bytes)));
		if (!TestNotNull(TEXT("读取角色档"), Loaded.Get())) return false;
		TestTrue(TEXT("重新读取仍保留种族"), Loaded->Record.CharacterRace == Row.Race);
		TestTrue(TEXT("重新读取仍保留配置标签"), Loaded->Record.CharacterConfigID == Row.CharacterConfigID);
		Loaded->Record.CharacterClass.Reset();
		TestTrue(TEXT("仅凭标签即可恢复玩家类型"), LxCharacterRace::ResolveCharacterRecord(Loaded->Record, Error));
		TestEqual(TEXT("类型解析与对应行一致"), Loaded->Record.CharacterClass.ToSoftObjectPath(), Row.PlayerCharacterClass.ToSoftObjectPath());
		// 模拟升级前的存档；未来多个配置共用角色类时，应拒绝有歧义的旧档迁移。
		Loaded->Record.CharacterConfigID = FGameplayTag();
		const int32 LegacyMatches = Rows.FilterByPredicate([&Row](const FLxCharacterRaceConfig& Candidate)
		{
			return Candidate.Race == Row.Race && Candidate.PlayerCharacterClass.LoadSynchronous() == Row.PlayerCharacterClass.LoadSynchronous();
		}).Num();
		TestEqual(TEXT("旧档仅在原角色类和种族唯一匹配时迁移"), LxCharacterRace::ResolveCharacterRecord(Loaded->Record, Error), LegacyMatches == 1);
		if (LegacyMatches == 1) TestTrue(TEXT("迁移没有选错同种族角色"), Loaded->Record.CharacterConfigID == Row.CharacterConfigID);
	}
	return true;
}

/** 验证相同显示文字的下拉选项保留独立索引，且运行时采集不丢失第二个配置身份。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRaceDuplicateNameTest, "LxARPG.Menu.Race.DuplicateNamesAndCapture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRaceDuplicateNameTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UComboBoxString> Combo(NewObject<UComboBoxString>());
	Combo->AddOption(TEXT("人类"));
	Combo->AddOption(TEXT("人类"));
	Combo->TakeWidget();
	Combo->SetSelectedIndex(1);
	TestEqual(TEXT("同名第二选项仍是索引1"), Combo->GetSelectedIndex(), 1);
	TestEqual(TEXT("同名选项不会被合并"), Combo->GetOptionCount(), 2);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ALxPlayerCharacter* Player = World->SpawnActor<ALxPlayerCharacter>();
	if (!TestNotNull(TEXT("测试角色可生成"), Player)) return false;
	Player->SetCharacterConfigIDForSession(TestConfigB);
	Player->SetCharacterRaceForSession(ELxCharacterRaceType::Human);
	FLxCharacterSaveRecord Record;
	LxCharacterPresentation::Capture(Player, Record);
	TestTrue(TEXT("采集保留选择的配置标签"), Record.CharacterConfigID == TestConfigB);
	TestTrue(TEXT("采集同时保留种族逻辑分类"), Record.CharacterRace == ELxCharacterRaceType::Human);
	return true;
}

/** 验证实际主菜单蓝图从标签配置填充全部选项，同名行不合并且仍能解析到各自配置。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxRaceBlueprintOptionsTest, "LxARPG.Menu.Race.BlueprintOptions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLxRaceBlueprintOptionsTest::RunTest(const FString& Parameters)
{
	UClass* MenuClass = GetDefault<ULxMainMenuSettings>()->MenuWidgetClass.LoadSynchronous();
	if (!TestNotNull(TEXT("项目主菜单蓝图可加载"), MenuClass)) return false;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	World->SetGameInstance(GameInstance.Get());
	GameInstance->Init();
	ULxGameInstanceSubsystem* Global = GameInstance->GetSubsystem<ULxGameInstanceSubsystem>();
	if (Global && Global->GetSaveManager()) Global->GetSaveManager()->SetReadOnly(true);
	ON_SCOPE_EXIT { GameInstance->Shutdown(); World->SetGameInstance(nullptr); };
	TStrongObjectPtr<ULxMainMenuWidget> Menu(NewObject<ULxMainMenuWidget>(World, MenuClass));
	Menu->Initialize();
	TSharedPtr<SWidget> Slate = Menu->TakeWidget();
	ON_SCOPE_EXIT { Slate.Reset(); Menu->ReleaseSlateResources(true); };
	Menu->OpenCharacterPanel();
	UComboBoxString* Combo = Cast<UComboBoxString>(Menu->GetWidgetFromName(TEXT("种族列表")));
	if (!TestNotNull(TEXT("实际蓝图存在种族下拉框"), Combo)) return false;
	const TArray<FLxCharacterRaceConfig> Configs = Menu->GetAvailableCharacterRaces();
	TestEqual(TEXT("蓝图显示表中的全部配置"), Combo->GetOptionCount(), Configs.Num());
	TestTrue(TEXT("实际项目保留两个人类配置"), Configs.Num() >= 2);
	for (int32 Index = 0; Index < Configs.Num(); ++Index)
	{
		TestEqual(TEXT("每个选项显示对应配置名称"), Combo->GetOptionAtIndex(Index), Configs[Index].RaceName.ToString());
	}
	ULxGameDataTablesManager* Manager = const_cast<ULxGameDataTablesManager*>(LxCharacterRace::GetConfiguredManager());
	if (!Manager) return false;
	TStrongObjectPtr<UDataTable> OriginalTable(Manager->CharacterRaceTable);
	ON_SCOPE_EXIT { Manager->CharacterRaceTable = OriginalTable.Get(); };
	TStrongObjectPtr<UDataTable> SameNameTable(NewObject<UDataTable>());
	SameNameTable->RowStruct = FLxCharacterRaceConfig::StaticStruct();
	for (FLxCharacterRaceConfig Config : Configs)
	{
		Config.RaceName = FText::FromString(TEXT("同名角色"));
		SameNameTable->AddRow(Config.CharacterConfigID.GetTagName(), Config);
	}
	Manager->CharacterRaceTable = SameNameTable.Get();
	Menu->ClosePanel();
	Menu->OpenCharacterPanel();
	TestEqual(TEXT("蓝图同名选项保持完整数量"), Combo->GetOptionCount(), Configs.Num());
	for (int32 Index = 0; Index < Configs.Num(); ++Index)
	{
		Combo->SetSelectedIndex(Index);
		TestEqual(TEXT("蓝图同名选项可独立选中"), Combo->GetSelectedIndex(), Index);
		const TArray<FLxCharacterRaceConfig> SelectedConfigs = Menu->GetAvailableCharacterRaces();
		FLxCharacterSaveRecord Record;
		FString Error;
		if (!SelectedConfigs.IsValidIndex(Combo->GetSelectedIndex())) return false;
		TestTrue(TEXT("选项对应标签可建立角色记录"), LxCharacterRace::CreateCharacterRecord(SelectedConfigs[Combo->GetSelectedIndex()].CharacterConfigID, Record, Error));
		TestTrue(TEXT("同名选项没有解析到其他配置"), Record.CharacterConfigID == Configs[Index].CharacterConfigID);
	}
	return true;
}

#endif
