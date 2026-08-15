// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxRingSurfaceActor.h"

#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

ALxRingSurfaceActor::ALxRingSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RingSurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RingSurfaceMesh"));
	SetRootComponent(RingSurfaceMesh);
	RingSurfaceMesh->SetMobility(EComponentMobility::Movable);
	RingSurfaceMesh->SetCastShadow(true);
	RingSurfaceMesh->bCastDynamicShadow = true;
	RingSurfaceMesh->bCastStaticShadow = false;
	RingSurfaceMesh->bCastContactShadow = true;
	RingSurfaceMesh->bAffectDynamicIndirectLighting = true;
	RingSurfaceMesh->bAffectDistanceFieldLighting = true;
	RingSurfaceMesh->bEditableWhenInherited = false;
	RingSurfaceMesh->bUseAsyncCooking = false;
	RingSurfaceMesh->bUseComplexAsSimpleCollision = false;
	RingSurfaceMesh->CanCharacterStepUpOn = ECB_Yes;
}

void ALxRingSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RebuildRingSurface();
}

void ALxRingSurfaceActor::RebuildRingSurface()
{
	ClearGeneratedRingSurface();
	if (!IsValid(RingSurfaceMesh))
	{
		return;
	}

	const float SafeOuterDiameter = FMath::Max(OuterDiameter, 2.0f);
	const float SafeOuterRadius = SafeOuterDiameter * 0.5f;
	const float SafeInnerDiameter = FMath::Clamp(InnerDiameter, 1.0f, SafeOuterDiameter - 1.0f);
	const float SafeInnerRadius = SafeInnerDiameter * 0.5f;
	const float SafeThickness = FMath::Max(SurfaceThickness, 0.1f);
	const int32 SafeSegmentCount = FMath::Clamp(CircleSegmentCount, 8, 512);
	const float SafeMaterialDensity = FMath::Max(MaterialDensity, 0.001f);
	const FVector ActorScale = GetActorScale3D().GetAbs();
	const float RadialScale = (ActorScale.X + ActorScale.Y) * 0.5f;
	const float MiddleRadius = (SafeOuterRadius + SafeInnerRadius) * 0.5f;
	const float MiddleCircumference = 2.0f * PI * MiddleRadius * RadialScale;
	const float RingWidth = (SafeOuterRadius - SafeInnerRadius) * RadialScale;
	const float TextureStartAlpha = TextureStartAngle / 360.0f;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(SafeSegmentCount * 16);
	Triangles.Reserve(SafeSegmentCount * 24);

	/** 向网格数据中添加一个具有独立法线、UV 和切线的顶点。 */
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

	/** 按程序化网格正面绕序添加一个四边形。 */
	const auto AddQuad = [&Triangles](const int32 A, const int32 B, const int32 C, const int32 D)
	{
		Triangles.Append({A, B, C, C, B, D});
	};

	for (int32 SegmentIndex = 0; SegmentIndex < SafeSegmentCount; ++SegmentIndex)
	{
		const float StartAlpha = static_cast<float>(SegmentIndex) / static_cast<float>(SafeSegmentCount);
		const float EndAlpha = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SafeSegmentCount);
		const float StartAngle = StartAlpha * 2.0f * PI;
		const float EndAngle = EndAlpha * 2.0f * PI;
		const FVector StartDirection(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0.0f);
		const FVector EndDirection(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0.0f);
		const FVector StartTangent(-StartDirection.Y, StartDirection.X, 0.0f);
		const FVector EndTangent(-EndDirection.Y, EndDirection.X, 0.0f);

		const FVector OuterStartTop = StartDirection * SafeOuterRadius;
		const FVector OuterEndTop = EndDirection * SafeOuterRadius;
		const FVector InnerStartTop = StartDirection * SafeInnerRadius;
		const FVector InnerEndTop = EndDirection * SafeInnerRadius;
		const FVector BottomOffset(0.0f, 0.0f, -SafeThickness);
		const FVector OuterStartBottom = OuterStartTop + BottomOffset;
		const FVector OuterEndBottom = OuterEndTop + BottomOffset;
		const FVector InnerStartBottom = InnerStartTop + BottomOffset;
		const FVector InnerEndBottom = InnerEndTop + BottomOffset;

		// 顶面将角度映射到 U、由外向内的半径映射到 V，普通方形地砖因此会形成同心圆带。
		const float StartU = (StartAlpha + TextureStartAlpha) * MiddleCircumference / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const float EndU = (EndAlpha + TextureStartAlpha) * MiddleCircumference / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const float OuterV = TextureOffset.Y;
		const float InnerV = RingWidth / 100.0f * SafeMaterialDensity + TextureOffset.Y;
		const int32 TopOuterStartIndex = AddVertex(OuterStartTop, FVector::UpVector, FVector2D(StartU, OuterV), StartTangent);
		const int32 TopInnerStartIndex = AddVertex(InnerStartTop, FVector::UpVector, FVector2D(StartU, InnerV), StartTangent);
		const int32 TopOuterEndIndex = AddVertex(OuterEndTop, FVector::UpVector, FVector2D(EndU, OuterV), EndTangent);
		const int32 TopInnerEndIndex = AddVertex(InnerEndTop, FVector::UpVector, FVector2D(EndU, InnerV), EndTangent);
		AddQuad(TopOuterStartIndex, TopInnerStartIndex, TopOuterEndIndex, TopInnerEndIndex);

		// 底面沿用同心圆 UV，但反转径向 V，保持向下表面的切线空间方向一致。
		const int32 BottomOuterStartIndex = AddVertex(OuterStartBottom, FVector::DownVector, FVector2D(StartU, InnerV), StartTangent);
		const int32 BottomOuterEndIndex = AddVertex(OuterEndBottom, FVector::DownVector, FVector2D(EndU, InnerV), EndTangent);
		const int32 BottomInnerStartIndex = AddVertex(InnerStartBottom, FVector::DownVector, FVector2D(StartU, OuterV), StartTangent);
		const int32 BottomInnerEndIndex = AddVertex(InnerEndBottom, FVector::DownVector, FVector2D(EndU, OuterV), EndTangent);
		AddQuad(BottomOuterStartIndex, BottomOuterEndIndex, BottomInnerStartIndex, BottomInnerEndIndex);

		const float OuterStartSideU = (StartAlpha + TextureStartAlpha) * 2.0f * PI * SafeOuterRadius * RadialScale / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const float OuterEndSideU = (EndAlpha + TextureStartAlpha) * 2.0f * PI * SafeOuterRadius * RadialScale / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const float SideHeightV = SafeThickness * ActorScale.Z / 100.0f * SafeMaterialDensity + TextureOffset.Y;
		const int32 OuterStartBottomIndex = AddVertex(OuterStartBottom, StartDirection, FVector2D(OuterStartSideU, TextureOffset.Y), StartTangent);
		const int32 OuterStartTopIndex = AddVertex(OuterStartTop, StartDirection, FVector2D(OuterStartSideU, SideHeightV), StartTangent);
		const int32 OuterEndBottomIndex = AddVertex(OuterEndBottom, EndDirection, FVector2D(OuterEndSideU, TextureOffset.Y), EndTangent);
		const int32 OuterEndTopIndex = AddVertex(OuterEndTop, EndDirection, FVector2D(OuterEndSideU, SideHeightV), EndTangent);
		AddQuad(OuterStartBottomIndex, OuterStartTopIndex, OuterEndBottomIndex, OuterEndTopIndex);

		const float InnerStartSideU = (StartAlpha + TextureStartAlpha) * 2.0f * PI * SafeInnerRadius * RadialScale / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const float InnerEndSideU = (EndAlpha + TextureStartAlpha) * 2.0f * PI * SafeInnerRadius * RadialScale / 100.0f * SafeMaterialDensity + TextureOffset.X;
		const int32 InnerStartBottomIndex = AddVertex(InnerStartBottom, -StartDirection, FVector2D(InnerStartSideU, SideHeightV), StartTangent);
		const int32 InnerEndBottomIndex = AddVertex(InnerEndBottom, -EndDirection, FVector2D(InnerEndSideU, SideHeightV), EndTangent);
		const int32 InnerStartTopIndex = AddVertex(InnerStartTop, -StartDirection, FVector2D(InnerStartSideU, TextureOffset.Y), StartTangent);
		const int32 InnerEndTopIndex = AddVertex(InnerEndTop, -EndDirection, FVector2D(InnerEndSideU, TextureOffset.Y), EndTangent);
		AddQuad(InnerStartBottomIndex, InnerEndBottomIndex, InnerStartTopIndex, InnerEndTopIndex);
	}

	RingSurfaceMesh->SetMobility(EComponentMobility::Movable);
	RingSurfaceMesh->SetCastShadow(bCastShadow);
	RingSurfaceMesh->bCastDynamicShadow = bCastShadow;
	RingSurfaceMesh->bCastStaticShadow = false;
	RingSurfaceMesh->bCastContactShadow = bCastShadow;
	RingSurfaceMesh->bVisibleInRayTracing = true;
	RingSurfaceMesh->SetCollisionEnabled(bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision)
	{
		RingSurfaceMesh->SetCollisionProfileName(CollisionProfileName);
		RingSurfaceMesh->SetCollisionObjectType(ECC_WorldStatic);
	}
	RingSurfaceMesh->CreateMeshSection_LinearColor(
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
		BuildStableCollision(SafeOuterRadius, SafeInnerRadius, SafeThickness, SafeSegmentCount);
	}
	RingSurfaceMesh->SetMaterial(0, RingSurfaceMaterial);
}

void ALxRingSurfaceActor::ClearGeneratedRingSurface()
{
	if (IsValid(RingSurfaceMesh))
	{
		RingSurfaceMesh->ClearAllMeshSections();
		RingSurfaceMesh->ClearCollisionConvexMeshes();
	}
}

void ALxRingSurfaceActor::BuildStableCollision(
	const float OuterRadius,
	const float InnerRadius,
	const float Thickness,
	const int32 SegmentCount)
{
	if (!IsValid(RingSurfaceMesh) || SegmentCount < 3)
	{
		return;
	}

	TArray<TArray<FVector>> CollisionConvexMeshes;
	CollisionConvexMeshes.Reserve(SegmentCount);
	for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
	{
		const float StartAngle = 2.0f * PI * static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
		const float EndAngle = 2.0f * PI * static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
		const FVector StartDirection(FMath::Cos(StartAngle), FMath::Sin(StartAngle), 0.0f);
		const FVector EndDirection(FMath::Cos(EndAngle), FMath::Sin(EndAngle), 0.0f);
		const FVector BottomOffset(0.0f, 0.0f, -Thickness);

		// 单个圆周分段是一个闭合凸棱柱，组合后能准确保留中心孔洞。
		TArray<FVector> ConvexPrismVertices;
		ConvexPrismVertices.Reserve(8);
		ConvexPrismVertices.Add(StartDirection * InnerRadius);
		ConvexPrismVertices.Add(StartDirection * OuterRadius);
		ConvexPrismVertices.Add(EndDirection * OuterRadius);
		ConvexPrismVertices.Add(EndDirection * InnerRadius);
		ConvexPrismVertices.Add(StartDirection * InnerRadius + BottomOffset);
		ConvexPrismVertices.Add(StartDirection * OuterRadius + BottomOffset);
		ConvexPrismVertices.Add(EndDirection * OuterRadius + BottomOffset);
		ConvexPrismVertices.Add(EndDirection * InnerRadius + BottomOffset);
		CollisionConvexMeshes.Add(MoveTemp(ConvexPrismVertices));
	}

	RingSurfaceMesh->SetCollisionConvexMeshes(CollisionConvexMeshes);
}
