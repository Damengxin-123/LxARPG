// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "LxAIRouteActor.generated.h"

class USplineComponent;

/** 可放置在场景中的 AI 路线，直接使用样条点配置路线。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="AI路线"))
class LXARPG_API ALxAIRouteActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建带有两个默认点的可编辑路线样条。 */
	ALxAIRouteActor();

	/** 注册到当前游戏实例持有的 AI 导航注册表。 */
	virtual void BeginPlay() override;

	/** 从 AI 导航注册表注销当前对象。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取路线 ID。 */
	FGameplayTag GetRouteId() const { return RouteId; }

	/** 返回由 Actor 变换后的全部世界空间路线点。 */
	UFUNCTION(BlueprintPure, Category="AI导航|路线", DisplayName="获取世界空间路线点")
	TArray<FVector> GetWorldRoutePoints() const;

	/** 按索引读取一个世界空间路线点。 */
	UFUNCTION(BlueprintPure, Category="AI导航|路线", DisplayName="获取世界空间路线点（索引）")
	bool GetWorldRoutePoint(int32 PointIndex, FVector& OutWorldPoint) const;

	/** 在路线末端添加一个可拖动的样条点，延续末段方向。 */
	UFUNCTION(CallInEditor, BlueprintCallable, Category="AI导航|路线", DisplayName="添加路径点")
	void AddRoutePoint();

	/** 路线的精确 GameplayTag 标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI导航|路线", meta=(DisplayName="路线ID", Categories="AI.路线"))
	FGameplayTag RouteId;

	/** 可在场景视口拖动控制点的路线样条，编辑时显示路线。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI导航|路线", meta=(DisplayName="路线样条"))
	TObjectPtr<USplineComponent> RouteSpline;

};
