#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxGameSettings.generated.h"

class ULxGameDataTablesManager;
class ULxInteractionUIManager;
class ULxUIManager;

/** 项目全局开发者设置，保存运行时需要创建的核心管理器类型。 */
UCLASS(config=Game, defaultconfig, DisplayName="游戏设置")
class LXARPG_API ULxGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 游戏数据表管理器类型，用于加载物品、词条、属性等数据表。 */
	UPROPERTY(EditDefaultsOnly, config, Category="数据|管理器类型", DisplayName="游戏数据表管理器类型")
	TSubclassOf<ULxGameDataTablesManager> GameDataTablesManagerClass;

	/** 主 UI 管理器类型，用于创建主 UI 蓝图并统一管理所有子 UI 层级。 */
	UPROPERTY(EditDefaultsOnly, config, Category="UI|管理器类型", DisplayName="UI管理器类型")
	TSubclassOf<ULxUIManager> UIManagerClass;

	/** 交互 UI 管理器类型。该对象现在由主 UI 管理器创建，不再单独 AddToPlayerScreen。 */
	UPROPERTY(EditDefaultsOnly, config, Category="UI|管理器类型", DisplayName="交互UI管理器类型")
	TSubclassOf<ULxInteractionUIManager> InteractionUIManagerClass;
};
