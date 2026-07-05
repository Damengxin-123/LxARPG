#pragma once

#include "CoreMinimal.h"
#include "LxGameStateTypes.generated.h"

/** 本地玩家所处的游戏流程状态。 */
UENUM(BlueprintType)
enum class ELxGameState : uint8
{
	/** 游戏程序正在启动并完成必要的初始化。 */
	OpeningGame UMETA(DisplayName="正在打开游戏"),

	/** 本地玩家当前位于游戏主菜单。 */
	MainMenu UMETA(DisplayName="处于游戏主菜单"),

	/** 本地玩家当前处于单机或非联机游戏流程中。 */
	InGame UMETA(DisplayName="处于游戏中"),

	/** 本地玩家当前处于联机游戏流程中。 */
	InOnlineGame UMETA(DisplayName="处于联机游戏中"),

	/** 游戏程序正在执行退出流程。 */
	ExitingGame UMETA(DisplayName="退出游戏中")
};

/** 本地玩家游戏流程状态发生变化时触发。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLxGameStateChanged, ELxGameState, PreviousState,
	ELxGameState, NewState);
