// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxStraightStairActor.h"

#include "KismetProceduralMeshLibrary.h"
#include "ProceduralMeshComponent.h"
#include "UObject/UnrealType.h"

ALxStraightStairActor::ALxStraightStairActor()
{
	PrimaryActorTick.bCanEverTick = false;

	StraightStairMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StraightStairMesh"));
	SetRootComponent(StraightStairMesh);
	StraightStairMesh->SetMobility(EComponentMobility::Movable);
	StraightStairMesh->SetCastShadow(true);
	StraightStairMesh->bCastDynamicShadow = true;
	StraightStairMesh->bCastStaticShadow = false;
	StraightStairMesh->bCastContactShadow = true;
	StraightStairMesh->bEditableWhenInherited = false;
	StraightStairMesh->bUseAsyncCooking = false;
	StraightStairMesh->bUseComplexAsSimpleCollision = false;
	StraightStairMesh->CanCharacterStepUpOn = ECB_Yes;
	StraightStairMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ALxStraightStairActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildStraightStair();
}

#if WITH_EDITOR
void ALxStraightStairActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	StepCount = FMath::Clamp(StepCount, 1, 512);

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ALxStraightStairActor, StepLength))
	{
		StepLength = FMath::Max(StepLength, 1.0f);
		StairLength = StepLength * static_cast<float>(StepCount);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALxStraightStairActor, StepHeight))
	{
		StepHeight = FMath::Max(StepHeight, 1.0f);
		StairHeight = StepHeight * static_cast<float>(StepCount);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALxStraightStairActor, StepCount))
	{
		StairLength = FMath::Max(StairLength, 1.0f);
		StairHeight = FMath::Max(StairHeight, 1.0f);
		StepLength = StairLength / static_cast<float>(StepCount);
		StepHeight = StairHeight / static_cast<float>(StepCount);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALxStraightStairActor, StairLength))
	{
		StairLength = FMath::Max(StairLength, 1.0f);
		StepLength = StairLength / static_cast<float>(StepCount);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ALxStraightStairActor, StairHeight))
	{
		StairHeight = FMath::Max(StairHeight, 1.0f);
		StepHeight = StairHeight / static_cast<float>(StepCount);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void ALxStraightStairActor::RebuildStraightStair()
{
	ClearGeneratedStraightStair();
	if (!IsValid(StraightStairMesh))
	{
		return;
	}

	SanitizeDimensions();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	const float SafeMaterialDensity = FMath::Max(MaterialDensity, 0.001f);
	const float RotationRadians = FMath::DegreesToRadians(MaterialRotation);
	const float RotationCos = FMath::Cos(RotationRadians);
	const float RotationSin = FMath::Sin(RotationRadians);
	const float HalfWidth = StairWidth * 0.5f;

	/** 将厘米尺寸转换为经过密度、偏转和位移处理的纹理坐标。 */
	const auto MakeUV = [SafeMaterialDensity, RotationCos, RotationSin, this](const float U, const float V)
	{
		const FVector2D BaseUV(U / 100.0f, V / 100.0f);
		const FVector2D RotatedUV(
			BaseUV.X * RotationCos - BaseUV.Y * RotationSin,
			BaseUV.X * RotationSin + BaseUV.Y * RotationCos);
		return RotatedUV * SafeMaterialDensity + MaterialOffset;
	};

	/** 添加一个具有独立顶点和 UV 的平面，四个点按几何法线朝向外侧的顺序排列。 */
	const auto AddFace = [&Vertices, &Triangles, &UV0, &VertexColors, &MakeUV](
		const FVector& P0,
		const FVector& P1,
		const FVector& P2,
		const FVector& P3,
		const FVector2D& UV00,
		const FVector2D& UV10,
		const FVector2D& UV01,
		const FVector2D& UV11)
	{
		const int32 StartIndex = Vertices.Num();
		Vertices.Append({P0, P1, P2, P3});
		// 程序化网格使用顺时针顶点作为正面，因此需要反转数学坐标系中的外侧绕序。
		Triangles.Append({StartIndex, StartIndex + 2, StartIndex + 1, StartIndex + 2, StartIndex + 3, StartIndex + 1});
		UV0.Append({
			MakeUV(UV00.X, UV00.Y),
			MakeUV(UV10.X, UV10.Y),
			MakeUV(UV01.X, UV01.Y),
			MakeUV(UV11.X, UV11.Y)});
		VertexColors.Append({FLinearColor::White, FLinearColor::White, FLinearColor::White, FLinearColor::White});
	};

	for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
	{
		const float StartX = StepLength * static_cast<float>(StepIndex);
		const float EndX = StepLength * static_cast<float>(StepIndex + 1);
		const float PreviousHeight = StepHeight * static_cast<float>(StepIndex);
		const float CurrentHeight = StepHeight * static_cast<float>(StepIndex + 1);

		// 每一级踏面都从相同的 UV 原点开始，确保相同材质图案在各级踏面上的位置一致。
		AddFace(
			FVector(StartX, -HalfWidth, CurrentHeight), FVector(EndX, -HalfWidth, CurrentHeight),
			FVector(StartX, HalfWidth, CurrentHeight), FVector(EndX, HalfWidth, CurrentHeight),
			FVector2D(0.0f, 0.0f), FVector2D(StepLength, 0.0f),
			FVector2D(0.0f, StairWidth), FVector2D(StepLength, StairWidth));

		// 每一级立板同样使用独立的局部 UV，使所有正面立板显示完全一致的材质区域。
		AddFace(
			FVector(StartX, -HalfWidth, PreviousHeight), FVector(StartX, -HalfWidth, CurrentHeight),
			FVector(StartX, HalfWidth, PreviousHeight), FVector(StartX, HalfWidth, CurrentHeight),
			FVector2D(0.0f, 0.0f), FVector2D(0.0f, StepHeight),
			FVector2D(StairWidth, 0.0f), FVector2D(StairWidth, StepHeight));

		// 左右两侧按每阶分别封闭，拼合后形成阶梯化的直角三角形侧面。
		AddFace(
			FVector(StartX, -HalfWidth, 0.0f), FVector(EndX, -HalfWidth, 0.0f),
			FVector(StartX, -HalfWidth, CurrentHeight), FVector(EndX, -HalfWidth, CurrentHeight),
			FVector2D(StartX, 0.0f), FVector2D(EndX, 0.0f),
			FVector2D(StartX, CurrentHeight), FVector2D(EndX, CurrentHeight));
		AddFace(
			FVector(StartX, HalfWidth, 0.0f), FVector(StartX, HalfWidth, CurrentHeight),
			FVector(EndX, HalfWidth, 0.0f), FVector(EndX, HalfWidth, CurrentHeight),
			FVector2D(StartX, 0.0f), FVector2D(StartX, CurrentHeight),
			FVector2D(EndX, 0.0f), FVector2D(EndX, CurrentHeight));
	}

	// 封闭最高端背面与底面，使楼梯成为完整实体。
	AddFace(
		FVector(StairLength, -HalfWidth, 0.0f), FVector(StairLength, HalfWidth, 0.0f),
		FVector(StairLength, -HalfWidth, StairHeight), FVector(StairLength, HalfWidth, StairHeight),
		FVector2D(0.0f, 0.0f), FVector2D(StairWidth, 0.0f),
		FVector2D(0.0f, StairHeight), FVector2D(StairWidth, StairHeight));
	AddFace(
		FVector(0.0f, -HalfWidth, 0.0f), FVector(0.0f, HalfWidth, 0.0f),
		FVector(StairLength, -HalfWidth, 0.0f), FVector(StairLength, HalfWidth, 0.0f),
		FVector2D(0.0f, 0.0f), FVector2D(0.0f, StairWidth),
		FVector2D(StairLength, 0.0f), FVector2D(StairLength, StairWidth));

	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

	StraightStairMesh->SetMobility(EComponentMobility::Movable);
	StraightStairMesh->SetCastShadow(bCastShadow);
	StraightStairMesh->bCastDynamicShadow = bCastShadow;
	StraightStairMesh->bCastStaticShadow = false;
	StraightStairMesh->bCastContactShadow = bCastShadow;
	StraightStairMesh->bVisibleInRayTracing = true;
	StraightStairMesh->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision)
	{
		StraightStairMesh->SetCollisionProfileName(CollisionProfileName);
		StraightStairMesh->SetCollisionObjectType(ECC_WorldStatic);
	}
	StraightStairMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		false);
	if (bEnableCollision)
	{
		BuildStableCollision();
	}
	StraightStairMesh->SetMaterial(0, StairMaterial);
}

void ALxStraightStairActor::ClearGeneratedStraightStair()
{
	if (IsValid(StraightStairMesh))
	{
		StraightStairMesh->ClearAllMeshSections();
		StraightStairMesh->ClearCollisionConvexMeshes();
	}
}

void ALxStraightStairActor::SanitizeDimensions()
{
	StepCount = FMath::Clamp(StepCount, 1, 512);
	StairWidth = FMath::Max(StairWidth, 1.0f);
	StairHeight = FMath::Max(StairHeight, 1.0f);
	StairLength = FMath::Max(StairLength, 1.0f);
	StepLength = StairLength / static_cast<float>(StepCount);
	StepHeight = StairHeight / static_cast<float>(StepCount);
}

void ALxStraightStairActor::BuildStableCollision()
{
	if (!IsValid(StraightStairMesh) || StepCount < 1)
	{
		return;
	}

	const float HalfWidth = StairWidth * 0.5f;
	TArray<TArray<FVector>> CollisionConvexMeshes;
	CollisionConvexMeshes.Reserve(StepCount);
	for (int32 StepIndex = 0; StepIndex < StepCount; ++StepIndex)
	{
		const float StartX = StepLength * static_cast<float>(StepIndex);
		const float EndX = StepLength * static_cast<float>(StepIndex + 1);
		const float CurrentHeight = StepHeight * static_cast<float>(StepIndex + 1);

		// 每一级使用从地面延伸到踏面的封闭长方体，多个凸体组合后准确贴合阶梯轮廓。
		TArray<FVector> StepCollisionVertices;
		StepCollisionVertices.Reserve(8);
		StepCollisionVertices.Append({
			FVector(StartX, -HalfWidth, 0.0f),
			FVector(EndX, -HalfWidth, 0.0f),
			FVector(StartX, HalfWidth, 0.0f),
			FVector(EndX, HalfWidth, 0.0f),
			FVector(StartX, -HalfWidth, CurrentHeight),
			FVector(EndX, -HalfWidth, CurrentHeight),
			FVector(StartX, HalfWidth, CurrentHeight),
			FVector(EndX, HalfWidth, CurrentHeight)});
		CollisionConvexMeshes.Add(MoveTemp(StepCollisionVertices));
	}

	StraightStairMesh->SetCollisionConvexMeshes(CollisionConvexMeshes);
}
