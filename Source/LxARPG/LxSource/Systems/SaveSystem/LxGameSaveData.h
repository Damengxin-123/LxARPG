#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LxCharacterSaveData.h"
#include "LxInteractionSaveData.h"
#include "LxGameSaveData.generated.h"

/** 游戏实例的存档，只保存可重建运行对象的属性和稳定标识。 */
UCLASS(BlueprintType, DisplayName="游戏存档数据")
class LXARPG_API ULxGameSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	/** 存档格式版本；不兼容的版本不会被自动覆盖。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="存档", DisplayName="格式版本")
	int32 FormatVersion = 1;

	/** 通过玩家存档标签精确索引背包、装备、任务和职业。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="存档", DisplayName="玩家数据")
	TMap<FGameplayTag, FLxCharacterSaveRecord> Players;

	/** 通过全局唯一的交互对象标签精确索引所有交互功能。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="存档", DisplayName="交互对象数据")
	TMap<FGameplayTag, FLxInteractionSaveRecord> Interactions;
};
