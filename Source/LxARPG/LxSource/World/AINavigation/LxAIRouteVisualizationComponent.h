// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "LxAIRouteVisualizationComponent.generated.h"

/** 只读绘制 AI 路线折线的编辑器可视组件，不提供视口编辑命中代理。 */
UCLASS(ClassGroup=(LxARPG), NotBlueprintable, meta=(DisplayName="AI路线预览"))
class LXARPG_API ULxAIRouteVisualizationComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	/** 创建只在编辑器显示且不参与碰撞的路线预览组件。 */
	ULxAIRouteVisualizationComponent();

	/** 用局部空间路线点、闭合状态与颜色刷新预览。 */
	void SetRoutePreview(const TArray<FVector>& InLocalPoints, bool bInClosedLoop, const FLinearColor& InColor);

	/** 获取当前派生预览点数量。 */
	int32 GetPreviewPointCount() const { return PreviewPoints.Num(); }

	/** 按索引读取一个局部空间派生预览点。 */
	bool GetPreviewLocalPoint(int32 PointIndex, FVector& OutLocalPoint) const;

	/** 创建不含任何编辑命中代理的路线线框场景代理。 */
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	/** 计算覆盖全部预览点的组件包围范围。 */
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
	/** 从路线 Actor 的唯一数据源复制出的局部空间预览点。 */
	TArray<FVector> PreviewPoints;

	/** 是否绘制末点到首点的闭合线段。 */
	bool bPreviewClosedLoop = false;

	/** 预览线条颜色。 */
	FLinearColor PreviewColor = FLinearColor::White;
};
