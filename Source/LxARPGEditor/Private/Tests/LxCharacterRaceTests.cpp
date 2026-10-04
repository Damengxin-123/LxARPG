#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/StrongObjectPtr.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/DatabaseSystem/LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxSaveProfiles.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

namespace
{
	/** 创建不依赖项目资产的人类配置，方便验证缺失、重复与错误类边界。 */
	FLxCharacterRaceConfig MakeHumanRace()
	{
		FLxCharacterRaceConfig Row;
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
	TestFalse(TEXT("未配置精灵时拒绝创建"), Manager->GetCharacterRaceConfig(ELxCharacterRaceType::Elves, Found, Error));
	Table->AddRow(TEXT("重复人类"), MakeHumanRace());
	TestFalse(TEXT("重复种族不能任取一行"), Manager->GetCharacterRaceConfigs(Rows, Error));
	TestTrue(TEXT("失败不返回部分选项"), Rows.IsEmpty());
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
	const TSubclassOf<ULxGameDataTablesManager> OriginalManager = Settings->GameDataTablesManagerClass;
	TStrongObjectPtr<UDataTable> OriginalTable(Manager->CharacterRaceTable);
	ON_SCOPE_EXIT { Settings->GameDataTablesManagerClass = OriginalManager; Manager->CharacterRaceTable = OriginalTable.Get(); };
	TStrongObjectPtr<UDataTable> Table(NewObject<UDataTable>());
	Table->RowStruct = FLxCharacterRaceConfig::StaticStruct();
	Table->AddRow(TEXT("人类"), MakeHumanRace());
	Settings->GameDataTablesManagerClass = ULxGameDataTablesManager::StaticClass();
	Manager->CharacterRaceTable = Table.Get();
	FString Error;
	FLxCharacterSaveRecord Record;
	TestTrue(TEXT("人类新角色可建立"), LxCharacterRace::CreateCharacterRecord(ELxCharacterRaceType::Human, Record, Error));
	TestTrue(TEXT("新档显式保存人类"), Record.CharacterRace == ELxCharacterRaceType::Human);
	TestFalse(TEXT("首次进入保留蓝图初始内容"), Record.bHasGameplayData);
	Record.BackpackSlotCount = 37;
	Record.bHasGameplayData = true;
	Record.CharacterClass = APawn::StaticClass();
	TestTrue(TEXT("已存种族覆盖旧的角色类缓存"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("角色类型来自人类配置表"), Record.CharacterClass.Get() == ALxPlayerCharacter::StaticClass());
	Record.CharacterRace = ELxCharacterRaceType::None;
	TestTrue(TEXT("缺少种族的旧档可从唯一类映射迁移"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("迁移得到人类种族"), Record.CharacterRace == ELxCharacterRaceType::Human);
	TestEqual(TEXT("旧档背包容量不丢失"), Record.BackpackSlotCount, 37);
	TestTrue(TEXT("旧档进度标记不丢失"), Record.bHasGameplayData);
	Record.CharacterRace = ELxCharacterRaceType::Elves;
	TestFalse(TEXT("没有精灵配置时保留错误"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
	TestTrue(TEXT("失败不把精灵覆盖成人类"), Record.CharacterRace == ELxCharacterRaceType::Elves);
	TestEqual(TEXT("失败保留原进度"), Record.BackpackSlotCount, 37);
	TestFalse(TEXT("新建不接受无效种族"), LxCharacterRace::CreateCharacterRecord(ELxCharacterRaceType::None, Record, Error));
	FLxCharacterRaceConfig Other = MakeHumanRace();
	Other.Race = ELxCharacterRaceType::Elves;
	Table->AddRow(TEXT("精灵"), Other);
	Record.CharacterRace = ELxCharacterRaceType::None;
	TestFalse(TEXT("旧档类对应多个种族时禁止猜测"), LxCharacterRace::ResolveCharacterRecord(Record, Error));
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
	TestEqual(TEXT("当前仅配置人类"), Rows.Num(), 1);
	TestTrue(TEXT("首个可选种族为人类"), Rows[0].Race == ELxCharacterRaceType::Human);
	TestEqual(TEXT("种族显示名称为中文"), Rows[0].RaceName.ToString(), FString(TEXT("人类")));
	TestEqual(TEXT("使用当前玩家角色"), Rows[0].PlayerCharacterClass.ToSoftObjectPath().ToString(),
		FString(TEXT("/Game/项目内容/实体资产/角色/测试角色-人类法师/测试角色-玩家控制角色.测试角色-玩家控制角色_C")));
	TStrongObjectPtr<ULxCharacterProfileSave> Save(NewObject<ULxCharacterProfileSave>());
	if (!TestTrue(TEXT("实际人类角色创建成功"), LxCharacterRace::CreateCharacterRecord(ELxCharacterRaceType::Human, Save->Record, Error))) return false;
	TArray<uint8> Bytes;
	TestTrue(TEXT("序列化角色档"), UGameplayStatics::SaveGameToMemory(Save.Get(), Bytes));
	TStrongObjectPtr<ULxCharacterProfileSave> Loaded(Cast<ULxCharacterProfileSave>(UGameplayStatics::LoadGameFromMemory(Bytes)));
	if (!TestNotNull(TEXT("读取角色档"), Loaded.Get())) return false;
	TestTrue(TEXT("重新读取仍保留种族"), Loaded->Record.CharacterRace == ELxCharacterRaceType::Human);
	Loaded->Record.CharacterClass.Reset();
	TestTrue(TEXT("仅凭种族即可恢复玩家类型"), LxCharacterRace::ResolveCharacterRecord(Loaded->Record, Error));
	TestEqual(TEXT("类型解析与表一致"), Loaded->Record.CharacterClass.ToSoftObjectPath(), Rows[0].PlayerCharacterClass.ToSoftObjectPath());
	return true;
}

#endif
