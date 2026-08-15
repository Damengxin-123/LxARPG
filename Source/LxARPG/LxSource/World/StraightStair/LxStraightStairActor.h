// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxStraightStairActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
struct FPropertyChangedEvent;

/**
 * 可通过尺寸参数实时生成直线楼梯的演员。
 * 楼梯沿本地 X 轴正方向上升，宽度沿本地 Y 轴展开，侧面整体呈阶梯化直角三角形。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="直线楼梯"))
class LXARPG_API ALxStraightStairActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建直线楼梯演员并初始化程序化网格体。 */
	ALxStraightStairActor();

	/** 当楼梯尺寸、材质或演员变换发生变化时重新生成楼梯。 */
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	/** 在细节面板修改单阶或整体尺寸时同步另一组尺寸。 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** 立即根据当前配置重新生成直线楼梯。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="直线楼梯|生成", meta=(DisplayName="重新生成直线楼梯"))
	void RebuildStraightStair();

	/** 清空当前生成的直线楼梯网格体。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="直线楼梯|生成", meta=(DisplayName="清空直线楼梯"))
	void ClearGeneratedStraightStair();

	/** 保存实时生成直线楼梯的程序化网格体组件。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="直线楼梯|网格体", meta=(DisplayName="直线楼梯网格体"))
	TObjectPtr<UProceduralMeshComponent> StraightStairMesh;

	/** 楼梯包含的台阶数量；修改后保持整体尺寸并重新计算单阶尺寸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|台阶", meta=(DisplayName="台阶数量", ClampMin="1", ClampMax="512", UIMin="1", UIMax="100"))
	int32 StepCount = 10;

	/** 每阶沿楼梯前进方向占用的长度，也就是单阶踏步进深；修改后会同步整体长度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|台阶", meta=(DisplayName="单阶进深（宽度）", ClampMin="1.0", UIMin="10.0", Units="cm"))
	float StepLength = 30.0f;

	/** 相邻两阶之间的垂直高度；修改后会同步楼梯整体高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|台阶", meta=(DisplayName="单阶高度", ClampMin="1.0", UIMin="5.0", Units="cm"))
	float StepHeight = 20.0f;

	/** 楼梯在本地 Y 轴方向上的整体宽度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|整体尺寸", meta=(DisplayName="楼梯整体宽度", ClampMin="1.0", UIMin="50.0", Units="cm"))
	float StairWidth = 300.0f;

	/** 楼梯从底部到最高一级踏面的整体高度；修改后会同步单阶高度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|整体尺寸", meta=(DisplayName="楼梯整体高度", ClampMin="1.0", UIMin="50.0", Units="cm"))
	float StairHeight = 200.0f;

	/** 楼梯在本地 X 轴方向上的整体水平长度；修改后会同步单阶进深。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|整体尺寸", meta=(DisplayName="楼梯整体长度", ClampMin="1.0", UIMin="50.0", Units="cm"))
	float StairLength = 300.0f;

	/** 应用到楼梯全部表面的材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|材质", meta=(DisplayName="楼梯材质"))
	TObjectPtr<UMaterialInterface> StairMaterial;

	/** 每米范围内纹理重复的次数，数值越大纹理越密。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|材质", meta=(DisplayName="材质密度", ClampMin="0.001", UIMin="0.1"))
	float MaterialDensity = 1.0f;

	/** 所有楼梯表面纹理 UV 的统一偏转角度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|材质", meta=(DisplayName="材质偏转角度", Units="deg"))
	float MaterialRotation = 0.0f;

	/** 所有楼梯表面纹理 UV 的统一平移偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|材质", meta=(DisplayName="材质偏移"))
	FVector2D MaterialOffset = FVector2D::ZeroVector;

	/** 是否为生成的楼梯启用碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|碰撞", meta=(DisplayName="启用碰撞"))
	bool bEnableCollision = true;

	/** 生成楼梯使用的碰撞预设名称。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|碰撞", meta=(DisplayName="碰撞预设"))
	FName CollisionProfileName = TEXT("BlockAll");

	/** 是否让生成的楼梯投射阴影。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="直线楼梯|渲染", meta=(DisplayName="投射阴影"))
	bool bCastShadow = true;

private:
	/** 对楼梯尺寸进行安全限制，并以整体尺寸计算当前实际单阶尺寸。 */
	void SanitizeDimensions();

	/** 为每一级台阶生成独立的凸棱柱碰撞，避免凹形楼梯使用复杂碰撞时角色检测不稳定。 */
	void BuildStableCollision();
};
