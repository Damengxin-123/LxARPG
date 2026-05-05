#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LxGameSaveData.generated.h"

/**
 * 游戏存档数据基类。
 *
 * 当前只作为项目存档类型占位，后续需要持久化角色、背包、
 * 任务或设置数据时，可以在该类中添加对应字段。
 */
UCLASS()
class LXARPG_API ULxGameSaveData : public USaveGame
{
	GENERATED_BODY()
};
