// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxSplineSurfaceActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class ULxGeneratedMeshRenderComponent;
class USplineComponent;

/**
 * 闭合样条平面演员。
 * 根据闭合样条线生成可自由调整轮廓的平整网格体，适合地面、路面与区域填充。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="闭合样条平面", Category="海港城市"))
class LXARPG_API ALxSplineSurfaceActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建闭合样条平面演员并初始化默认矩形轮廓。 */
	ALxSplineSurfaceActor();

	/** 当演员或样条线发生变化时重新生成平面。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 记录米制参数版本，旧关卡保留原厘米数据用于一次性迁移。 */
	virtual void Serialize(FArchive& Ar) override;
	/** 加载旧蓝图及关卡时自动换算尺寸，不改变原有地面形状。 */
	virtual void PostLoad() override;
	/** 运行前提交尚未生成的完整轮廓与碰撞。 */
	virtual void BeginPlay() override;
	/** 保存及烹饪前完成最后一次编辑。 */
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#if WITH_EDITOR
	/** 数值参数交互保留组件注册，避免地面在拖动滑块时消失。 */
	virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
	/** 合并滑块输入，避免反复注销组件及同步烹饪碰撞。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
	/** 整体移动复用本地几何，轮廓编辑交由统一预览队列处理。 */
	virtual void PostEditMove(bool bFinished) override;
	/** 撤销后刷新预览，保存前再恢复完整结果。 */
	virtual void PostEditUndo() override;
#endif

	/** 退出简化预览，恢复精确轮廓、行走碰撞及最终渲染缓存。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="闭合样条平面|生成", meta=(DisplayName="完成地面生成"))
	void FinalizeSurface();

	/** 立即按当前编辑或完整状态生成平面；完成地面生成可显式提交完整结果。 */
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

 /** 保留原网格碰撞，使用可保存的 Nanite 几何承担显示与投影。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="生成优化", meta=(DisplayName="优化渲染网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> OptimizedMesh;

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

	/** 保留轮廓控制点高度，用于坡道连接片；内部按三角形线性插值，不自动贴地。默认关闭以兼容已有平面。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="保留控制点高度"))
	bool bPreserveSplineHeights = false;

	/** 旧版厘米高度，仅用于旧资产和蓝图引用兼容；编辑请使用米制高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|兼容", meta=(DisplayName="旧版高度偏移", DeprecatedProperty, DeprecationMessage="请使用平面高度偏移（米）"))
	float HeightOffset = 0.0f;

	/** 相对样条线的垂直偏移，单位为米，保留两位小数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="平面高度偏移", Units="m", ForceUnits="m", MaxFractionalDigits="2"))
	float HeightOffsetMeters = 0.0f;

	/** 旧版厘米位置，仅保留旧资产数据与蓝图引用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|兼容", meta=(DisplayName="旧版生成位置", DeprecatedProperty, DeprecationMessage="请使用生成平面位置（米）"))
	FVector GeneratedSurfaceLocation = FVector::ZeroVector;

	/** 生成平面相对样条线的附加位置，单位为米，保留两位小数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面位置", Units="m", ForceUnits="m", MaxFractionalDigits="2"))
	FVector GeneratedSurfaceLocationMeters = FVector::ZeroVector;

	/** 生成平面相对样条线的附加旋转，便于通过数值精确调整平面朝向。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面旋转", Units="deg"))
	FRotator GeneratedSurfaceRotation = FRotator::ZeroRotator;

	/** 生成平面相对样条线的附加缩放，便于通过数值精确调整平面尺寸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|变换", meta=(DisplayName="生成平面缩放"))
	FVector GeneratedSurfaceScale = FVector::OneVector;

	/** 用于围成单个平面的样条控制点数量；修改后会沿当前轮廓重新均匀采样并保留整体形状。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="控制点数量", ClampMin="3", UIMin="3"))
	int32 ControlPointCount = 4;

	/** 旧版厘米采样间距，仅用于旧资产兼容。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|兼容", meta=(DisplayName="旧版边缘采样间距", DeprecatedProperty, DeprecationMessage="请使用最大边缘采样间距（米）"))
	float MaxEdgeSampleLength = 100.0f;

	/** 完整曲线轮廓的最大采样间距，单位为米；直线段直接保留控制点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|形状", meta=(DisplayName="最大边缘采样间距", ClampMin="0.05", UIMin="0.10", Units="m", ForceUnits="m", MaxFractionalDigits="2"))
	float EdgeSampleSpacingMeters = 1.0f;

	/** 编辑时采用更疏的曲线采样，保存和运行时恢复完整精度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|预览", meta=(DisplayName="预览采样间距", ClampMin="0.05", Units="m", ForceUnits="m", MaxFractionalDigits="2"))
	float PreviewSampleSpacingMeters = 4.0f;

	/** 是否为生成的平面启用碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="启用碰撞"))
	bool bEnableCollision = true;

	/** 旧版厘米碰撞厚度，仅用于旧资产兼容。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|兼容", meta=(DisplayName="旧版碰撞厚度", DeprecatedProperty, DeprecationMessage="请使用碰撞厚度（米）"))
	float CollisionThickness = 10.0f;

	/** 从可见平面向下挤出的行走碰撞厚度，单位为米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="碰撞厚度", ClampMin="0.01", UIMin="0.01", Units="m", ForceUnits="m", MaxFractionalDigits="2"))
	float CollisionThicknessMeters = 0.10f;

	/** 生成平面使用的碰撞预设名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|碰撞", meta=(DisplayName="碰撞预设"))
	FName CollisionProfileName = TEXT("BlockAll");

	/** 是否让生成的平面投射阴影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="闭合样条平面|渲染", meta=(DisplayName="投射阴影"))
	bool bCastShadow = true;

	/** 无效轮廓及采样预算的中文提示。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="闭合样条平面|诊断", meta=(DisplayName="生成说明"))
	FString GenerationDiagnostic;
	/** 当前地面是否为不带行走碰撞的编辑预览。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="闭合样条平面|诊断", meta=(DisplayName="简化预览"))
	bool bSurfacePreview = false;
	/** 当前轮廓实际顶点数量，完整生成最多2048点，编辑预览最多256点。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="闭合样条平面|诊断", meta=(DisplayName="轮廓顶点数量"))
	int32 GeneratedVertexCount = 0;
	/** 当前实际三角形数量。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="闭合样条平面|诊断", meta=(DisplayName="三角形数量"))
	int32 GeneratedTriangleCount = 0;
	/** 本次会话实际生成次数，整体移动和未修改保存不增加。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="闭合样条平面|诊断", meta=(DisplayName="实际重建次数"))
	int32 GeometryBuildCount = 0;

private:
	/** 让样条控制点数量与参数保持同步，并在增减控制点时沿原轮廓重新采样。 */
	void SynchronizeSplineControlPointCount();

	/** 将全部样条控制点约束到演员本地 Z=0，从而始终跟随演员高度。 */
	void ConstrainSplinePointsToActorHeight();

	/** 沿闭合样条线采样并生成平面轮廓顶点。 */
	bool BuildOutlineVertices(TArray<FVector>& OutVertices);
	/** 拒绝自交或折返轮廓，避免生成跨越空区域的错误地面及碰撞。 */
	bool IsOutlineSimple(const TArray<FVector>& Vertices) const;

	/** 移除轮廓中重复或共线的点，避免产生退化三角形。 */
	void SanitizeOutlineVertices(TArray<FVector>& InOutVertices) const;

	/** 使用耳切法将简单的凹多边形轮廓三角化。 */
	bool TriangulatePolygon(TArray<FVector>& InOutVertices, TArray<int32>& OutTriangles) const;

	/** 按照可见平面的三角形向下生成具有厚度的简单凸碰撞。 */
	void BuildStableCollision(const TArray<FVector>& Vertices, const TArray<int32>& Triangles);

	/** 判断二维点是否位于指定的逆时针三角形内部或边缘。 */
	bool IsPointInsideTriangle2D(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C) const;

	/** 记录最近一次由参数应用的控制点数量，用于兼容已有实例和手动编辑样条点。 */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已应用控制点数量"))
	int32 LastAppliedControlPointCount = 4;

	/** 本地参数快照不包含演员世界变换，移动与旋转可复用同一几何。 */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="地面本地输入快照"))
	TArray<uint8> LocalSurfaceInputs;
	/** 是否需要在PostLoad中换算旧版厘米参数。 */
	bool bMigrateLegacyUnits = false;

	/** 兼容旧蓝图对厘米属性的读写；新详情面板仍以米制参数为准。 */
	void SynchronizeLegacyParameters();
	/** 最近同步的旧参数值，识别已有蓝图设置节点是否修改了旧字段。 */
	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="旧参数同步快照"))
	TArray<double> AppliedLegacyParameters = {0, 0, 0, 0, 100, 10};
};
