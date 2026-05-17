#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxGameSettings.generated.h"

class ULxGameDataTablesManager;
class ULxInteractionUIManager;
class ULxUIManager;

/**
 * 项目全局开发者设置。
 * 配置保存到 Game 配置文件中，用于指定运行时需要创建的核心管理器类型。
 */
UCLASS(config=Game, defaultconfig, DisplayName="游戏设置")
class LXARPG_API ULxGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 游戏数据表管理器类型，用于加载物品、词条、属性等数据表。 */
	UPROPERTY(EditDefaultsOnly, config, Category="Data|ManagerClass", DisplayName="游戏数据表管理器类型")
	TSubclassOf<ULxGameDataTablesManager> GameDataTablesManagerClass;

	/** UI管理器类型，用于创建和管理HUD、背包、弹窗等通用UI功能。 */
	UPROPERTY(EditDefaultsOnly, config, Category="UI|ManagerClass", DisplayName="UI管理器类型")
	TSubclassOf<ULxUIManager> UIManagerClass;

	/** 交互UI管理器类型，用于创建和管理交互入口、对话、容器等交互相关UI。 */
	UPROPERTY(EditDefaultsOnly, config, Category="UI|ManagerClass", DisplayName="交互UI管理器类型")
	TSubclassOf<ULxInteractionUIManager> InteractionUIManagerClass;
};
