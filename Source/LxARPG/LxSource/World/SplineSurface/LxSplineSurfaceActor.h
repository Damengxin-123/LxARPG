// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxSplineSurfaceActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class USplineComponent;

/**
 * 闭合样条平面演员。
 * 根据闭合样条线生成可自由调整轮廓的平整网格体，适合地面、路面与区域填充。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="闭合样条平面"))
class LXARPG_API ALxSplineSurfaceActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建闭合样条平面演员并初始化默认矩形轮廓。 */
	ALxSplineSurfaceActor();

	/** 当演员或样条线发生变化时重新生成平面。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 立即根据当前闭合样条线重新生成平面。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="闭合样条平面|生成", meta=(DisplayName="重新生成平面"))
	void RebuildSurface();

	/** 清空当前生成的平面网格体。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="闭合样条平面|生成", meta=(DisplayName="清空生成的平面"))
	void ClearGeneratedSurface();

	/** 用于在关卡视口中编辑平面轮廓的闭合样条线。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="闭合样条平面|样条线", meta=(DisplayName="平面轮廓样条线"))
	TObjectPtr<USplineComponent> SurfaceSpline;

	/** 保存实时生成三角形平面的程序化网格体组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="闭合样条平面|网格体", meta=(DisplayName="生成的平面网格体"))
	TObjectPtr<UProceduralMeshComponent> SurfaceMesh;

	/** 应用到生成平面的材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|材质", meta=(DisplayName="平面材质"))
	TObjectPtr<UMaterialInterface> SurfaceMaterial;

	/** 每米范围内纹理重复的次数，数值越大纹理越密。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|材质", meta=(DisplayName="材质密度", ClampMin="0.001", UIMin="0.1"))
	float MaterialDensity = 1.0f;

	/** 纹理 UV 在本地平面上的旋转角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|材质", meta=(DisplayName="纹理旋转", Units="deg"))
	float TextureRotation = 0.0f;

	/** 纹理 UV 的额外平移偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|材质", meta=(DisplayName="纹理偏移"))
	FVector2D TextureOffset = FVector2D::ZeroVector;

	/** 相对样条线高度的平面垂直偏移，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="平面高度偏移", Units="cm"))
	float HeightOffset = 0.0f;

	/** 生成平面相对样条线的附加位置，便于通过数值精确调整平面位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面位置", Units="cm"))
	FVector GeneratedSurfaceLocation = FVector::ZeroVector;

	/** 生成平面相对样条线的附加旋转，便于通过数值精确调整平面朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面旋转", Units="deg"))
	FRotator GeneratedSurfaceRotation = FRotator::ZeroRotator;

	/** 生成平面相对样条线的附加缩放，便于通过数值精确调整平面尺寸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面缩放"))
	FVector GeneratedSurfaceScale = FVector::OneVector;

	/** 用于围成单个平面的样条控制点数量；修改后会沿当前轮廓重新均匀采样并保留整体形状。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="控制点数量", ClampMin="3", UIMin="3"))
	int32 ControlPointCount = 4;

	/** 曲线边缘的最大采样间距；数值越小，弧形轮廓越圆润。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="最大边缘采样间距", ClampMin="5.0", UIMin="10.0", Units="cm"))
	float MaxEdgeSampleLength = 100.0f;

	/** 是否为生成的平面启用碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="启用碰撞"))
	bool bEnableCollision = true;

	/** 从可见平面向下挤出的碰撞厚度，用于稳定角色地面检测。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="碰撞厚度", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float CollisionThickness = 10.0f;

	/** 生成平面使用的碰撞预设名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="碰撞预设"))
	FName CollisionProfileName = TEXT("BlockAll");

	/** 是否让生成的平面投射阴影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|渲染", meta=(DisplayName="投射阴影"))
	bool bCastShadow = true;

private:
	/** 让样条控制点数量与参数保持同步，并在增减控制点时沿原轮廓重新采样。 */
	void SynchronizeSplineControlPointCount();

	/** 将全部样条控制点约束到演员本地 Z=0，从而始终跟随演员高度。 */
	void ConstrainSplinePointsToActorHeight();

	/** 沿闭合样条线采样并生成平面轮廓顶点。 */
	void BuildOutlineVertices(TArray<FVector>& OutVertices) const;

	/** 移除轮廓中重复或共线的点，避免产生退化三角形。 */
	void SanitizeOutlineVertices(TArray<FVector>& InOutVertices) const;

	/** 使用耳切法将简单的凹多边形轮廓三角化。 */
	bool TriangulatePolygon(TArray<FVector>& InOutVertices, TArray<int32>& OutTriangles) const;

	/** 按照可见平面的三角形向下生成具有厚度的简单凸碰撞。 */
	void BuildStableCollision(const TArray<FVector>& Vertices, const TArray<int32>& Triangles);

	/** 判断二维点是否位于指定的逆时针三角形内部或边缘。 */
	bool IsPointInsideTriangle2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C) const;

	/** 记录最近一次由参数应用的控制点数量，用于兼容已有实例和手动编辑样条点。 */
	UPROPERTY(Transient)
	int32 LastAppliedControlPointCount = 4;
};
