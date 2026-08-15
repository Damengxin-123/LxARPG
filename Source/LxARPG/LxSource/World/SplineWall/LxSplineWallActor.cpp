// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxSplineWallActor.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace LxSplineWallPrivate
{
	/** 标记由样条围墙系统自动生成的组件，便于构造脚本重跑时可靠清理。 */
	const FName GeneratedWallComponentTag(TEXT("Lx.GeneratedSplineWall"));
}

ALxSplineWallActor::ALxSplineWallActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WallSpline = CreateDefaultSubobject<USplineComponent>(TEXT("WallSpline"));
	SetRootComponent(WallSpline);
	WallSpline->SetMobility(EComponentMobility::Static);
	WallSpline->ClearSplinePoints(false);
	WallSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	WallSpline->AddSplinePoint(FVector(500.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, false);
	WallSpline->SetSplinePointType(0, ESplinePointType::Curve, false);
	WallSpline->SetSplinePointType(1, ESplinePointType::Curve, false);
	WallSpline->UpdateSpline();

	// 默认使用引擎立方体，使新建蓝图无需额外配置即可显示基础围墙。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultWallMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultWallMesh.Succeeded())
	{
		WallMesh = DefaultWallMesh.Object;
	}
}

void ALxSplineWallActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildWall();
}

void ALxSplineWallActor::RebuildWall()
{
	ClearGeneratedWall();

	if (!IsValid(WallSpline) || !IsValid(WallMesh) || WallSpline->GetNumberOfSplinePoints() < 2)
	{
		return;
	}

	const FBox MeshBounds = WallMesh->GetBoundingBox();
	const FVector MeshSize = MeshBounds.GetSize();
	if (MeshSize.Y <= UE_SMALL_NUMBER || MeshSize.Z <= UE_SMALL_NUMBER)
	{
		return;
	}

	const FVector2D WallScale(
		FMath::Max(WallThickness, 1.0f) / MeshSize.Y,
		FMath::Max(WallHeight, 1.0f) / MeshSize.Z);
	const float MeshBottomOffset = bAlignMeshBottomToSpline ? -MeshBounds.Min.Z * WallScale.Y : 0.0f;
	const float SafeMaxSegmentLength = FMath::Max(MaxSegmentLength, 10.0f);
	const int32 SplinePointCount = WallSpline->GetNumberOfSplinePoints();
	const int32 SplineSectionCount = WallSpline->IsClosedLoop() ? SplinePointCount : SplinePointCount - 1;
	const float TotalSplineLength = WallSpline->GetSplineLength();

	for (int32 SplineSectionIndex = 0; SplineSectionIndex < SplineSectionCount; ++SplineSectionIndex)
	{
		const float SectionStartDistance = WallSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex);
		const float SectionEndDistance = SplineSectionIndex + 1 < SplinePointCount
			? WallSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex + 1)
			: TotalSplineLength;
		const float SectionLength = SectionEndDistance - SectionStartDistance;
		const int32 SubdivisionCount = FMath::Max(1, FMath::CeilToInt(SectionLength / SafeMaxSegmentLength));

		for (int32 SubdivisionIndex = 0; SubdivisionIndex < SubdivisionCount; ++SubdivisionIndex)
		{
			const float StartAlpha = static_cast<float>(SubdivisionIndex) / static_cast<float>(SubdivisionCount);
			const float EndAlpha = static_cast<float>(SubdivisionIndex + 1) / static_cast<float>(SubdivisionCount);
			CreateWallSegment(
				FMath::Lerp(SectionStartDistance, SectionEndDistance, StartAlpha),
				FMath::Lerp(SectionStartDistance, SectionEndDistance, EndAlpha),
				WallScale,
				MeshBottomOffset);
		}
	}
}

void ALxSplineWallActor::ClearGeneratedWall()
{
	TArray<USplineMeshComponent*> ExistingSplineMeshComponents;
	GetComponents(ExistingSplineMeshComponents);

	for (USplineMeshComponent* WallSegment : ExistingSplineMeshComponents)
	{
		if (!IsValid(WallSegment))
		{
			continue;
		}

		const bool bIsTaggedGeneratedSegment = WallSegment->ComponentHasTag(LxSplineWallPrivate::GeneratedWallComponentTag);
		const bool bIsTrackedGeneratedSegment = GeneratedWallSegments.Contains(WallSegment);
		const bool bIsLegacyGeneratedSegment = WallSegment->GetAttachParent() == WallSpline
			&& (WallSegment->CreationMethod == EComponentCreationMethod::Instance
				|| WallSegment->CreationMethod == EComponentCreationMethod::UserConstructionScript);
		if (bIsTaggedGeneratedSegment || bIsTrackedGeneratedSegment || bIsLegacyGeneratedSegment)
		{
			RemoveInstanceComponent(WallSegment);
			WallSegment->DestroyComponent();
		}
	}

	GeneratedWallSegments.Reset();
}

void ALxSplineWallActor::CreateWallSegment(
	const float StartDistance,
	const float EndDistance,
	const FVector2D& WallScale,
	const float MeshBottomOffset)
{
	const float SegmentLength = EndDistance - StartDistance;
	if (SegmentLength <= UE_SMALL_NUMBER)
	{
		return;
	}

	USplineMeshComponent* WallSegment = NewObject<USplineMeshComponent>(
		this,
		NAME_None,
		RF_Transactional | RF_DuplicateTransient);
	if (!IsValid(WallSegment))
	{
		return;
	}

	// 将墙段登记为构造脚本生成组件，禁止编辑器把它当作可手动复制的实例组件。
	RemoveOwnedComponent(WallSegment);
	WallSegment->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	WallSegment->bEditableWhenInherited = false;
	AddOwnedComponent(WallSegment);
	WallSegment->ComponentTags.Add(LxSplineWallPrivate::GeneratedWallComponentTag);
	WallSegment->SetMobility(EComponentMobility::Static);
	WallSegment->SetStaticMesh(WallMesh);
	WallSegment->SetForwardAxis(ESplineMeshAxis::X, false);
	WallSegment->SetSplineUpDir(FVector::UpVector, false);
	WallSegment->SetCastShadow(bCastShadow);
	WallSegment->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision)
	{
		WallSegment->SetCollisionProfileName(CollisionProfileName);
	}

	const FVector HeightOffset(0.0f, 0.0f, MeshBottomOffset + VerticalOffset);
	const FVector StartPosition = WallSpline->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local) + HeightOffset;
	const FVector EndPosition = WallSpline->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local) + HeightOffset;
	const FVector StartTangent = WallSpline->GetDirectionAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local) * SegmentLength;
	const FVector EndTangent = WallSpline->GetDirectionAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local) * SegmentLength;

	WallSegment->SetStartAndEnd(StartPosition, StartTangent, EndPosition, EndTangent, false);
	WallSegment->SetStartScale(WallScale, false);
	WallSegment->SetEndScale(WallScale, false);
	WallSegment->SetStartRoll(FMath::DegreesToRadians(
		WallSpline->GetRotationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::Local).Roll), false);
	WallSegment->SetEndRoll(FMath::DegreesToRadians(
		WallSpline->GetRotationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::Local).Roll), false);
	WallSegment->UpdateMesh();

	WallSegment->AttachToComponent(WallSpline, FAttachmentTransformRules::KeepRelativeTransform);
	WallSegment->RegisterComponent();
	ApplyWallMaterial(WallSegment);
	GeneratedWallSegments.Add(WallSegment);
}

void ALxSplineWallActor::ApplyWallMaterial(USplineMeshComponent* WallSegment) const
{
	if (!IsValid(WallSegment))
	{
		return;
	}

	UMaterialInterface* BaseMaterial = WallMaterial ? WallMaterial.Get() : WallSegment->GetMaterial(0);
	if (!IsValid(BaseMaterial))
	{
		return;
	}

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, WallSegment);
	if (!IsValid(DynamicMaterial))
	{
		return;
	}

	DynamicMaterial->SetScalarParameterValue(MaterialDensityParameterName, FMath::Max(MaterialDensity, 0.01f));
	WallSegment->SetMaterial(0, DynamicMaterial);
}
