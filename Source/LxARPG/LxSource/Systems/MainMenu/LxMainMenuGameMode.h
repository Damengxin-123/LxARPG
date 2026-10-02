#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LxMainMenuGameMode.generated.h"

/** 只读菜单世界：不分发正式 BeginPlay，因此关卡玩法不会在后台启动。 */
UCLASS(Blueprintable, DisplayName="主菜单游戏模式")
class LXARPG_API ALxMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	/** 菜单不自动生成受控玩家。 */
	ALxMainMenuGameMode();
	/** 在关卡对象初始化前开启存档只读保护。 */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	/** 预览世界刻意不调用父类，避免广播关卡正式开始事件。 */
	virtual void StartPlay() override;
	/** 在不启动关卡玩法的前提下推进菜单区域加载。 */
	virtual void Tick(float DeltaSeconds) override;
	/** 屏蔽默认出生流程。 */
	virtual void RestartPlayer(AController* NewPlayer) override;
};
