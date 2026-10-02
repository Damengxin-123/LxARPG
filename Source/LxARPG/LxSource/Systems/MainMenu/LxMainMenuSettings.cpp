#include "LxMainMenuSettings.h"

ULxMainMenuSettings::ULxMainMenuSettings()
{
	DefaultLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/项目内容/关卡/总关卡.总关卡")));
	MenuLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/项目内容/关卡/主菜单.主菜单")));
	DefaultCharacter = TSoftClassPtr<ALxPlayerCharacter>(FSoftObjectPath(TEXT("/Game/项目内容/实体资产/角色/测试角色-人类法师/测试角色-玩家控制角色.测试角色-玩家控制角色_C")));
	GameplayMode = TSoftClassPtr<AGameModeBase>(FSoftObjectPath(TEXT("/Game/项目内容/游戏模式/ARPG游戏模式.ARPG游戏模式_C")));
}
