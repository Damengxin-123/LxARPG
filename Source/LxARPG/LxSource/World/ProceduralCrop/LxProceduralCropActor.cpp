// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxProceduralCropActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"

ALxProceduralCropActor::ALxProceduralCropActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CropBoundarySpline = CreateDefaultSubobject<USplineComponent>(TEXT("CropBoundarySpline"));
	SetRootComponent(CropBoundarySpline);
	CropBoundarySpline->SetMobility(EComponentMobility::Static);
	CropBoundarySpline->ClearSplinePoints(false);
	CropBoundarySpline->AddSplinePoint(FVector(-500.0f, -250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	CropBoundarySpline->AddSplinePoint(FVector(500.0f, -250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	CropBoundarySpline->AddSplinePoint(FVector(500.0f, 250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	CropBoundarySpline->AddSplinePoint(FVector(-500.0f, 250.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	for (int32 PointIndex = 0; PointIndex < CropBoundarySpline->GetNumberOfSplinePoints(); ++PointIndex)
	{
		CropBoundarySpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	CropBoundarySpline->SetClosedLoop(true, false);
	CropBoundarySpline->UpdateSpline();

	CropInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CropInstances"));
	CropInstances->SetupAttachment(CropBoundarySpline);
	CropInstances->SetMobility(EComponentMobility::Static);
	CropInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CropInstances->SetCastShadow(true);
	CropInstances->bEditableWhenInherited = false;
}

void ALxProceduralCropActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildCrops();
}

void ALxProceduralCropActor::RebuildCrops()
{
	SynchronizeSplineControlPointCount();
	ConstrainSplinePointsToActorHeight();
	ClearGeneratedCrops();

	if (!IsValid(CropBoundarySpline) || !IsValid(CropInstances)
		|| !IsValid(CropMesh) || CropBoundarySpline->GetNumberOfSplinePoints() < 3)
	{
		return;
	}

	if (!CropBoundarySpline->IsClosedLoop())
	{
		CropBoundarySpline->SetClosedLoop(true, false);
		CropBoundarySpline->UpdateSpline();
	}

	CropInstances->SetStaticMesh(CropMesh);

	TArray<FVector2D> BoundaryPoints;
	BuildBoundaryPolygon(BoundaryPoints);
	if (BoundaryPoints.Num() < 3)
	{
		return;
	}

	FBox2D BoundaryBounds(EForceInit::ForceInit);
	for (const FVector2D& BoundaryPoint : BoundaryPoints)
	{
		BoundaryBounds += BoundaryPoint;
	}
	if (!BoundaryBounds.bIsValid)
	{
		return;
	}

	ColumnsPerGroup = FMath::Max(ColumnsPerGroup, 1);
	RowCount = FMath::Max(RowCount, 1);
	IntraGroupColumnSpacing = FMath::Max(IntraGroupColumnSpacing, 1.0f);
	GroupSpacing = FMath::Max(GroupSpacing, 1.0f);
	RowSpacing = FMath::Max(RowSpacing, 1.0f);

	// 每组以本地 X=组索引*组跨度为中心，组内列也关于组中心对称，便于围绕演员原点稳定调整范围。
	const float GroupWidth = static_cast<float>(ColumnsPerGroup - 1) * IntraGroupColumnSpacing;
	const float GroupPitch = GroupWidth + GroupSpacing;
	const int32 MinimumGroupIndex = FMath::FloorToInt((BoundaryBounds.Min.X - GroupWidth * 0.5f) / GroupPitch) - 1;
	const int32 MaximumGroupIndex = FMath::CeilToInt((BoundaryBounds.Max.X + GroupWidth * 0.5f) / GroupPitch) + 1;
	const float FirstRowY = -static_cast<float>(RowCount - 1) * RowSpacing * 0.5f;

	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.Reserve((MaximumGroupIndex - MinimumGroupIndex + 1) * ColumnsPerGroup * RowCount);
	for (int32 RowIndex = 0; RowIndex < RowCount; ++RowIndex)
	{
		const float RowY = FirstRowY + static_cast<float>(RowIndex) * RowSpacing;
		for (int32 GroupIndex = MinimumGroupIndex; GroupIndex <= MaximumGroupIndex; ++GroupIndex)
		{
			const float GroupCenterX = static_cast<float>(GroupIndex) * GroupPitch;
			for (int32 ColumnIndex = 0; ColumnIndex < ColumnsPerGroup; ++ColumnIndex)
			{
				const float ColumnX = GroupCenterX - GroupWidth * 0.5f
					+ static_cast<float>(ColumnIndex) * IntraGroupColumnSpacing;
				if (IsPointInsideBoundary(FVector2D(ColumnX, RowY), BoundaryPoints))
				{
					InstanceTransforms.Emplace(FRotator::ZeroRotator, FVector(ColumnX, RowY, CropHeight));
				}
			}
		}
	}

	CropInstances->AddInstances(InstanceTransforms, false, false, true);
}

void ALxProceduralCropActor::ClearGeneratedCrops()
{
	if (IsValid(CropInstances))
	{
		CropInstances->ClearInstances();
	}
}

void ALxProceduralCropActor::SynchronizeSplineControlPointCount()
{
	if (!IsValid(CropBoundarySpline))
	{
		return;
	}

	const int32 ExistingPointCount = CropBoundarySpline->GetNumberOfSplinePoints();
	ControlPointCount = FMath::Max(ControlPointCount, 3);
	if (ExistingPointCount == ControlPointCount)
	{
		LastAppliedControlPointCount = ControlPointCount;
		return;
	}

	// 参数未变化时尊重用户在视口中手动增删的控制点，并把实际数量同步回属性。
	if (ControlPointCount == LastAppliedControlPointCount && ExistingPointCount >= 3)
	{
		ControlPointCount = ExistingPointCount;
		LastAppliedControlPointCount = ExistingPointCount;
		return;
	}

	if (!CropBoundarySpline->IsClosedLoop())
	{
		CropBoundarySpline->SetClosedLoop(true, false);
		CropBoundarySpline->UpdateSpline();
	}

	TArray<FVector> ResampledLocations;
	ResampledLocations.Reserve(ControlPointCount);
	const float ExistingSplineLength = CropBoundarySpline->GetSplineLength();
	if (ExistingPointCount >= 3 && ExistingSplineLength > KINDA_SMALL_NUMBER)
	{
		for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
		{
			const float SampleDistance = ExistingSplineLength
				* static_cast<float>(PointIndex) / static_cast<float>(ControlPointCount);
			FVector SampleLocation = CropBoundarySpline->GetLocationAtDistanceAlongSpline(
				SampleDistance,
				ESplineCoordinateSpace::Local);
			SampleLocation.Z = 0.0f;
			ResampledLocations.Add(SampleLocation);
		}
	}
	else
	{
		constexpr float DefaultRadius = 500.0f;
		for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
		{
			const float PointAngle = 2.0f * PI * static_cast<float>(PointIndex) / static_cast<float>(ControlPointCount);
			ResampledLocations.Add(FVector(FMath::Cos(PointAngle), FMath::Sin(PointAngle), 0.0f) * DefaultRadius);
		}
	}

	CropBoundarySpline->ClearSplinePoints(false);
	for (const FVector& PointLocation : ResampledLocations)
	{
		CropBoundarySpline->AddSplinePoint(PointLocation, ESplineCoordinateSpace::Local, false);
	}
	for (int32 PointIndex = 0; PointIndex < ControlPointCount; ++PointIndex)
	{
		CropBoundarySpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
	}
	CropBoundarySpline->SetClosedLoop(true, false);
	CropBoundarySpline->UpdateSpline();
	LastAppliedControlPointCount = ControlPointCount;
}

void ALxProceduralCropActor::ConstrainSplinePointsToActorHeight()
{
	if (!IsValid(CropBoundarySpline))
	{
		return;
	}

	bool bUpdatedSpline = false;
	for (int32 PointIndex = 0; PointIndex < CropBoundarySpline->GetNumberOfSplinePoints(); ++PointIndex)
	{
		FVector PointLocation = CropBoundarySpline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::Local);
		if (!FMath::IsNearlyZero(PointLocation.Z))
		{
			PointLocation.Z = 0.0f;
			CropBoundarySpline->SetLocationAtSplinePoint(PointIndex, PointLocation, ESplineCoordinateSpace::Local, false);
			bUpdatedSpline = true;
		}
	}

	if (bUpdatedSpline)
	{
		CropBoundarySpline->UpdateSpline();
	}
}

void ALxProceduralCropActor::BuildBoundaryPolygon(TArray<FVector2D>& OutBoundaryPoints) const
{
	OutBoundaryPoints.Reset();
	if (!IsValid(CropBoundarySpline))
	{
		return;
	}

	const float SafeSampleLength = FMath::Max(MaxBoundarySampleLength, 5.0f);
	const int32 SplinePointCount = CropBoundarySpline->GetNumberOfSplinePoints();
	const float TotalSplineLength = CropBoundarySpline->GetSplineLength();
	for (int32 SplineSectionIndex = 0; SplineSectionIndex < SplinePointCount; ++SplineSectionIndex)
	{
		const float SectionStartDistance = CropBoundarySpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex);
		const float SectionEndDistance = SplineSectionIndex + 1 < SplinePointCount
			? CropBoundarySpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex + 1)
			: TotalSplineLength;
		const float SectionLength = SectionEndDistance - SectionStartDistance;
		const int32 SubdivisionCount = FMath::Max(1, FMath::CeilToInt(SectionLength / SafeSampleLength));

		for (int32 SubdivisionIndex = 0; SubdivisionIndex < SubdivisionCount; ++SubdivisionIndex)
		{
			const float Alpha = static_cast<float>(SubdivisionIndex) / static_cast<float>(SubdivisionCount);
			const FVector SampleLocation = CropBoundarySpline->GetLocationAtDistanceAlongSpline(
				FMath::Lerp(SectionStartDistance, SectionEndDistance, Alpha),
				ESplineCoordinateSpace::Local);
			OutBoundaryPoints.Emplace(SampleLocation.X, SampleLocation.Y);
		}
	}
}

bool ALxProceduralCropActor::IsPointInsideBoundary(
	const FVector2D& Point,
	const TArray<FVector2D>& BoundaryPoints) const
{
	bool bIsInside = false;
	for (int32 CurrentIndex = 0, PreviousIndex = BoundaryPoints.Num() - 1;
		CurrentIndex < BoundaryPoints.Num(); PreviousIndex = CurrentIndex++)
	{
		const FVector2D& Current = BoundaryPoints[CurrentIndex];
		const FVector2D& Previous = BoundaryPoints[PreviousIndex];
		const FVector2D Edge = Current - Previous;
		const FVector2D ToPoint = Point - Previous;
		const double CrossProduct = static_cast<double>(Edge.X) * ToPoint.Y - static_cast<double>(Edge.Y) * ToPoint.X;
		const double EdgeLengthSquared = static_cast<double>(Edge.SizeSquared());
		if (EdgeLengthSquared > UE_SMALL_NUMBER && FMath::Abs(CrossProduct) <= 0.01 * FMath::Sqrt(EdgeLengthSquared))
		{
			const double Projection = static_cast<double>(FVector2D::DotProduct(ToPoint, Edge));
			if (Projection >= 0.0 && Projection <= EdgeLengthSquared)
			{
				return true;
			}
		}

		const bool bCrossesHorizontalRay = (Current.Y > Point.Y) != (Previous.Y > Point.Y);
		if (bCrossesHorizontalRay)
		{
			const double IntersectionX = static_cast<double>(Previous.X)
				+ static_cast<double>(Point.Y - Previous.Y) * static_cast<double>(Current.X - Previous.X)
				/ static_cast<double>(Current.Y - Previous.Y);
			if (Point.X < IntersectionX)
			{
				bIsInside = !bIsInside;
			}
		}
	}
	return bIsInside;
}
