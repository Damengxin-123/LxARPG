// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LxARPGGameMode.generated.h"

/**
 * ARPG 游戏模式
 * 负责管理玩家生成、角色创建、重生等核心逻辑
 */
UCLASS()
class LXARPG_API ALxARPGGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:

	ALxARPGGameMode();

	/** 登录事件触发前确定主菜单阶段，阻止预览期间初始化正式玩家和写入存档。 */
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	/** 查询当前世界是否仍在只读主菜单阶段。 */
	bool IsShowingMainMenu() const { return bShowingMainMenu; }

	/** 在尚未开始玩法的当前世界启动选中会话，区域就绪后再分发游戏开始事件。 */
	bool BeginMenuSession();

	/** 菜单发起的会话先等待角色位置周边加载，再启动关卡玩法。 */
	virtual void StartPlay() override;

	/** 推进正式会话的区域加载和角色恢复。 */
	virtual void Tick(float DeltaSeconds) override;
	
	/** 游戏开始 */
	virtual void BeginPlay() override;

	/** 玩家登录时调用 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 玩家重新生成 */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** 玩家角色死亡处理 */
	UFUNCTION(BlueprintCallable, Category="ARPG|GameMode")
	virtual void HandlePlayerDeath(AController* DeadPlayer);

	/** 创建玩家角色 */
	UFUNCTION(BlueprintCallable, Category="ARPG|GameMode")
	virtual APawn* SpawnPlayerCharacter(AController* NewPlayer);

protected:
	/** 当前世界停留在只读主菜单，尚未分发正式玩法的开始事件。 */
	bool bShowingMainMenu = false;
	/** 当前正式关卡正在等待角色所在区域加载。 */
	bool bWaitingForMenuSession = false;
	/** 区域就绪后才允许蓝图或登录事件创建正式玩家。 */
	bool bSessionSpawnAllowed = false;

	/** 默认玩家Pawn类型 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ARPG|Character")
	TSubclassOf<APawn> DefaultPlayerPawnClass;

	/** 玩家重生延迟时间 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ARPG|Respawn")
	float RespawnDelay = 3.0f;

};
