// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxRingCylinderActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * 可快速生成实心或中空圆环柱体的演员。
 * 外圈直径和高度由演员缩放控制，中空半径、材质与材质密度可在细节面板中调整。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="圆环柱体"))
class LXARPG_API ALxRingCylinderActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建圆环柱体演员并初始化程序化网格体。 */
	ALxRingCylinderActor();

	/** 当演员参数或缩放发生变化时重新生成圆环柱体。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 立即根据当前参数重新生成圆环柱体。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="圆环柱体|生成", meta=(DisplayName="重新生成圆环柱体"))
	void RebuildRingCylinder();

	/** 清空当前生成的圆环柱体网格。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="圆环柱体|生成", meta=(DisplayName="清空圆环柱体"))
	void ClearGeneratedRingCylinder();

	/** 保存实时生成圆环柱体的程序化网格体组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="圆环柱体|网格体", meta=(DisplayName="圆环柱体网格体"))
	TObjectPtr<UProceduralMeshComponent> RingCylinderMesh;

	/** 中间孔洞的本地半径；设为 0 时生成实心圆柱体，最大不会超过外圈半径。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环柱体|形状", meta=(DisplayName="中空半径", ClampMin="0.0", ClampMax="49.9", UIMin="0.0", UIMax="49.9", Units="cm"))
	float InnerRadius = 25.0f;

	/** 应用到圆环柱体全部表面的材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环柱体|材质", meta=(DisplayName="圆环柱体材质"))
	TObjectPtr<UMaterialInterface> RingCylinderMaterial;

	/** 每米范围内纹理重复的次数，数值越大纹理越密。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆环柱体|材质", meta=(DisplayName="材质密度", ClampMin="0.001", UIMin="0.1"))
	float MaterialDensity = 1.0f;

private:
	/** 圆周使用的固定分段数，在生成速度和轮廓圆滑度之间保持平衡。 */
	static constexpr int32 CircleSegmentCount = 64;

	/** 未缩放网格的固定外圈半径，外圈最终尺寸由演员 X/Y 缩放控制。 */
	static constexpr float BaseOuterRadius = 50.0f;

	/** 未缩放网格的固定高度，最终高度由演员 Z 缩放控制。 */
	static constexpr float BaseHeight = 100.0f;
};
