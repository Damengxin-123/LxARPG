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

	/** 默认玩家Pawn类型 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ARPG|Character")
	TSubclassOf<APawn> DefaultPlayerPawnClass;

	/** 玩家重生延迟时间 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ARPG|Respawn")
	float RespawnDelay = 3.0f;

};
