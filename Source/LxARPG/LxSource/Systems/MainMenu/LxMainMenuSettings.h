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
class ULxPauseMenuWidget;

/** 主菜单、初始角色与场景展示的项目配置。 */
UCLASS(config=Game, defaultconfig, DisplayName="主菜单设置")
class LXARPG_API ULxMainMenuSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	/** 使用当前项目已有的关卡、角色和游戏模式作为默认配置。 */
	ULxMainMenuSettings();
	/** 提供完整布局的主菜单控件蓝图，必须继承主菜单逻辑基类。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|界面", DisplayName="主菜单界面类型")
	TSoftClassPtr<ULxMainMenuWidget> MenuWidgetClass;
	/** 游戏中按 Esc 打开的暂停菜单蓝图。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|界面", DisplayName="暂停菜单界面类型")
	TSoftClassPtr<ULxPauseMenuWidget> PauseMenuWidgetClass;
	/** 独立存档目录前缀，与旧单槽文件并存。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|存档", DisplayName="目录前缀")
	FString SavePrefix = TEXT("LxARPG_Profiles");
	/** 菜单展示与正式游玩共用的唯一总关卡，各地图档只切换其中的单位状态。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|场景", DisplayName="总关卡")
	TSoftObjectPtr<UWorld> DefaultLevel;
	/** 仅用于兼容没有种族和角色类的旧档；新建角色通过数据表管理器的角色种族表选择类型。 */
	UPROPERTY(EditAnywhere, config, Category="主菜单|角色", DisplayName="默认角色类型")
	TSoftClassPtr<ALxPlayerCharacter> DefaultCharacter;
	/** 仅保留旧配置的反序列化兼容；启动与返回菜单统一使用总关卡。 */
	UPROPERTY(BlueprintReadOnly, config, Category="主菜单|兼容", DisplayName="旧菜单入口关卡")
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
