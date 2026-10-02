#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LxMenuPreferences.generated.h"

/** 用户全局音量与操作偏好，与所有角色和地图存档独立。 */
UCLASS(config=GameUserSettings, DisplayName="全局菜单偏好")
class LXARPG_API ULxMenuPreferences : public UObject
{
	GENERATED_BODY()
public:
	/** 主音量比例，零为静音，一为默认音量。 */
	UPROPERTY(config, EditAnywhere, Category="设置|声音", DisplayName="主音量", meta=(ClampMin="0", ClampMax="1"))
	float MasterVolume = 1.f;
	/** 视角输入倍率，影响本地玩家的水平和垂直转向。 */
	UPROPERTY(config, EditAnywhere, Category="设置|操作", DisplayName="视角灵敏度", meta=(ClampMin="0.1", ClampMax="3"))
	float LookSensitivity = 1.f;
	/** 是否反转玩家视角的垂直输入。 */
	UPROPERTY(config, EditAnywhere, Category="设置|操作", DisplayName="反转垂直视角")
	bool bInvertLookY = false;
	/** 校验配置并把音量应用到当前世界的音频设备。 */
	void Apply(UWorld* World);
};
