#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "LxARPG/LxSource/Model/Input/DataType/LxInputData.h"
#include "LxPlayerSettingsSaveData.generated.h"

/** 独立于角色和地图进度的本地玩家设置，只记录玩家覆盖的默认配置。 */
UCLASS(BlueprintType, DisplayName="玩家设置存档")
class LXARPG_API ULxPlayerSettingsSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	/** 设置存档格式版本；无法识别的版本保留原文件并拒绝覆盖。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="玩家设置|存档", DisplayName="格式版本")
	int32 FormatVersion = 1;

	/** 双副本递增修订号，读取时只选择已经完整校验的最新副本。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="玩家设置|存档", DisplayName="修订号")
	int64 Revision = 0;

	/** 非默认的按键覆盖；新增输入行为没有此记录时自动采用配置表预设。 */
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="玩家设置|输入", DisplayName="用户按键配置")
	TArray<FLxInputKeySetting> InputKeyOverrides;
};
