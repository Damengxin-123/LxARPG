// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxProceduralCropActor.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class USplineComponent;
class UStaticMesh;

/**
 * 程序化农作物演员。
 * 在闭合样条范围内按本地 X 轴排列分组列、按本地 Y 轴排列均匀行，旋转演员即可改变整体种植朝向。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="程序化农作物"))
class LXARPG_API ALxProceduralCropActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建程序化农作物演员并初始化默认矩形生成范围。 */
	ALxProceduralCropActor();

	/** 当演员、样条线或生成参数发生变化时重新生成农作物。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 立即根据当前闭合样条范围和排列参数重新生成农作物。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="程序化农作物|生成", meta=(DisplayName="重新生成农作物"))
	void RebuildCrops();

	/** 清空当前已经生成的全部农作物实例。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="程序化农作物|生成", meta=(DisplayName="清空生成的农作物"))
	void ClearGeneratedCrops();

	/** 用于在关卡视口中编辑农作物生成范围的闭合样条线。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="程序化农作物|范围", meta=(DisplayName="农作物范围曲线"))
	TObjectPtr<USplineComponent> CropBoundarySpline;

	/** 保存全部农作物实例的分层实例化静态网格体组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="程序化农作物|生成", meta=(DisplayName="生成的农作物实例"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> CropInstances;

	/** 当前演员生成时使用的唯一一种农作物静态网格体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|外观", meta=(DisplayName="农作物网格体"))
	TObjectPtr<UStaticMesh> CropMesh;

	/** 闭合范围曲线的控制点数量；修改后会沿当前轮廓均匀重新采样。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|范围", meta=(DisplayName="曲线控制点数量", ClampMin="3", UIMin="3"))
	int32 ControlPointCount = 4;

	/** 曲线边缘用于范围判断的最大采样间距；越小越贴合弯曲边缘。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|范围", meta=(DisplayName="曲线最大采样间距", ClampMin="5.0", UIMin="10.0", Units="cm"))
	float MaxBoundarySampleLength = 50.0f;

	/** 每一组中连续排列的农作物列数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|列", meta=(DisplayName="每组列数", ClampMin="1", UIMin="1"))
	int32 ColumnsPerGroup = 2;

	/** 同一组内相邻两列农作物中心之间的距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|列", meta=(DisplayName="组内列间隔", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float IntraGroupColumnSpacing = 50.0f;

	/** 相邻两组最靠近的两列农作物中心之间的距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|列", meta=(DisplayName="组间隔", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float GroupSpacing = 150.0f;

	/** 沿演员本地 Y 轴生成的农作物行数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|行", meta=(DisplayName="农作物行数", ClampMin="1", UIMin="1"))
	int32 RowCount = 5;

	/** 相邻两行农作物中心之间的均匀距离。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|行", meta=(DisplayName="行间隔", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float RowSpacing = 100.0f;

	/** 全部农作物实例相对演员原点使用的统一本地高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="程序化农作物|高度", meta=(DisplayName="统一高度", Units="cm"))
	float CropHeight = 0.0f;

private:
	/** 让样条控制点数量与参数保持同步，并在数量变化时沿原轮廓重新采样。 */
	void SynchronizeSplineControlPointCount();

	/** 将全部样条控制点约束到演员本地 Z=0，使生成范围保持为平面。 */
	void ConstrainSplinePointsToActorHeight();

	/** 沿闭合样条线采样出用于二维范围判断的多边形轮廓。 */
	void BuildBoundaryPolygon(TArray<FVector2D>& OutBoundaryPoints) const;

	/** 判断指定本地二维位置是否位于闭合范围内部或边缘。 */
	bool IsPointInsideBoundary(const FVector2D& Point, const TArray<FVector2D>& BoundaryPoints) const;

	/** 记录最近一次由参数应用的控制点数量，用于兼容手动增删样条点。 */
	UPROPERTY(Transient)
	int32 LastAppliedControlPointCount = 4;
};
