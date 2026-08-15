// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxRingCylinderActor.h"

#include "ProceduralMeshComponent.h"

ALxRingCylinderActor::ALxRingCylinderActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RingCylinderMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RingCylinderMesh"));
	SetRootComponent(RingCylinderMesh);
	RingCylinderMesh->SetMobility(EComponentMobility::Movable);
	RingCylinderMesh->SetCastShadow(true);
	RingCylinderMesh->bCastDynamicShadow = true;
	RingCylinderMesh->bCastStaticShadow = false;
	RingCylinderMesh->bEditableWhenInherited = false;
	RingCylinderMesh->bUseAsyncCooking = false;
	RingCylinderMesh->bUseComplexAsSimpleCollision = true;
	RingCylinderMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ALxRingCylinderActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildRingCylinder();
}

void ALxRingCylinderActor::RebuildRingCylinder()
{
	ClearGeneratedRingCylinder();
	if (!IsValid(RingCylinderMesh))
	{
		return;
	}

	const float SafeInnerRadius = FMath::Clamp(InnerRadius, 0.0f, BaseOuterRadius - 0.1f);
	const bool bHasHole = SafeInnerRadius > KINDA_SMALL_NUMBER;
	const float HalfHeight = BaseHeight * 0.5f;
	const float SafeMaterialDensity = FMath::Max(MaterialDensity, 0.001f);
	const FVector ActorScale = GetActorScale3D().GetAbs();
	const float CircumferenceScale = (ActorScale.X + ActorScale.Y) * 0.5f;
	const float OuterCircumference = 2.0f * PI * BaseOuterRadius * CircumferenceScale;
	const float InnerCircumference = 2.0f * PI * SafeInnerRadius * CircumferenceScale;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;

	/** 向网格数据中添加一个带完整切线空间信息的顶点。 */
	const auto AddVertex = [&Vertices, &Normals, &UV0, &VertexColors, &Tangents](
		const FVector& Position,
		const FVector& Normal,
		const FVector2D& UV,
		const FVector& Tangent)
	{
		const int32 NewVertexIndex = Vertices.Add(Position);
		Normals.Add(Normal);
		UV0.Add(UV);
		VertexColors.Add(FLinearColor::White);
		Tangents.Add(FProcMeshTangent(Tangent, false));
		return NewVertexIndex;
	};

	/** 按虚幻程序化网格的正面绕序添加一个四边形。 */
	const auto AddQuad = [&Triangles](const int32 A, const int32 B, const int32 C, const int32 D)
	{
		Triangles.Append({A, B, C, C, B, D});
	};

	for (int32 SegmentIndex = 0; SegmentIndex < CircleSegmentCount; ++SegmentIndex)
	{
		const float StartAlpha = static_cast<float>(SegmentIndex) / static_cast<float>(CircleSegmentCount);
		const float EndAlpha = static_cast<float>(SegmentIndex + 1) / static_cast<float>(CircleSegmentCount);
		const float StartAngle = StartAlpha * 2.0f * PI;
		const float EndAngle = EndAlpha * 2.0f * PI;
		const FVector StartDirection(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0.0f);
		const FVector EndDirection(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0.0f);
		const FVector StartTangent(-StartDirection.Y, StartDirection.X, 0.0f);
		const FVector EndTangent(-EndDirection.Y, EndDirection.X, 0.0f);
		const FVector OuterStartBottom = StartDirection * BaseOuterRadius - FVector(0.0f, 0.0f, HalfHeight);
		const FVector OuterStartTop = StartDirection * BaseOuterRadius + FVector(0.0f, 0.0f, HalfHeight);
		const FVector OuterEndBottom = EndDirection * BaseOuterRadius - FVector(0.0f, 0.0f, HalfHeight);
		const FVector OuterEndTop = EndDirection * BaseOuterRadius + FVector(0.0f, 0.0f, HalfHeight);
		const float OuterStartU = StartAlpha * OuterCircumference / 100.0f * SafeMaterialDensity;
		const float OuterEndU = EndAlpha * OuterCircumference / 100.0f * SafeMaterialDensity;
		const float HeightV = BaseHeight * ActorScale.Z / 100.0f * SafeMaterialDensity;

		const int32 OuterStartBottomIndex = AddVertex(OuterStartBottom, StartDirection, FVector2D(OuterStartU, 0.0f), StartTangent);
		const int32 OuterStartTopIndex = AddVertex(OuterStartTop, StartDirection, FVector2D(OuterStartU, HeightV), StartTangent);
		const int32 OuterEndBottomIndex = AddVertex(OuterEndBottom, EndDirection, FVector2D(OuterEndU, 0.0f), EndTangent);
		const int32 OuterEndTopIndex = AddVertex(OuterEndTop, EndDirection, FVector2D(OuterEndU, HeightV), EndTangent);
		AddQuad(OuterStartBottomIndex, OuterStartTopIndex, OuterEndBottomIndex, OuterEndTopIndex);

		const auto MakeCapUV = [ActorScale, SafeMaterialDensity](const FVector& Position)
		{
			return FVector2D(Position.X * ActorScale.X, Position.Y * ActorScale.Y) / 100.0f * SafeMaterialDensity;
		};

		if (bHasHole)
		{
			const FVector InnerStartBottom = StartDirection * SafeInnerRadius - FVector(0.0f, 0.0f, HalfHeight);
			const FVector InnerStartTop = StartDirection * SafeInnerRadius + FVector(0.0f, 0.0f, HalfHeight);
			const FVector InnerEndBottom = EndDirection * SafeInnerRadius - FVector(0.0f, 0.0f, HalfHeight);
			const FVector InnerEndTop = EndDirection * SafeInnerRadius + FVector(0.0f, 0.0f, HalfHeight);
			const float InnerStartU = StartAlpha * InnerCircumference / 100.0f * SafeMaterialDensity;
			const float InnerEndU = EndAlpha * InnerCircumference / 100.0f * SafeMaterialDensity;

			const int32 InnerStartBottomIndex = AddVertex(InnerStartBottom, -StartDirection, FVector2D(InnerStartU, 0.0f), -StartTangent);
			const int32 InnerEndBottomIndex = AddVertex(InnerEndBottom, -EndDirection, FVector2D(InnerEndU, 0.0f), -EndTangent);
			const int32 InnerStartTopIndex = AddVertex(InnerStartTop, -StartDirection, FVector2D(InnerStartU, HeightV), -StartTangent);
			const int32 InnerEndTopIndex = AddVertex(InnerEndTop, -EndDirection, FVector2D(InnerEndU, HeightV), -EndTangent);
			AddQuad(InnerStartBottomIndex, InnerEndBottomIndex, InnerStartTopIndex, InnerEndTopIndex);

			const int32 TopOuterStartIndex = AddVertex(OuterStartTop, FVector::UpVector, MakeCapUV(OuterStartTop), StartTangent);
			const int32 TopOuterEndIndex = AddVertex(OuterEndTop, FVector::UpVector, MakeCapUV(OuterEndTop), EndTangent);
			const int32 TopInnerStartIndex = AddVertex(InnerStartTop, FVector::UpVector, MakeCapUV(InnerStartTop), StartTangent);
			const int32 TopInnerEndIndex = AddVertex(InnerEndTop, FVector::UpVector, MakeCapUV(InnerEndTop), EndTangent);
			AddQuad(TopOuterStartIndex, TopInnerStartIndex, TopOuterEndIndex, TopInnerEndIndex);

			const int32 BottomOuterStartIndex = AddVertex(OuterStartBottom, FVector::DownVector, MakeCapUV(OuterStartBottom), -StartTangent);
			const int32 BottomInnerStartIndex = AddVertex(InnerStartBottom, FVector::DownVector, MakeCapUV(InnerStartBottom), -StartTangent);
			const int32 BottomOuterEndIndex = AddVertex(OuterEndBottom, FVector::DownVector, MakeCapUV(OuterEndBottom), -EndTangent);
			const int32 BottomInnerEndIndex = AddVertex(InnerEndBottom, FVector::DownVector, MakeCapUV(InnerEndBottom), -EndTangent);
			AddQuad(BottomOuterStartIndex, BottomOuterEndIndex, BottomInnerStartIndex, BottomInnerEndIndex);
		}
		else
		{
			const FVector TopCenter(0.0f, 0.0f, HalfHeight);
			const FVector BottomCenter(0.0f, 0.0f, -HalfHeight);
			const int32 TopCenterIndex = AddVertex(TopCenter, FVector::UpVector, MakeCapUV(TopCenter), FVector::ForwardVector);
			const int32 TopEndIndex = AddVertex(OuterEndTop, FVector::UpVector, MakeCapUV(OuterEndTop), EndTangent);
			const int32 TopStartIndex = AddVertex(OuterStartTop, FVector::UpVector, MakeCapUV(OuterStartTop), StartTangent);
			Triangles.Append({TopCenterIndex, TopEndIndex, TopStartIndex});

			const int32 BottomCenterIndex = AddVertex(BottomCenter, FVector::DownVector, MakeCapUV(BottomCenter), FVector::ForwardVector);
			const int32 BottomStartIndex = AddVertex(OuterStartBottom, FVector::DownVector, MakeCapUV(OuterStartBottom), -StartTangent);
			const int32 BottomEndIndex = AddVertex(OuterEndBottom, FVector::DownVector, MakeCapUV(OuterEndBottom), -EndTangent);
			Triangles.Append({BottomCenterIndex, BottomStartIndex, BottomEndIndex});
		}
	}

	RingCylinderMesh->SetCastShadow(true);
	RingCylinderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RingCylinderMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UV0,
		VertexColors,
		Tangents,
		true);
	RingCylinderMesh->SetMaterial(0, RingCylinderMaterial);
}

void ALxRingCylinderActor::ClearGeneratedRingCylinder()
{
	if (IsValid(RingCylinderMesh))
	{
		RingCylinderMesh->ClearAllMeshSections();
		RingCylinderMesh->ClearCollisionConvexMeshes();
	}
}
