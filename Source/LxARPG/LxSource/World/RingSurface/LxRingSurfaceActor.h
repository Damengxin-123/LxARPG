// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxRingSurfaceActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * 可在编辑器中实时生成带厚度圆环平面的演员。
 * 顶面使用极坐标 UV，使普通平铺材质沿圆心形成同心圆带。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="圆环平面"))
class LXARPG_API ALxRingSurfaceActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建圆环平面演员并初始化程序化网格体。 */
	ALxRingSurfaceActor();

	/** 当演员参数发生变化时重新生成圆环平面。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 立即根据当前参数重新生成圆环平面。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="圆环平面|生成", meta=(DisplayName="重新生成圆环平面"))
	void RebuildRingSurface();

	/** 清空当前生成的圆环平面网格体。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="圆环平面|生成", meta=(DisplayName="清空圆环平面"))
	void ClearGeneratedRingSurface();

	/** 保存实时生成圆环平面的程序化网格体组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="圆环平面|网格体", meta=(DisplayName="圆环平面网格体"))
	TObjectPtr<UProceduralMeshComponent> RingSurfaceMesh;

	/** 圆环外缘的完整直径，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|形状", meta=(DisplayName="外直径", ClampMin="2.0", UIMin="100.0", Units="cm"))
	float OuterDiameter = 1000.0f;

	/** 圆环中心孔洞的完整直径，必须小于外直径，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|形状", meta=(DisplayName="内直径", ClampMin="1.0", UIMin="1.0", Units="cm"))
	float InnerDiameter = 500.0f;

	/** 从可见顶面向下延伸的实体厚度，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|形状", meta=(DisplayName="平面厚度", ClampMin="0.1", UIMin="1.0", Units="cm"))
	float SurfaceThickness = 10.0f;

	/** 圆周使用的分段数量，数值越高轮廓越圆滑。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|形状", meta=(DisplayName="圆周分段数", ClampMin="8", ClampMax="512", UIMin="16", UIMax="256"))
	int32 CircleSegmentCount = 96;

	/** 应用到圆环全部表面的材质；顶面会使用同心圆极坐标 UV。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|材质", meta=(DisplayName="圆环材质"))
	TObjectPtr<UMaterialInterface> RingSurfaceMaterial;

	/** 每米范围内纹理重复的次数，数值越大纹理越密。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|材质", meta=(DisplayName="材质密度", ClampMin="0.001", UIMin="0.1"))
	float MaterialDensity = 1.0f;

	/** 同心圆 UV 的额外平移；X 沿圆周，Y 沿半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|材质", meta=(DisplayName="纹理偏移"))
	FVector2D TextureOffset = FVector2D::ZeroVector;

	/** 沿圆周旋转同心圆纹理的起点角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|材质", meta=(DisplayName="纹理起始角度", Units="deg"))
	float TextureStartAngle = 0.0f;

	/** 是否为生成的圆环实体启用碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|碰撞", meta=(DisplayName="启用碰撞"))
	bool bEnableCollision = true;

	/** 生成圆环实体使用的碰撞预设名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|碰撞", meta=(DisplayName="碰撞预设"))
	FName CollisionProfileName = TEXT("BlockAll");

	/** 是否让生成的圆环平面投射阴影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环平面|渲染", meta=(DisplayName="投射阴影"))
	bool bCastShadow = true;

private:
	/** 为圆环的每个分段生成一个带厚度的凸碰撞棱柱。 */
	void BuildStableCollision(float OuterRadius, float InnerRadius, float Thickness, int32 SegmentCount);
};
