// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "LxAINavigationRegistry.generated.h"

class ALxAIPointActor;
class ALxAIRouteActor;

/**
 * AI 导航场景对象注册表。
 * 按世界和精确 GameplayTag 隔离缓存路线与点位，并仅持有弱引用。
 */
UCLASS(BlueprintType, DisplayName="AI导航注册表")
class LXARPG_API ULxAINavigationRegistry : public UObject
{
	GENERATED_BODY()

public:
	/** 注册一条 AI 路线；同一世界内重复的路线 ID 会被拒绝。 */
	bool RegisterRoute(ALxAIRouteActor* RouteActor);

	/** 仅当注册项仍指向指定对象时注销 AI 路线。 */
	void UnregisterRoute(const ALxAIRouteActor* RouteActor);

	/** 注册一个 AI 点位；同一世界内重复的点位 ID 会被拒绝。 */
	bool RegisterPoint(ALxAIPointActor* PointActor);

	/** 仅当注册项仍指向指定对象时注销 AI 点位。 */
	void UnregisterPoint(const ALxAIPointActor* PointActor);

	/** 按世界和精确 ID 查询 AI 路线。 */
	UFUNCTION(BlueprintPure, Category="AI导航|路线", DisplayName="按ID获取AI路线",
		meta=(WorldContext="WorldContextObject", Categories="AI.路线"))
	ALxAIRouteActor* FindRoute(const UObject* WorldContextObject, FGameplayTag RouteId);

	/** 按世界和精确 ID 查询 AI 点位。 */
	UFUNCTION(BlueprintPure, Category="AI导航|点位", DisplayName="按ID获取AI点位",
		meta=(WorldContext="WorldContextObject", Categories="AI.点位"))
	ALxAIPointActor* FindPoint(const UObject* WorldContextObject, FGameplayTag PointId);

	/** 清空全部世界的路线和点位弱引用。 */
	void Deinitialize();

private:
	/** 单条路线的世界隔离弱引用记录。 */
	struct FRouteEntry
	{
		/** 路线所属的运行世界。 */
		TWeakObjectPtr<UWorld> World;
		/** 路线的精确 GameplayTag 标识。 */
		FGameplayTag Id;
		/** 已注册路线 Actor 的弱引用。 */
		TWeakObjectPtr<ALxAIRouteActor> Actor;
	};

	/** 单个点位的世界隔离弱引用记录。 */
	struct FPointEntry
	{
		/** 点位所属的运行世界。 */
		TWeakObjectPtr<UWorld> World;
		/** 点位的精确 GameplayTag 标识。 */
		FGameplayTag Id;
		/** 已注册点位 Actor 的弱引用。 */
		TWeakObjectPtr<ALxAIPointActor> Actor;
	};

	/** 注册时移除失效记录；查询直接通过弱引用过滤失效对象，避免每次扫描两类缓存。 */
	void RemoveStaleEntries();

	TArray<FRouteEntry> RouteEntries;
	TArray<FPointEntry> PointEntries;
};
