#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LxARPG/LxSource/Model/Attribute/DataType/LxAttributeEnumType.h"
#include "LxCharacterRaceConfig.generated.h"

class ALxPlayerCharacter;
class ULxGameDataTablesManager;
struct FLxCharacterSaveRecord;

/** 可玩种族配置，每个种族唯一对应一个玩家角色子类。 */
USTRUCT(BlueprintType, DisplayName="角色种族配置")
struct LXARPG_API FLxCharacterRaceConfig : public FTableRowBase
{
	GENERATED_BODY()

	/** 存档使用的稳定种族标识，数据表行名称不参与匹配。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="种族")
	ELxCharacterRaceType Race = ELxCharacterRaceType::None;

	/** 创建角色和存档界面使用的种族名称，打包后仍可读取。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="种族名称")
	FText RaceName;

	/** 该种族正式生成的玩家角色蓝图，必须是可实例化的玩家角色子类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="角色|种族配置", DisplayName="玩家角色类型")
	TSoftClassPtr<ALxPlayerCharacter> PlayerCharacterClass;
};

/** 新建角色与旧档迁移共用种族解析，避免预览和正式角色使用不同配置。 */
namespace LxCharacterRace
{
	/** 获取游戏设置指定的数据表管理类型默认对象，菜单启动前也能读取配置。 */
	LXARPG_API const ULxGameDataTablesManager* GetConfiguredManager();
	/** 从存档种族查询玩家类；旧档缺少种族时只从原角色配置迁移，失败不修改记录。 */
	LXARPG_API bool ResolveCharacterRecord(FLxCharacterSaveRecord& Record, FString& OutError);
	/** 根据指定种族建立新角色初始记录，失败不生成存档。 */
	LXARPG_API bool CreateCharacterRecord(ELxCharacterRaceType Race, FLxCharacterSaveRecord& OutRecord, FString& OutError);
}
