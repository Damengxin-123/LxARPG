// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxSplineSurfaceActor.h"

#include "Components/SplineComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

ALxSplineSurfaceActor::ALxSplineSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SurfaceSpline = CreateDefaultSubobject<USplineComponent>(TEXT("SurfaceSpline"));
	SetRootComponent(SurfaceSpline);
	SurfaceSpline->SetMobility(EComponentMobility::Static);
	SurfaceSpline->ClearSplinePoints(false);
	SurfaceSpline->AddSplinePoint(FVector(-250.0f, -250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SurfaceSpline->AddSplinePoint(FVector(250.0f, -250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SurfaceSpline->AddSplinePoint(FVector(250.0f, 250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	SurfaceSpline->AddSplinePoint(FVector(-250.0f, 250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	for (int32 PointIndex = 0; PointIndex < SurfaceSpline->GetNumberOfSplinePoints(); ++PointIndex)
	{
		SurfaceSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	SurfaceSpline->SetClosedLoop(true, false);
	SurfaceSpline->UpdateSpline();

	SurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceMesh"));
	SurfaceMesh->SetupAttachment(SurfaceSpline);
	// 程序化网格没有可供静态烘焙使用的光照贴图，因此使用可移动渲染路径接收实时光照和阴影。
	SurfaceMesh->SetMobility(EComponentMobility::Movable);
	SurfaceMesh->SetCastShadow(true);
	SurfaceMesh->bCastDynamicShadow = true;
	SurfaceMesh->bCastStaticShadow = false;
	SurfaceMesh->bCastContactShadow = true;
	SurfaceMesh->bAffectDynamicIndirectLighting = true;
	SurfaceMesh->bAffectDistanceFieldLighting = true;
	// 程序化网格体只允许由本类型重建，不允许在继承蓝图中手动修改或复制。
	SurfaceMesh->bEditableWhenInherited = false;
	// 静态地面使用同步生成的简单凸碰撞，避免角色落地时碰撞仍在异步烹饪。
	SurfaceMesh->bUseAsyncCooking = false;
	SurfaceMesh->bUseComplexAsSimpleCollision = false;
	SurfaceMesh->CanCharacterStepUpOn = ECB_Yes;
}

void ALxSplineSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildSurface();
}

void ALxSplineSurfaceActor::RebuildSurface()
{
	SynchronizeSplineControlPointCount();
	ConstrainSplinePointsToActorHeight();
	ClearGeneratedSurface();

	if (!IsValid(SurfaceSpline) || !IsValid(SurfaceMesh) || SurfaceSpline->GetNumberOfSplinePoints() < 3)
	{
		return;
	}

	// 已放入关卡的旧实例可能保存了静态移动性；每次重建都强制切换到动态光照路径。
	SurfaceMesh->SetMobility(EComponentMobility::Movable);

	if (!SurfaceSpline->IsClosedLoop())
	{
		SurfaceSpline->SetClosedLoop(true, false);
		SurfaceSpline->UpdateSpline();
	}

	TArray<FVector> Vertices;
	BuildOutlineVertices(Vertices);
	SanitizeOutlineVertices(Vertices);

	TArray<int32> Triangles;
	if (!TriangulatePolygon(Vertices, Triangles))
	{
		return;
	}
	// 耳切算法以数学坐标系的逆时针顺序输出，而程序化网格的正面绕序相反；翻转后顶面才真正朝上。
	for (int32 TriangleIndex = 0; TriangleIndex + 2 < Triangles.Num(); TriangleIndex += 3)
	{
		Swap(Triangles[TriangleIndex + 1], Triangles[TriangleIndex + 2]);
	}

	// 样条轮廓仍保持可视化编辑，生成结果再应用独立变换，允许在细节面板中精确输入位置、旋转和缩放。
	const FVector SafeSurfaceScale(
		FMath::IsNearlyZero(GeneratedSurfaceScale.X) ? KINDA_SMALL_NUMBER : GeneratedSurfaceScale.X,
		FMath::IsNearlyZero(GeneratedSurfaceScale.Y) ? KINDA_SMALL_NUMBER : GeneratedSurfaceScale.Y,
		FMath::IsNearlyZero(GeneratedSurfaceScale.Z) ? KINDA_SMALL_NUMBER : GeneratedSurfaceScale.Z);
	const FTransform GeneratedSurfaceTransform(GeneratedSurfaceRotation, GeneratedSurfaceLocation, SafeSurfaceScale);
	for (FVector& Vertex : Vertices)
	{
		Vertex = GeneratedSurfaceTransform.TransformPosition(Vertex);
	}

	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	VertexColors.Init(FLinearColor::White, Vertices.Num());
	UV0.Reserve(Vertices.Num());

	const float SafeMaterialDensity = FMath::Max(MaterialDensity, 0.001f);
	const float RotationRadians = FMath::DegreesToRadians(TextureRotation);
	const float RotationCos = FMath::Cos(RotationRadians);
	const float RotationSin = FMath::Sin(RotationRadians);
	for (const FVector& Vertex : Vertices)
	{
		const FVector2D BaseUV(Vertex.X / 100.0f, Vertex.Y / 100.0f);
		const FVector2D RotatedUV(
			BaseUV.X * RotationCos - BaseUV.Y * RotationSin,
			BaseUV.X * RotationSin + BaseUV.Y * RotationCos);
		UV0.Add(RotatedUV * SafeMaterialDensity + TextureOffset);
	}
	// 根据最终三角形和 UV 生成一致的法线、切线空间，保证直接光照和阴影采样方向正确。
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UV0, Normals, Tangents);

	// 程序化网格走动态光照路径，相关阴影标志需要与公开参数同时更新。
	SurfaceMesh->SetCastShadow(bCastShadow);
	SurfaceMesh->bCastDynamicShadow = bCastShadow;
	SurfaceMesh->bCastStaticShadow = false;
	SurfaceMesh->bCastContactShadow = bCastShadow;
	SurfaceMesh->bAffectDynamicIndirectLighting = true;
	SurfaceMesh->bAffectDistanceFieldLighting = true;
	SurfaceMesh->bVisibleInRayTracing = true;
	SurfaceMesh->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision)
	{
		SurfaceMesh->SetCollisionProfileName(CollisionProfileName);
		SurfaceMesh->SetCollisionObjectType(ECC_WorldStatic);
	}
	SurfaceMesh->CreateMeshSection_LinearColor(
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
		BuildStableCollision(Vertices, Triangles);
	}

	SurfaceMesh->SetMaterial(0, SurfaceMaterial);
}

void ALxSplineSurfaceActor::SynchronizeSplineControlPointCount()
{
	if (!IsValid(SurfaceSpline))
	{
		return;
	}

	const int32 ExistingPointCount = SurfaceSpline->GetNumberOfSplinePoints();
	ControlPointCount = FMath::Max(ControlPointCount, 3);
	if (ExistingPointCount == ControlPointCount)
	{
		LastAppliedControlPointCount = ControlPointCount;
		return;
	}

	// 兼容旧关卡以及直接在视口中增删控制点的用法：参数没有变化时，以样条的实际点数为准。
	if (ControlPointCount == LastAppliedControlPointCount && ExistingPointCount >= 3)
	{
		ControlPointCount = ExistingPointCount;
		LastAppliedControlPointCount = ExistingPointCount;
		return;
	}

	if (!SurfaceSpline->IsClosedLoop())
	{
		SurfaceSpline->SetClosedLoop(true, false);
		SurfaceSpline->UpdateSpline();
	}

	TArray<FVector> ResampledPointLocations;
	ResampledPointLocations.Reserve(ControlPointCount);
	const float ExistingSplineLength = SurfaceSpline->GetSplineLength();
	if (ExistingPointCount >= 3 && ExistingSplineLength > KINDA_SMALL_NUMBER)
	{
		for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
		{
			const float SampleDistance = ExistingSplineLength
				* static_cast<float>(PointIndex) / static_cast<float>(ControlPointCount);
			FVector SampleLocation = SurfaceSpline->GetLocationAtDistanceAlongSpline(
				SampleDistance,
				ESplineCoordinateSpace::Local);
			SampleLocation.Z = 0.0f;
			ResampledPointLocations.Add(SampleLocation);
		}
	}
	else
	{
		constexpr float DefaultRadius = 250.0f;
		for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
		{
			const float PointAngle = 2.0f * PI * static_cast<float>(PointIndex) / static_cast<float>(ControlPointCount);
			ResampledPointLocations.Add(FVector(FMath::Cos(PointAngle), FMath::Sin(PointAngle), 0.0f) * DefaultRadius);
		}
	}

	SurfaceSpline->ClearSplinePoints(false);
	for (const FVector& PointLocation : ResampledPointLocations)
	{
		SurfaceSpline->AddSplinePoint(PointLocation, ESplineCoordinateSpace::Local, false);
	}
	for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
	{
		SurfaceSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	SurfaceSpline->SetClosedLoop(true, false);
	SurfaceSpline->UpdateSpline();
	LastAppliedControlPointCount = ControlPointCount;
}

void ALxSplineSurfaceActor::ClearGeneratedSurface()
{
	if (IsValid(SurfaceMesh))
	{
		SurfaceMesh->ClearAllMeshSections();
		SurfaceMesh->ClearCollisionConvexMeshes();
	}
}

void ALxSplineSurfaceActor::ConstrainSplinePointsToActorHeight()
{
	if (!IsValid(SurfaceSpline))
	{
		return;
	}

	bool bUpdatedSpline = false;
	for (int32 PointIndex = 0; PointIndex < SurfaceSpline->GetNumberOfSplinePoints(); ++PointIndex)
	{
		FVector PointLocation = SurfaceSpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::Local);
		if (!FMath::IsNearlyZero(PointLocation.Z))
		{
			PointLocation.Z = 0.0f;
			SurfaceSpline->SetLocationAtSplinePoint(PointIndex, PointLocation, ESplineCoordinateSpace::Local, false);
			bUpdatedSpline = true;
		}
	}

	if (bUpdatedSpline)
	{
		SurfaceSpline->UpdateSpline();
	}
}

void ALxSplineSurfaceActor::BuildOutlineVertices(TArray<FVector>& OutVertices) const
{
	OutVertices.Reset();

	const float SafeSampleLength = FMath::Max(MaxEdgeSampleLength, 5.0f);
	const int32 SplinePointCount = SurfaceSpline->GetNumberOfSplinePoints();
	const float TotalSplineLength = SurfaceSpline->GetSplineLength();
	for (int32 SplineSectionIndex = 0; SplineSectionIndex < SplinePointCount; ++SplineSectionIndex)
	{
		const float SectionStartDistance = SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex);
		const float SectionEndDistance = SplineSectionIndex + 1 < SplinePointCount
			? SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex + 1)
			: TotalSplineLength;
		const float SectionLength = SectionEndDistance - SectionStartDistance;
		const int32 SubdivisionCount = FMath::Max(1, FMath::CeilToInt(SectionLength / SafeSampleLength));

		for (int32 SubdivisionIndex = 0; SubdivisionIndex < SubdivisionCount; ++SubdivisionIndex)
		{
			const float Alpha = static_cast<float>(SubdivisionIndex) / static_cast<float>(SubdivisionCount);
			const float SampleDistance = FMath::Lerp(SectionStartDistance, SectionEndDistance, Alpha);
			FVector SamplePosition = SurfaceSpline->GetLocationAtDistanceAlongSpline(SampleDistance, ESplineCoordinateSpace::Local);
			SamplePosition.Z = HeightOffset;
			OutVertices.Add(SamplePosition);
		}
	}
}

void ALxSplineSurfaceActor::SanitizeOutlineVertices(TArray<FVector>& InOutVertices) const
{
	constexpr float DuplicatePointToleranceSquared = 0.01f;
	constexpr double CollinearTolerance = 0.01;

	for (int32 VertexIndex = InOutVertices.Num() - 1; VertexIndex >= 0 && InOutVertices.Num() >= 3; --VertexIndex)
	{
		const int32 NextIndex = (VertexIndex + 1) % InOutVertices.Num();
		if (FVector::DistSquared2D(InOutVertices[VertexIndex], InOutVertices[NextIndex]) <= DuplicatePointToleranceSquared)
		{
			InOutVertices.RemoveAt(VertexIndex);
		}
	}

	bool bRemovedVertex = true;
	while (bRemovedVertex && InOutVertices.Num() >= 3)
	{
		bRemovedVertex = false;
		for (int32 VertexIndex = 0; VertexIndex < InOutVertices.Num(); ++VertexIndex)
		{
			const FVector2D Previous(InOutVertices[(VertexIndex - 1 + InOutVertices.Num()) % InOutVertices.Num()]);
			const FVector2D Current(InOutVertices[VertexIndex]);
			const FVector2D Next(InOutVertices[(VertexIndex + 1) % InOutVertices.Num()]);
			const FVector2D Incoming = Current - Previous;
			const FVector2D Outgoing = Next - Current;
			const double CrossProduct = static_cast<double>(Incoming.X) * Outgoing.Y - static_cast<double>(Incoming.Y) * Outgoing.X;
			if (FMath::Abs(CrossProduct) <= CollinearTolerance && FVector2D::DotProduct(Incoming, Outgoing) >= 0.0f)
			{
				InOutVertices.RemoveAt(VertexIndex);
				bRemovedVertex = true;
				break;
			}
		}
	}
}

bool ALxSplineSurfaceActor::TriangulatePolygon(TArray<FVector>& InOutVertices, TArray<int32>& OutTriangles) const
{
	OutTriangles.Reset();
	if (InOutVertices.Num() < 3)
	{
		return false;
	}

	double SignedArea = 0.0;
	for (int32 VertexIndex = 0; VertexIndex < InOutVertices.Num(); ++VertexIndex)
	{
		const FVector& Current = InOutVertices[VertexIndex];
		const FVector& Next = InOutVertices[(VertexIndex + 1) % InOutVertices.Num()];
		SignedArea += static_cast<double>(Current.X) * Next.Y - static_cast<double>(Next.X) * Current.Y;
	}
	if (FMath::Abs(SignedArea) <= UE_SMALL_NUMBER)
	{
		return false;
	}
	if (SignedArea < 0.0)
	{
		Algo::Reverse(InOutVertices);
	}

	TArray<int32> RemainingIndices;
	RemainingIndices.Reserve(InOutVertices.Num());
	for (int32 VertexIndex = 0; VertexIndex < InOutVertices.Num(); ++VertexIndex)
	{
		RemainingIndices.Add(VertexIndex);
	}

	int32 SafetyCounter = InOutVertices.Num() * InOutVertices.Num();
	while (RemainingIndices.Num() > 3 && SafetyCounter-- > 0)
	{
		bool bFoundEar = false;
		for (int32 RemainingIndex = 0; RemainingIndex < RemainingIndices.Num(); ++RemainingIndex)
		{
			const int32 PreviousVertexIndex = RemainingIndices[(RemainingIndex - 1 + RemainingIndices.Num()) % RemainingIndices.Num()];
			const int32 CurrentVertexIndex = RemainingIndices[RemainingIndex];
			const int32 NextVertexIndex = RemainingIndices[(RemainingIndex + 1) % RemainingIndices.Num()];
			const FVector2D A(InOutVertices[PreviousVertexIndex]);
			const FVector2D B(InOutVertices[CurrentVertexIndex]);
			const FVector2D C(InOutVertices[NextVertexIndex]);
			const double CrossProduct = static_cast<double>(B.X - A.X) * (C.Y - B.Y)
				- static_cast<double>(B.Y - A.Y) * (C.X - B.X);
			if (CrossProduct <= UE_SMALL_NUMBER)
			{
				continue;
			}

			bool bContainsOtherVertex = false;
			for (const int32 TestVertexIndex : RemainingIndices)
			{
				if (TestVertexIndex == PreviousVertexIndex || TestVertexIndex == CurrentVertexIndex || TestVertexIndex == NextVertexIndex)
				{
					continue;
				}
				if (IsPointInsideTriangle2D(FVector2D(InOutVertices[TestVertexIndex]), A, B, C))
				{
					bContainsOtherVertex = true;
					break;
				}
			}

			if (!bContainsOtherVertex)
			{
				OutTriangles.Append({PreviousVertexIndex, CurrentVertexIndex, NextVertexIndex});
				RemainingIndices.RemoveAt(RemainingIndex);
				bFoundEar = true;
				break;
			}
		}

		if (!bFoundEar)
		{
			OutTriangles.Reset();
			return false;
		}
	}

	if (RemainingIndices.Num() == 3)
	{
		OutTriangles.Append({RemainingIndices[0], RemainingIndices[1], RemainingIndices[2]});
	}
	return OutTriangles.Num() >= 3;
}

void ALxSplineSurfaceActor::BuildStableCollision(const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
{
	if (!IsValid(SurfaceMesh) || Vertices.Num() < 3 || Triangles.Num() < 3)
	{
		return;
	}

	const float SafeCollisionThickness = FMath::Max(CollisionThickness, 1.0f);
	const FVector BottomOffset(0.0f, 0.0f, -SafeCollisionThickness);
	TArray<TArray<FVector>> CollisionConvexMeshes;
	CollisionConvexMeshes.Reserve(Triangles.Num() / 3);
	for (int32 TriangleIndex = 0; TriangleIndex + 2 < Triangles.Num(); TriangleIndex += 3)
	{
		const int32 IndexA = Triangles[TriangleIndex];
		const int32 IndexB = Triangles[TriangleIndex + 1];
		const int32 IndexC = Triangles[TriangleIndex + 2];
		if (!Vertices.IsValidIndex(IndexA) || !Vertices.IsValidIndex(IndexB) || !Vertices.IsValidIndex(IndexC))
		{
			continue;
		}

		// 每个可见三角形对应一个向下挤出的三棱柱，组合后可以准确覆盖凹多边形。
		TArray<FVector> ConvexPrismVertices;
		ConvexPrismVertices.Reserve(6);
		ConvexPrismVertices.Add(Vertices[IndexA]);
		ConvexPrismVertices.Add(Vertices[IndexB]);
		ConvexPrismVertices.Add(Vertices[IndexC]);
		ConvexPrismVertices.Add(Vertices[IndexA] + BottomOffset);
		ConvexPrismVertices.Add(Vertices[IndexB] + BottomOffset);
		ConvexPrismVertices.Add(Vertices[IndexC] + BottomOffset);
		CollisionConvexMeshes.Add(MoveTemp(ConvexPrismVertices));
	}

	// 一次性提交全部凸体，避免逐三角形反复烹饪碰撞。
	SurfaceMesh->SetCollisionConvexMeshes(CollisionConvexMeshes);
}

bool ALxSplineSurfaceActor::IsPointInsideTriangle2D(
	const FVector2D& Point,
	const FVector2D& A,
	const FVector2D& B,
	const FVector2D& C) const
{
	const auto EdgeCross = [](const FVector2D& EdgeStart, const FVector2D& EdgeEnd, const FVector2D& TestPoint)
	{
		return static_cast<double>(EdgeEnd.X - EdgeStart.X) * (TestPoint.Y - EdgeStart.Y)
			- static_cast<double>(EdgeEnd.Y - EdgeStart.Y) * (TestPoint.X - EdgeStart.X);
	};

	constexpr double InsideTolerance = -0.001;
	return EdgeCross(A, B, Point) >= InsideTolerance
		&& EdgeCross(B, C, Point) >= InsideTolerance
		&& EdgeCross(C, A, Point) >= InsideTolerance;
}
