// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxAIRouteVisualizationComponent.h"

#include "Engine/CollisionProfile.h"
#include "MeshElementCollector.h"
#include "PrimitiveSceneProxy.h"
#include "PrimitiveViewRelevance.h"
#include "SceneManagement.h"
#include "SceneView.h"

/** AI 路线只读线框的渲染线程代理。 */
class FLxAIRouteVisualizationSceneProxy final : public FPrimitiveSceneProxy
{
public:
	/** 从组件快照构造不会反向修改组件数据的渲染代理。 */
	explicit FLxAIRouteVisualizationSceneProxy(const ULxAIRouteVisualizationComponent* InComponent,
		TArray<FVector> InPreviewPoints, const bool bInClosedLoop, const FLinearColor& InColor)
		: FPrimitiveSceneProxy(InComponent)
		, PreviewPoints(MoveTemp(InPreviewPoints))
		, bClosedLoop(bInClosedLoop)
		, Color(InColor.ToFColor(true))
	{
		bWillEverBeLit = false;
	}

	/** 返回该场景代理类型的稳定唯一标识。 */
	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	/** 在所有可见编辑器视图中绘制路线线段，不创建 HitProxy。 */
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		if (PreviewPoints.Num() < 2)
		{
			return;
		}

		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			if ((VisibilityMap & (1u << ViewIndex)) == 0)
			{
				continue;
			}
			FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
			const uint8 DepthPriorityGroup = GetDepthPriorityGroup(Views[ViewIndex]);
			for (int32 PointIndex = 1; PointIndex < PreviewPoints.Num(); ++PointIndex)
			{
				PDI->DrawLine(GetLocalToWorld().TransformPosition(PreviewPoints[PointIndex - 1]),
					GetLocalToWorld().TransformPosition(PreviewPoints[PointIndex]), Color, DepthPriorityGroup, 2.0f);
			}
			if (bClosedLoop && PreviewPoints.Num() > 2)
			{
				PDI->DrawLine(GetLocalToWorld().TransformPosition(PreviewPoints.Last()),
					GetLocalToWorld().TransformPosition(PreviewPoints[0]), Color, DepthPriorityGroup, 2.0f);
			}
		}
	}

	/** 声明预览仅使用动态编辑器绘制。 */
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bDynamicRelevance = true;
		Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
		return Result;
	}

	/** 返回场景代理及其动态数组占用的内存。 */
	virtual uint32 GetMemoryFootprint() const override
	{
		return sizeof(*this) + GetAllocatedSize();
	}

	/** 返回路线点快照的动态内存占用。 */
	uint32 GetAllocatedSize() const
	{
		return FPrimitiveSceneProxy::GetAllocatedSize() + PreviewPoints.GetAllocatedSize();
	}

private:
	/** 局部空间路线点快照。 */
	TArray<FVector> PreviewPoints;
	/** 是否绘制闭合线段。 */
	bool bClosedLoop;
	/** 绘制颜色。 */
	FColor Color;
};

ULxAIRouteVisualizationComponent::ULxAIRouteVisualizationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bUseEditorCompositing = true;
	SetHiddenInGame(true);
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
#if WITH_EDITORONLY_DATA
	SetIsVisualizationComponent(true);
	bEditableWhenInherited = false;
#endif
}

void ULxAIRouteVisualizationComponent::SetRoutePreview(const TArray<FVector>& InLocalPoints,
	const bool bInClosedLoop, const FLinearColor& InColor)
{
	PreviewPoints = InLocalPoints;
	bPreviewClosedLoop = bInClosedLoop;
	PreviewColor = InColor;
	UpdateBounds();
	MarkRenderStateDirty();
}

bool ULxAIRouteVisualizationComponent::GetPreviewLocalPoint(const int32 PointIndex, FVector& OutLocalPoint) const
{
	if (!PreviewPoints.IsValidIndex(PointIndex))
	{
		return false;
	}
	OutLocalPoint = PreviewPoints[PointIndex];
	return true;
}

FPrimitiveSceneProxy* ULxAIRouteVisualizationComponent::CreateSceneProxy()
{
	return new FLxAIRouteVisualizationSceneProxy(this, PreviewPoints, bPreviewClosedLoop, PreviewColor);
}

FBoxSphereBounds ULxAIRouteVisualizationComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox LocalBox(ForceInit);
	for (const FVector& PreviewPoint : PreviewPoints)
	{
		LocalBox += PreviewPoint;
	}
	if (!LocalBox.IsValid)
	{
		LocalBox = FBox(FVector(-10.0f), FVector(10.0f));
	}
	return FBoxSphereBounds(LocalBox).TransformBy(LocalToWorld);
}
