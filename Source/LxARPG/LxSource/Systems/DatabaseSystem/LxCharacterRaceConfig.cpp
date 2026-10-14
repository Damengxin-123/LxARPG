#include "LxCharacterRaceConfig.h"

#include "LxGameDataTablesManager.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"
#include "LxARPG/LxSource/Systems/MainMenu/LxMainMenuSettings.h"
#include "LxARPG/LxSource/Systems/SaveSystem/LxCharacterSaveData.h"
#include "LxARPG/LxSource/Systems/SettingSystem/LxGameSettings.h"

const ULxGameDataTablesManager* LxCharacterRace::GetConfiguredManager()
{
	const TSubclassOf<ULxGameDataTablesManager> ManagerClass = GetDefault<ULxGameSettings>()->GameDataTablesManagerClass.LoadSynchronous();
	return ManagerClass ? ManagerClass->GetDefaultObject<ULxGameDataTablesManager>() : nullptr;
}

bool LxCharacterRace::ResolveCharacterRecord(FLxCharacterSaveRecord& Record, FString& OutError)
{
	const ULxGameDataTablesManager* Manager = GetConfiguredManager();
	TArray<FLxCharacterRaceConfig> Configs;
	if (!Manager) { OutError = TEXT("未配置游戏数据表管理器类型。"); return false; }
	if (!Manager->GetCharacterRaceConfigs(Configs, OutError)) return false;

	const FLxCharacterRaceConfig* Config = nullptr;
	if (Record.CharacterConfigID.IsValid())
	{
		// 新档身份只由精确标签决定，不能因标签失效而回退到另一个同种族角色。
		Config = Configs.FindByPredicate([&Record](const FLxCharacterRaceConfig& Item)
		{
			return Item.CharacterConfigID == Record.CharacterConfigID;
		});
	}
	else
	{
		// 旧档优先用已保存的角色类匹配；种族仅辅助过滤，不能区分同种族的多个配置。
		UClass* LegacyClass = nullptr;
		if (!Record.CharacterClass.IsNull())
		{
			LegacyClass = Record.CharacterClass.LoadSynchronous();
			if (!LegacyClass) { OutError = TEXT("旧角色类型无法加载，不能迁移角色配置ID。"); return false; }
		}
		else if (Record.CharacterRace == ELxCharacterRaceType::None)
		{
			LegacyClass = GetDefault<ULxMainMenuSettings>()->DefaultCharacter.LoadSynchronous();
			if (!LegacyClass) { OutError = TEXT("旧档缺少角色身份且默认角色类型无法加载。"); return false; }
		}
		for (const FLxCharacterRaceConfig& Candidate : Configs)
		{
			if (LegacyClass && Candidate.PlayerCharacterClass.LoadSynchronous() != LegacyClass) continue;
			if (Record.CharacterRace != ELxCharacterRaceType::None && Candidate.Race != Record.CharacterRace) continue;
			if (Config) { OutError = TEXT("旧角色档案匹配多个角色配置，无法唯一确定角色配置ID。"); return false; }
			Config = &Candidate;
		}
	}
	if (!Config) { OutError = TEXT("角色配置ID或旧档身份未匹配可玩角色，请检查角色种族表。"); return false; }
	Record.CharacterConfigID = Config->CharacterConfigID;
	Record.CharacterRace = Config->Race;
	Record.CharacterClass = Config->PlayerCharacterClass.ToSoftObjectPath();
	OutError.Reset();
	return true;
}

bool LxCharacterRace::CreateCharacterRecord(FGameplayTag CharacterConfigID, FLxCharacterSaveRecord& OutRecord, FString& OutError)
{
	if (!CharacterConfigID.IsValid()) { OutError = TEXT("请选择有效的角色配置ID。"); return false; }
	FLxCharacterSaveRecord Record;
	Record.CharacterConfigID = CharacterConfigID;
	Record.bHasGameplayData = false;
	Record.LevelPath = GetDefault<ULxMainMenuSettings>()->DefaultLevel.ToSoftObjectPath();
	if (!ResolveCharacterRecord(Record, OutError)) return false;
	const ALxPlayerCharacter* Defaults = Cast<ALxPlayerCharacter>(Record.CharacterClass.LoadSynchronous()->GetDefaultObject());
	Record.SaveID = Defaults->GetCharacterIDTag();
	if (!Record.SaveID.IsValid()) { OutError = TEXT("种族角色没有有效的玩家存档ID。"); return false; }
	OutRecord = MoveTemp(Record);
	return true;
}
