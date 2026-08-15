// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxSplineWallActor.generated.h"

class UMaterialInterface;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

/**
 * 样条曲线围墙演员。
 * 根据可编辑的样条线实时生成连续的曲线墙体，并支持尺寸、材质与纹理密度调整。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="样条曲线围墙"))
class LXARPG_API ALxSplineWallActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建样条曲线围墙演员并初始化默认样条线。 */
	ALxSplineWallActor();

	/** 当演员或样条线在编辑器中发生变化时重新生成墙体。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 立即根据当前样条线和配置重新生成墙体。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="样条围墙|生成", meta=(DisplayName="重新生成围墙"))
	void RebuildWall();

	/** 删除当前已经生成的全部墙体分段。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="样条围墙|生成", meta=(DisplayName="清空生成的围墙"))
	void ClearGeneratedWall();

	/** 用于在关卡视口中编辑围墙路径的样条线组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="样条围墙|样条线", meta=(DisplayName="围墙样条线"))
	TObjectPtr<USplineComponent> WallSpline;

	/** 沿样条线弯曲的墙体静态网格体，模型长度方向应为本地 X 轴。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|外观", meta=(DisplayName="墙体网格体"))
	TObjectPtr<UStaticMesh> WallMesh;

	/** 覆盖墙体网格体第 0 个材质槽的材质；为空时使用网格体自带材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|外观", meta=(DisplayName="覆盖材质"))
	TObjectPtr<UMaterialInterface> WallMaterial;

	/** 最终墙体高度，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|尺寸", meta=(DisplayName="墙体高度", ClampMin="1.0", UIMin="10.0", Units="cm"))
	float WallHeight = 300.0f;

	/** 最终墙体厚度，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|尺寸", meta=(DisplayName="墙体厚度", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float WallThickness = 30.0f;

	/** 单个墙体分段允许的最大长度；数值越小，曲线越圆润但组件数量越多。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|尺寸", meta=(DisplayName="最大分段长度", ClampMin="10.0", UIMin="25.0", Units="cm"))
	float MaxSegmentLength = 100.0f;

	/** 是否将网格体包围盒底面自动对齐到样条线高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|尺寸", meta=(DisplayName="底部对齐样条线"))
	bool bAlignMeshBottomToSpline = true;

	/** 墙体相对样条线的额外垂直偏移，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|尺寸", meta=(DisplayName="垂直偏移", Units="cm"))
	float VerticalOffset = 0.0f;

	/** 材质纹理密度；需要材质中存在同名标量参数，数值越大纹理重复越密。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|材质", meta=(DisplayName="材质密度", ClampMin="0.01", UIMin="0.1"))
	float MaterialDensity = 1.0f;

	/** 接收材质密度数值的材质标量参数名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|材质", meta=(DisplayName="材质密度参数名"))
	FName MaterialDensityParameterName = TEXT("TextureDensity");

	/** 是否为生成的墙体启用碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|碰撞", meta=(DisplayName="启用碰撞"))
	bool bEnableCollision = true;

	/** 生成墙体使用的碰撞预设名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|碰撞", meta=(DisplayName="碰撞预设"))
	FName CollisionProfileName = TEXT("BlockAll");

	/** 是否让生成的墙体投射阴影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="样条围墙|渲染", meta=(DisplayName="投射阴影"))
	bool bCastShadow = true;

protected:
	/** 当前由样条线生成的墙体网格体组件集合。 */
	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<USplineMeshComponent>> GeneratedWallSegments;

private:
	/** 在指定的样条线距离范围内创建一个墙体分段。 */
	void CreateWallSegment(float StartDistance, float EndDistance, const FVector2D& WallScale, float MeshBottomOffset);

	/** 为墙体分段应用覆盖材质与材质密度参数。 */
	void ApplyWallMaterial(USplineMeshComponent* WallSegment) const;
};
