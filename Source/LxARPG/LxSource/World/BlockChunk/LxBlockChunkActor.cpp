// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxBlockChunkActor.h"

#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace LxBlockChunkPrivate
{
	/** 方块六个轴向表面的外侧法线。 */
	const FVector FaceNormals[] =
	{
		FVector::ForwardVector,
		FVector::BackwardVector,
		FVector::RightVector,
		FVector::LeftVector,
		FVector::UpVector,
		FVector::DownVector
	};
}

ALxBlockChunkActor::ALxBlockChunkActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ChunkMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ChunkMesh"));
	SetRootComponent(ChunkMesh);
	ChunkMesh->SetMobility(EComponentMobility::Static);
	ChunkMesh->SetCastShadow(true);
	ChunkMesh->bCastDynamicShadow = true;
	ChunkMesh->bCastStaticShadow = true;
	ChunkMesh->bCastContactShadow = true;
	ChunkMesh->bUseAsyncCooking = false;
	ChunkMesh->bUseComplexAsSimpleCollision = true;
	ChunkMesh->CanCharacterStepUpOn = ECB_Yes;
	ChunkMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ChunkMesh->SetCollisionObjectType(ECC_WorldStatic);

	// 引擎顶点色材质无需额外创建资产，可以直接显示不同方块及不同朝向的颜色。
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> VertexColorMaterial(
		TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
	if (VertexColorMaterial.Succeeded())
	{
		ChunkMaterial = VertexColorMaterial.Object;
	}
}

void ALxBlockChunkActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildBlockChunk();
}

void ALxBlockChunkActor::RebuildBlockChunk()
{
	GenerateTerrainBlocks();
	BuildChunkMesh();
}

void ALxBlockChunkActor::ClearBlockChunk()
{
	Blocks.Init(ELxBlockType::Air, ChunkLength * ChunkLength * ChunkLength);
	if (IsValid(ChunkMesh))
	{
		ChunkMesh->ClearAllMeshSections();
		ChunkMesh->ClearCollisionConvexMeshes();
	}
}

ELxBlockType ALxBlockChunkActor::GetBlock(const FIntVector& BlockCoordinate) const
{
	if (!IsBlockCoordinateValid(BlockCoordinate.X, BlockCoordinate.Y, BlockCoordinate.Z)
		|| Blocks.Num() != ChunkLength * ChunkLength * ChunkLength)
	{
		return ELxBlockType::Air;
	}

	return Blocks[GetBlockIndex(BlockCoordinate.X, BlockCoordinate.Y, BlockCoordinate.Z)];
}

bool ALxBlockChunkActor::SetBlock(const FIntVector& BlockCoordinate, const ELxBlockType BlockType)
{
	if (!IsBlockCoordinateValid(BlockCoordinate.X, BlockCoordinate.Y, BlockCoordinate.Z))
	{
		return false;
	}

	if (Blocks.Num() != ChunkLength * ChunkLength * ChunkLength)
	{
		Blocks.Init(ELxBlockType::Air, ChunkLength * ChunkLength * ChunkLength);
	}

	Blocks[GetBlockIndex(BlockCoordinate.X, BlockCoordinate.Y, BlockCoordinate.Z)] = BlockType;
	BuildChunkMesh();
	return true;
}

int32 ALxBlockChunkActor::GetBlockIndex(const int32 X, const int32 Y, const int32 Z)
{
	return X + Y * ChunkLength + Z * ChunkLength * ChunkLength;
}

bool ALxBlockChunkActor::IsBlockCoordinateValid(const int32 X, const int32 Y, const int32 Z)
{
	return X >= 0 && X < ChunkLength
		&& Y >= 0 && Y < ChunkLength
		&& Z >= 0 && Z < ChunkLength;
}

void ALxBlockChunkActor::GenerateTerrainBlocks()
{
	Blocks.Init(ELxBlockType::Air, ChunkLength * ChunkLength * ChunkLength);

	const int32 SafeBaseHeight = FMath::Clamp(BaseTerrainHeight, 1, ChunkLength);
	const int32 SafeHeightVariation = FMath::Clamp(TerrainHeightVariation, 0, ChunkLength - 1);
	const int32 SafeDirtLayerThickness = FMath::Clamp(DirtLayerThickness, 0, ChunkLength - 1);
	const float SafeNoiseFrequency = FMath::Clamp(TerrainNoiseFrequency, 0.001f, 1.0f);
	FRandomStream SeedStream(TerrainSeed);
	const FVector2D SeedOffset(SeedStream.FRandRange(-10000.0f, 10000.0f), SeedStream.FRandRange(-10000.0f, 10000.0f));

	for (int32 Y = 0; Y < ChunkLength; ++Y)
	{
		for (int32 X = 0; X < ChunkLength; ++X)
		{
			const int32 WorldBlockX = ChunkCoordinate.X * ChunkLength + X;
			const int32 WorldBlockY = ChunkCoordinate.Y * ChunkLength + Y;
			const FVector2D NoisePosition = FVector2D(WorldBlockX, WorldBlockY) * SafeNoiseFrequency + SeedOffset;
			const float TerrainNoise = FMath::PerlinNoise2D(NoisePosition);
			const int32 ColumnHeight = FMath::Clamp(
				SafeBaseHeight + FMath::RoundToInt(TerrainNoise * static_cast<float>(SafeHeightVariation)),
				1,
				ChunkLength);

			for (int32 Z = 0; Z < ColumnHeight; ++Z)
			{
				ELxBlockType BlockType = ELxBlockType::Stone;
				if (Z == ColumnHeight - 1)
				{
					BlockType = ELxBlockType::Grass;
				}
				else if (Z >= ColumnHeight - 1 - SafeDirtLayerThickness)
				{
					BlockType = ELxBlockType::Dirt;
				}

				Blocks[GetBlockIndex(X, Y, Z)] = BlockType;
			}
		}
	}
}

void ALxBlockChunkActor::BuildChunkMesh()
{
	if (!IsValid(ChunkMesh) || Blocks.Num() != ChunkLength * ChunkLength * ChunkLength)
	{
		return;
	}

	ChunkMesh->ClearAllMeshSections();
	ChunkMesh->ClearCollisionConvexMeshes();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(ChunkLength * ChunkLength * 24);
	Triangles.Reserve(ChunkLength * ChunkLength * 36);

	const float HalfBlockSize = BlockSize * 0.5f;
	for (int32 Z = 0; Z < ChunkLength; ++Z)
	{
		for (int32 Y = 0; Y < ChunkLength; ++Y)
		{
			for (int32 X = 0; X < ChunkLength; ++X)
			{
				const ELxBlockType BlockType = Blocks[GetBlockIndex(X, Y, Z)];
				if (BlockType == ELxBlockType::Air)
				{
					continue;
				}

				const FVector BlockCenter(
					(static_cast<float>(X) + 0.5f) * BlockSize,
					(static_cast<float>(Y) + 0.5f) * BlockSize,
					(static_cast<float>(Z) + 0.5f) * BlockSize);

				for (const FVector& FaceNormal : LxBlockChunkPrivate::FaceNormals)
				{
					const FIntVector NeighborCoordinate(
						X + FMath::RoundToInt(FaceNormal.X),
						Y + FMath::RoundToInt(FaceNormal.Y),
						Z + FMath::RoundToInt(FaceNormal.Z));
					if (GetBlock(NeighborCoordinate) != ELxBlockType::Air)
					{
						continue;
					}

					// 参考轴和叉积共同构造稳定的面内横轴，并让六个面保持统一的正面绕序。
					const FVector FaceUp = FMath::Abs(FaceNormal.Z) > 0.5f ? FVector::RightVector : FVector::UpVector;
					const FVector FaceRight = FVector::CrossProduct(FaceNormal, FaceUp).GetSafeNormal();
					const FVector FaceCenter = BlockCenter + FaceNormal * HalfBlockSize;
					const int32 StartVertexIndex = Vertices.Num();
					Vertices.Append({
						FaceCenter - FaceRight * HalfBlockSize - FaceUp * HalfBlockSize,
						FaceCenter + FaceRight * HalfBlockSize - FaceUp * HalfBlockSize,
						FaceCenter - FaceRight * HalfBlockSize + FaceUp * HalfBlockSize,
						FaceCenter + FaceRight * HalfBlockSize + FaceUp * HalfBlockSize});
					Triangles.Append({
						StartVertexIndex, StartVertexIndex + 2, StartVertexIndex + 1,
						StartVertexIndex + 2, StartVertexIndex + 3, StartVertexIndex + 1});
					Normals.Append({FaceNormal, FaceNormal, FaceNormal, FaceNormal});
					UV0.Append({FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(0.0f, 1.0f), FVector2D(1.0f, 1.0f)});
					const FLinearColor FaceColor = GetBlockFaceColor(BlockType, FaceNormal);
					VertexColors.Append({FaceColor, FaceColor, FaceColor, FaceColor});
					const FProcMeshTangent FaceTangent(FaceRight, false);
					Tangents.Append({FaceTangent, FaceTangent, FaceTangent, FaceTangent});
				}
			}
		}
	}

	if (Vertices.IsEmpty())
	{
		return;
	}

	ChunkMesh->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	ChunkMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ChunkMesh->SetCollisionObjectType(ECC_WorldStatic);
	ChunkMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		bEnableCollision);
	ChunkMesh->SetMaterial(0, ChunkMaterial);
}

FLinearColor ALxBlockChunkActor::GetBlockFaceColor(const ELxBlockType BlockType, const FVector& FaceNormal) const
{
	FLinearColor BaseColor = StoneColor;
	switch (BlockType)
	{
	case ELxBlockType::Grass:
		BaseColor = GrassColor;
		break;
	case ELxBlockType::Dirt:
		BaseColor = DirtColor;
		break;
	case ELxBlockType::Stone:
		BaseColor = StoneColor;
		break;
	default:
		return FLinearColor::Transparent;
	}

	const float DirectionBrightness = FaceNormal.Z > 0.5f
		? 1.0f
		: FaceNormal.Z < -0.5f
			? 0.48f
			: FMath::Abs(FaceNormal.X) > 0.5f ? 0.78f : 0.64f;
	return FLinearColor(
		BaseColor.R * DirectionBrightness,
		BaseColor.G * DirectionBrightness,
		BaseColor.B * DirectionBrightness,
		BaseColor.A);
}
