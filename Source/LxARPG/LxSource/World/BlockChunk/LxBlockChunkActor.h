// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxBlockChunkTypes.h"
#include "LxBlockChunkActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * 按 16×16×16 网格保存并显示方块地形的区块演员。
 * 每个方块边长固定为一米，渲染时只为与空气相邻的表面生成几何体。
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=(LxARPG), meta=(DisplayName="方块地形区块"))
class LXARPG_API ALxBlockChunkActor : public AActor
{
	GENERATED_BODY()

public:
	/** 创建方块地形区块并初始化程序化网格体。 */
	ALxBlockChunkActor();

	/** 当区块生成参数在编辑器中变化时重新生成方块地形。 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 根据当前噪声参数重新填充 16×16×16 方块数据并构建网格。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="方块区块|生成", meta=(DisplayName="重新生成方块区块"))
	void RebuildBlockChunk();

	/** 清空当前区块的全部方块数据、渲染网格和碰撞。 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="方块区块|生成", meta=(DisplayName="清空方块区块"))
	void ClearBlockChunk();

	/** 返回指定区块本地网格坐标处的方块类型，越界位置视为空气。 */
	UFUNCTION(BlueprintPure, Category="方块区块|方块", meta=(DisplayName="获取方块"))
	ELxBlockType GetBlock(const FIntVector& BlockCoordinate) const;

	/** 修改指定区块本地网格坐标处的方块类型，并立即重建区块网格。 */
	UFUNCTION(BlueprintCallable, Category="方块区块|方块", meta=(DisplayName="设置方块"))
	bool SetBlock(const FIntVector& BlockCoordinate, ELxBlockType BlockType);

	/** 保存区块合并后的渲染网格和碰撞。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="方块区块|网格体", meta=(DisplayName="区块程序化网格体"))
	TObjectPtr<UProceduralMeshComponent> ChunkMesh;

	/** 区块在无限方块网格中的二维坐标，用于让相邻区块连续采样地形噪声。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="区块坐标"))
	FIntPoint ChunkCoordinate = FIntPoint::ZeroValue;

	/** 生成地形时使用的确定性随机种子。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="地形种子"))
	int32 TerrainSeed = 1337;

	/** 地形每一列默认填充的方块数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="基础地形高度", ClampMin="1", ClampMax="16", UIMin="1", UIMax="16"))
	int32 BaseTerrainHeight = 8;

	/** 二维噪声在基础高度上下可以改变的最大方块数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="地形高度变化", ClampMin="0", ClampMax="15", UIMin="0", UIMax="8"))
	int32 TerrainHeightVariation = 5;

	/** 二维噪声的采样频率；数值越小，地形起伏范围越宽。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="地形噪声频率", ClampMin="0.001", ClampMax="1.0", UIMin="0.01", UIMax="0.25"))
	float TerrainNoiseFrequency = 0.08f;

	/** 草地表层下方使用泥土方块填充的厚度。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|生成", meta=(DisplayName="泥土层厚度", ClampMin="0", ClampMax="15", UIMin="0", UIMax="8"))
	int32 DirtLayerThickness = 3;

	/** 为全部可见方块表面提供颜色显示的材质。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|材质", meta=(DisplayName="方块区块材质"))
	TObjectPtr<UMaterialInterface> ChunkMaterial;

	/** 草地方块在顶面和侧面使用的基础顶点颜色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|材质", meta=(DisplayName="草地方块颜色"))
	FLinearColor GrassColor = FLinearColor(0.18f, 0.62f, 0.12f, 1.0f);

	/** 泥土方块使用的基础顶点颜色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|材质", meta=(DisplayName="泥土方块颜色"))
	FLinearColor DirtColor = FLinearColor(0.38f, 0.18f, 0.07f, 1.0f);

	/** 岩石方块使用的基础顶点颜色。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|材质", meta=(DisplayName="岩石方块颜色"))
	FLinearColor StoneColor = FLinearColor(0.38f, 0.40f, 0.43f, 1.0f);

	/** 是否为合并后的区块网格生成可供角色行走的复杂碰撞。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方块区块|碰撞", meta=(DisplayName="启用区块碰撞"))
	bool bEnableCollision = true;

private:
	/** 区块在 X、Y、Z 三个方向上包含的固定方块数量。 */
	static constexpr int32 ChunkLength = 16;

	/** 单个方块的固定边长，虚幻单位为厘米。 */
	static constexpr float BlockSize = 100.0f;

	/** 将三维区块本地坐标转换为一维方块数组索引。 */
	static int32 GetBlockIndex(int32 X, int32 Y, int32 Z);

	/** 判断指定坐标是否处于当前 16×16×16 区块内部。 */
	static bool IsBlockCoordinateValid(int32 X, int32 Y, int32 Z);

	/** 使用连续二维噪声填充草地、泥土和岩石方块数据。 */
	void GenerateTerrainBlocks();

	/** 根据当前方块数组为所有暴露表面构建合并网格和碰撞。 */
	void BuildChunkMesh();

	/** 返回指定方块类型经过表面方向明暗修正后的顶点颜色。 */
	FLinearColor GetBlockFaceColor(ELxBlockType BlockType, const FVector& FaceNormal) const;

	/** 按 X 优先、Y 次之、Z 最后排列保存区块中的 4096 个方块。 */
	UPROPERTY(Transient)
	TArray<ELxBlockType> Blocks;
};
