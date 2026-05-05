#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxGameSettings.generated.h"

class ULxGameDataTablesManager;
class ULxUIManager;

/**
 * 项目全局开发者设置。
 *
 * 配置保存在 Game 配置文件中，用于指定运行时需要创建的核心管理器类型。
 */
UCLASS(config=Game, defaultconfig, DisplayName="游戏设置")
class LXARPG_API ULxGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 游戏数据表管理器类型，用于加载物品、词条、属性等数据表。 */
	UPROPERTY(EditDefaultsOnly, config, Category="Data|ManagerClass", DisplayName="游戏数据表管理器类型")
	TSubclassOf<ULxGameDataTablesManager> GameDataTablesManagerClass;

	/** UI 管理器类型，用于创建和管理 HUD、背包、弹窗等 UI 功能。 */
	UPROPERTY(EditDefaultsOnly, config, Category="UI|ManagerClass", DisplayName="UI管理器类型")
	TSubclassOf<ULxUIManager> UIManagerClass;
};
