// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "LxAIPointActor.generated.h"

class USceneComponent;
class USphereComponent;

/** 可放置在场景中的单个 AI 点位，Actor 位置就是点位中心。 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="AI点位"))
class LXARPG_API ALxAIPointActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建 AI 点位及编辑器范围预览。 */
	ALxAIPointActor();

	/** 刷新以米配置、以厘米显示且不受 Actor 缩放影响的范围预览。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 注册到当前游戏实例持有的 AI 导航注册表。 */
	virtual void BeginPlay() override;

	/** 从 AI 导航注册表注销当前对象。 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 获取点位 ID。 */
	FGameplayTag GetPointId() const { return PointId; }

	/** 获取世界空间点位中心。 */
	UFUNCTION(BlueprintPure, Category="AI导航|点位", DisplayName="获取世界空间点位中心")
	FVector GetWorldCenter() const;

	/** 获取范围半径，单位为厘米。 */
	UFUNCTION(BlueprintPure, Category="AI导航|点位", DisplayName="获取范围半径（厘米）")
	float GetRangeRadiusCentimeters() const;

	/** 按世界空间球体距离判断位置是否位于范围内，边界视为范围内。 */
	UFUNCTION(BlueprintPure, Category="AI导航|点位", DisplayName="判断世界位置是否在范围内")
	bool IsWorldLocationInRange(FVector WorldLocation) const;

	/** 点位的精确 GameplayTag 标识。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI导航|点位", meta=(DisplayName="点位ID", Categories="AI.点位"))
	FGameplayTag PointId;

	/** 球形范围半径，单位为米；语义不受 Actor 缩放影响。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI导航|点位", meta=(DisplayName="范围半径（米）", ClampMin="0.0", UIMin="0.0", Units="m"))
	float RangeRadiusMeters = 50.0f;

	/** 编辑器中的球形范围颜色。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AI导航|点位", meta=(DisplayName="范围颜色"))
	FColor RangeColor = FColor(255, 180, 40);

private:
	/** 点位 Actor 的场景根组件。 */
	UPROPERTY(VisibleAnywhere, Category="AI导航|点位", meta=(DisplayName="场景根组件"))
	TObjectPtr<USceneComponent> SceneRoot;

	/** 只用于编辑器观察范围的球体，不参与碰撞与范围判定。 */
	UPROPERTY(VisibleAnywhere, Category="AI导航|点位", meta=(DisplayName="范围预览"))
	TObjectPtr<USphereComponent> RangeVisualization;
};
