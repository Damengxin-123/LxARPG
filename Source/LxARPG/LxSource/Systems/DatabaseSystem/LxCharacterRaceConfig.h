#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxCharacterRaceConfig.generated.h"

class ALxPlayerCharacter;
class ULxGameDataTablesManager;
struct FLxCharacterSaveRecord;

/** 按标签唯一索引的可玩角色配置，种族与显示名称允许重复。 */
USTRUCT(BlueprintType, DisplayName="角色种族配置")
struct LXARPG_API FLxCharacterRaceConfig : public FTableRowBase
{
	GENERATED_BODY()

	/** 配置和存档共用的唯一标签，行名称、显示名称和种族不参与身份匹配。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="角色配置ID")
	FGameplayTag CharacterConfigID;

	/** 用于种族逻辑判断的分类枚举，不作为配置索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="种族")
	ELxCharacterRaceType Race = ELxCharacterRaceType::None;

	/** 创建角色和存档界面使用的种族名称，打包后仍可读取。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="种族名称")
	FText RaceName;

	/** 该配置生成的玩家角色蓝图，必须是可实例化的玩家角色子类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="玩家角色类型")
	TSoftClassPtr<ALxPlayerCharacter> PlayerCharacterClass;
};

/** 新建角色与旧档迁移共用种族解析，避免预览和正式角色使用不同配置。 */
namespace LxCharacterRace
{
	/** 获取游戏设置指定的数据表管理类型默认对象，菜单启动前也能读取配置。 */
	LXARPG_API const ULxGameDataTablesManager* GetConfiguredManager();
	/** 按存档配置标签查询玩家类；旧档只在原角色类或种族能唯一匹配时迁移，失败不修改记录。 */
	LXARPG_API bool ResolveCharacterRecord(FLxCharacterSaveRecord& Record, FString& OutError);
	/** 根据唯一配置标签建立新角色初始记录，失败不修改输出记录。 */
	LXARPG_API bool CreateCharacterRecord(FGameplayTag CharacterConfigID, FLxCharacterSaveRecord& OutRecord, FString& OutError);
}
