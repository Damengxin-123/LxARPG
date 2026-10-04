#include "LxCharacterRaceConfig.h"

#include "LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveData.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

const ULxGameDataTablesManager* LxCharacterRace::GetConfiguredManager()
{
	const TSubclassOf<ULxGameDataTablesManager> ManagerClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass;
	return ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
}

bool LxCharacterRace::ResolveCharacterRecord(FLxCharacterSaveRecord& Record, FString& OutError)
{
	const ULxGameDataTablesManager* Manager = GetConfiguredManager();
	TArray<FLxCharacterRaceConfig> Configs;
	if (!Manager) { OutError = TEXT("未配置游戏数据表管理器类型。"); return false; }
	if (!Manager->GetCharacterRaceConfigs(Configs, OutError)) return false;

	ELxCharacterRaceType Race = Record.CharacterRace;
	if (Race == ELxCharacterRaceType::None)
	{
		// 旧档允许使用原有默认角色补齐缺失类；新档始终显式指定种族。
		const UClass* LegacyClass = Record.CharacterClass.IsNull()
			? GetDefault<ULxMainMenuSettings>()->DefaultCharacter.LoadSynchronous()
			: Record.CharacterClass.LoadSynchronous();
		const ALxPlayerCharacter* Defaults = LegacyClass ? Cast<ALxPlayerCharacter>(LegacyClass->GetDefaultObject()) : nullptr;
		if (!Defaults) { OutError = TEXT("旧角色类型无法加载，不能确定存档种族。"); return false; }
		Race = Defaults->GetCharacterRace();
		if (Race == ELxCharacterRaceType::None)
		{
			// 更早的蓝图没有种族字段时，通过唯一的类映射迁移，禁止猜测为人类。
			for (const FLxCharacterRaceConfig& Config : Configs)
			{
				if (Config.PlayerCharacterClass.ToSoftObjectPath() != FSoftObjectPath(LegacyClass)) continue;
				if (Race != ELxCharacterRaceType::None) { OutError = TEXT("旧角色类型对应多个种族，无法自动迁移。"); return false; }
				Race = Config.Race;
			}
		}
	}
	const FLxCharacterRaceConfig* Config = Configs.FindByPredicate([Race](const FLxCharacterRaceConfig& Item) { return Item.Race == Race; });
	if (!Config) { OutError = TEXT("角色种族未配置可玩的角色类型，请检查角色种族表。"); return false; }
	Record.CharacterRace = Race;
	Record.CharacterClass = Config->PlayerCharacterClass.ToSoftObjectPath();
	OutError.Reset();
	return true;
}

bool LxCharacterRace::CreateCharacterRecord(ELxCharacterRaceType Race, FLxCharacterSaveRecord& OutRecord, FString& OutError)
{
	if (Race == ELxCharacterRaceType::None) { OutError = TEXT("请选择有效的角色种族。"); return false; }
	FLxCharacterSaveRecord Record;
	Record.CharacterRace = Race;
	Record.bHasGameplayData = false;
	Record.LevelPath = GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath();
	if (!ResolveCharacterRecord(Record, OutError)) return false;
	const ALxPlayerCharacter* Defaults = Cast<ALxPlayerCharacter>(Record.CharacterClass.LoadSynchronous()->GetDefaultObject());
	Record.SaveID = Defaults->GetCharacterIDTag();
	if (!Record.SaveID.IsValid()) { OutError = TEXT("种族角色没有有效的玩家存档ID。"); return false; }
	OutRecord = MoveTemp(Record);
	return true;
}
