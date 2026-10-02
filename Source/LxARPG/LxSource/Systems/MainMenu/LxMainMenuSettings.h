#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LxMainMenuSettings.generated.h"

class ALxPlayerCharacter;
class AGameModeBase;
class UAnimSequence;
class UWorld;
class UDataLayerAsset;
class ULxMainMenuWidget;

/** 主菜单、初始角色与场景展示的项目配置。 */
UCLASS(config=Game, defaultconfig, DisplayName="主菜单设置")
class LXARPG_API ULxMainMenuSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** 使用当前项目已有的关卡、角色和游戏模式作为默认配置。 */
	ULxMainMenuSettings();
	/** 可替换的主菜单控件蓝图；资源缺失时使用原生界面。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|界面", DisplayName="主菜单界面类型")
	TSoftClassPtr<ULxMainMenuWidget> MenuWidgetClass;
	/** 独立存档目录前缀，与旧单槽文件并存。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|存档", DisplayName="目录前缀")
	FString SavePrefix = TEXT("LxARPG_Profiles");
	/** 首次运行或旧档缺少位置时使用的基础关卡。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|场景", DisplayName="默认场景")
	TSoftObjectPtr<UWorld> DefaultLevel;
	/** 首次运行和旧档缺少角色类时使用的可玩角色。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|角色", DisplayName="默认角色类型")
	TSoftClassPtr<ALxPlayerCharacter> DefaultCharacter;
	/** 返回主菜单时打开的轻量入口关卡。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|场景", DisplayName="菜单入口关卡")
	TSoftObjectPtr<UWorld> MenuLevel;
	/** 正式进入游戏所使用的游戏模式，通常配置项目现有蓝图。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|流程", DisplayName="正式游戏模式")
	TSoftClassPtr<AGameModeBase> GameplayMode;
	/** 可选待机序列，必须兼容角色骨架；不设置时保持角色参考姿势。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|角色", DisplayName="展示待机动画")
	TSoftObjectPtr<UAnimSequence> IdleAnimation;
	/** 预览需要激活的环境数据层，不依赖关卡蓝图开始事件。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|场景", DisplayName="预览环境数据层")
	TArray<TSoftObjectPtr<UDataLayerAsset>> PreviewDataLayers;
	/** 镜头距角色的距离，单位厘米。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|镜头", DisplayName="镜头距离", meta=(ClampMin="100"))
	float CameraDistance = 560.f;
	/** 镜头朝向的角色高度，单位厘米。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|镜头", DisplayName="镜头目标高度")
	float CameraHeight = 0.f;
	/** 目标点的横向偏移，让角色位于画面右侧。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|镜头", DisplayName="画面横向偏移")
	float CameraOffset = 90.f;
	/** 等待目标区域流送的最大秒数。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|场景", DisplayName="场景加载超时", meta=(ClampMin="5"))
	float StreamingTimeout = 45.f;
};
