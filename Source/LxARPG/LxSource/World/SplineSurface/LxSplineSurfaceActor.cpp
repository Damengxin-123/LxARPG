// Copyright Epic Games, Inc. All Rights Reserved.

#include "LxSplineSurfaceActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshRenderComponent.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#include "Misc/ScopeExit.h"

#include "Components/SplineComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Serialization/CustomVersion.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/UnrealType.h"

namespace
{
	/** 保留旧厘米属性，避免旧资产未序列化默认值时出现重复或错误换算。 */
	const FGuid SurfaceVersionGuid(0x4F7D5E11, 0x103A4E27, 0xAED81F69, 0x709BAD21);
	/** 第一版使用米制编辑参数及本地输入缓存。 */
	FCustomVersionRegistration SurfaceVersion(SurfaceVersionGuid, 1, TEXT("LxSplineSurface"));
	/** 公开米制数值统一量化到两位小数，引擎几何仍使用厘米双精度坐标。 */
	double SurfaceMeters(double Value) { return FMath::RoundToDouble(Value * 100.0) / 100.0; }
}

ALxSplineSurfaceActor::ALxSplineSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = false;
#if WITH_EDITORONLY_DATA
	bRunConstructionScriptOnDrag = false;
#endif
	// 与现有海港建筑共享干净的浅灰石板风格，单次贴图周期覆盖两米。
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMaterial(
		TEXT("/Game/项目内容/美术资产/海港城市/圆形建筑/材质/大块浅灰石板地面"));
	SurfaceMaterial = DefaultMaterial.Object;
	MaterialDensity = 0.5f;

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

	SurfaceMesh = CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("SurfaceMesh"));
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
 OptimizedMesh=CreateDefaultSubobject<ULxGeneratedMeshRenderComponent>(TEXT("OptimizedMesh"));
 OptimizedMesh->SetupAttachment(SurfaceMesh);
}

void ALxSplineSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (IsTemplate()) { return; }
	SynchronizeLegacyParameters();
	const bool bNeedFullMesh = bSurfacePreview && !LxGeneratedMeshBuild::IsPreview(this);
	if (!bNeedFullMesh && !LocalSurfaceInputs.IsEmpty()
		&& LocalSurfaceInputs == LxGeneratedMeshBuild::CaptureInputs(this, false, false))
	{
		// 整体移动与旋转只改变组件变换；完整模型无需降级为预览。
		if (OptimizedMesh) { OptimizedMesh->AcceptUnchangedSource(); }
		return;
	}
	RebuildSurface();
}

void ALxSplineSurfaceActor::Serialize(FArchive& Ar)
{
	if (Ar.IsLoading()) { bMigrateLegacyUnits = Ar.CustomVer(SurfaceVersionGuid) < 1; }
	Ar.UsingCustomVersion(SurfaceVersionGuid);
	Super::Serialize(Ar);
}

void ALxSplineSurfaceActor::PostLoad()
{
	Super::PostLoad();
	if (bMigrateLegacyUnits)
	{
		HeightOffsetMeters = HeightOffset / 100.0f;
		GeneratedSurfaceLocationMeters = GeneratedSurfaceLocation / 100.0;
		EdgeSampleSpacingMeters = MaxEdgeSampleLength / 100.0f;
		CollisionThicknessMeters = CollisionThickness / 100.0f;
		LocalSurfaceInputs.Reset();
		bMigrateLegacyUnits = false;
		AppliedLegacyParameters = {HeightOffset, GeneratedSurfaceLocation.X, GeneratedSurfaceLocation.Y,
			GeneratedSurfaceLocation.Z, MaxEdgeSampleLength, CollisionThickness};
	}
}

void ALxSplineSurfaceActor::SynchronizeLegacyParameters()
{
	const TArray<double> Values = {HeightOffset, GeneratedSurfaceLocation.X, GeneratedSurfaceLocation.Y,
		GeneratedSurfaceLocation.Z, MaxEdgeSampleLength, CollisionThickness};
	if (AppliedLegacyParameters.Num() == Values.Num())
	{
		if (Values[0] != AppliedLegacyParameters[0]) { HeightOffsetMeters = Values[0] / 100.0; }
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			if (Values[Axis+1] != AppliedLegacyParameters[Axis+1]) { GeneratedSurfaceLocationMeters[Axis] = Values[Axis+1] / 100.0; }
		}
		if (Values[4] != AppliedLegacyParameters[4]) { EdgeSampleSpacingMeters = Values[4] / 100.0; }
		if (Values[5] != AppliedLegacyParameters[5]) { CollisionThicknessMeters = Values[5] / 100.0; }
	}
	// 旧读取节点继续得到厘米值；只在旧设置节点明确改动时才反向应用到米制参数。
	HeightOffset = HeightOffsetMeters * 100.0f;
	GeneratedSurfaceLocation = GeneratedSurfaceLocationMeters * 100.0;
	MaxEdgeSampleLength = EdgeSampleSpacingMeters * 100.0f;
	CollisionThickness = CollisionThicknessMeters * 100.0f;
	AppliedLegacyParameters = {HeightOffset, GeneratedSurfaceLocation.X, GeneratedSurfaceLocation.Y,
		GeneratedSurfaceLocation.Z, MaxEdgeSampleLength, CollisionThickness};
}

void ALxSplineSurfaceActor::BeginPlay()
{
	// 原生演员早于组件BeginPlay提交完整碰撞，游戏世界不采用简化采样。
	FinalizeSurface();
	Super::BeginPlay();
}

void ALxSplineSurfaceActor::PreSave(FObjectPreSaveContext SaveContext)
{
	if (!IsTemplate()) { FinalizeSurface(); }
	Super::PreSave(SaveContext);
}

void ALxSplineSurfaceActor::FinalizeSurface()
{
	if (IsTemplate()) { return; }
	LxGeneratedMeshBuild::EndPreview(this);
	OnConstruction(GetActorTransform());
	if (OptimizedMesh) { OptimizedMesh->FinalizeMesh(); }
}

#if WITH_EDITOR
void ALxSplineSurfaceActor::PreEditChange(FProperty* PropertyAboutToChange)
{
	if (!IsTemplate() && PropertyAboutToChange && PropertyAboutToChange->GetOwnerStruct() == StaticClass()
		&& !CastField<FObjectPropertyBase>(PropertyAboutToChange))
	{
		UObject::PreEditChange(PropertyAboutToChange);
		return;
	}
	Super::PreEditChange(PropertyAboutToChange);
}

void ALxSplineSurfaceActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	LxGeneratedMeshBuild::RequestPreview(this);
	if (!IsTemplate() && (Event.ChangeType & EPropertyChangeType::Interactive) != 0)
	{
		UObject::PostEditChangeProperty(Event);
		return;
	}
	Super::PostEditChangeProperty(Event);
}

void ALxSplineSurfaceActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
}

void ALxSplineSurfaceActor::PostEditUndo()
{
	LxGeneratedMeshBuild::RequestPreview(this);
	Super::PostEditUndo();
}
#endif

void ALxSplineSurfaceActor::RebuildSurface()
{
 SynchronizeLegacyParameters();
 // 包括无效参数和空几何在内的所有出口都同步渲染缓存。
 const double BuildStart = FPlatformTime::Seconds();
 double OutlineEnd = BuildStart, MeshEnd = BuildStart;
 ON_SCOPE_EXIT
 {
  LocalSurfaceInputs = LxGeneratedMeshBuild::CaptureInputs(this, false, false);
  if (OptimizedMesh) { OptimizedMesh->Synchronize(SurfaceMesh); }
  const double BuildEnd = FPlatformTime::Seconds();
  if (!bSurfacePreview)
  {
   UE_LOG(LogLxGeneratedMeshSave, Display, TEXT("样条地面 %s 顶点=%d 三角形=%d 轮廓及三角化=%.2f秒 网格=%.2f秒 碰撞及同步=%.2f秒 总计=%.2f秒 说明=%s"),
    *GetPathName(), GeneratedVertexCount, GeneratedTriangleCount, OutlineEnd-BuildStart, MeshEnd-OutlineEnd,
    BuildEnd-MeshEnd, BuildEnd-BuildStart, *GenerationDiagnostic);
  }
 };
	++GeometryBuildCount;
	GenerationDiagnostic.Reset();
	bSurfacePreview = LxGeneratedMeshBuild::IsPreview(this);
	const double Values[] = {HeightOffsetMeters, EdgeSampleSpacingMeters, PreviewSampleSpacingMeters, CollisionThicknessMeters,
		MaterialDensity, TextureRotation, TextureOffset.X, TextureOffset.Y, GeneratedSurfaceLocationMeters.X,
		GeneratedSurfaceLocationMeters.Y, GeneratedSurfaceLocationMeters.Z, GeneratedSurfaceScale.X,
		GeneratedSurfaceScale.Y, GeneratedSurfaceScale.Z, GeneratedSurfaceRotation.Pitch, GeneratedSurfaceRotation.Yaw, GeneratedSurfaceRotation.Roll};
	for (double Value : Values)
	{
		if (!FMath::IsFinite(Value))
		{
			ClearGeneratedSurface(); GenerationDiagnostic = TEXT("地面参数含无效数值，未生成网格。"); return;
		}
	}
	HeightOffsetMeters = SurfaceMeters(HeightOffsetMeters);
	GeneratedSurfaceLocationMeters = FVector(SurfaceMeters(GeneratedSurfaceLocationMeters.X),
		SurfaceMeters(GeneratedSurfaceLocationMeters.Y), SurfaceMeters(GeneratedSurfaceLocationMeters.Z));
	EdgeSampleSpacingMeters = FMath::Max(0.05, SurfaceMeters(EdgeSampleSpacingMeters));
	PreviewSampleSpacingMeters = FMath::Max(0.05, SurfaceMeters(PreviewSampleSpacingMeters));
	CollisionThicknessMeters = FMath::Max(0.01, SurfaceMeters(CollisionThicknessMeters));
	SynchronizeLegacyParameters();
	if (ControlPointCount > 2048 || (SurfaceSpline && SurfaceSpline->GetNumberOfSplinePoints() > 2048))
	{
		ClearGeneratedSurface(); GenerationDiagnostic = TEXT("控制点超过2048点，请分片铺设地面。"); return;
	}
	SynchronizeSplineControlPointCount();
	if (!bPreserveSplineHeights)
	{
		ConstrainSplinePointsToActorHeight();
	}
	ClearGeneratedSurface();

	if (!IsValid(SurfaceSpline) || !IsValid(SurfaceMesh) || SurfaceSpline->GetNumberOfSplinePoints() < 3)
	{
		return;
	}
	for (const auto& Point : SurfaceSpline->SplineCurves.Position.Points)
	{
		if (Point.OutVal.ContainsNaN() || Point.ArriveTangent.ContainsNaN() || Point.LeaveTangent.ContainsNaN())
		{
			GenerationDiagnostic = TEXT("样条轮廓含无效坐标或切线。"); return;
		}
	}

	// 已放入关卡的旧实例可能保存了静态移动性；每次重建都强制切换到动态光照路径。
	SurfaceMesh->SetMobility(EComponentMobility::Movable);

	if (!SurfaceSpline->IsClosedLoop())
	{
		SurfaceSpline->SetClosedLoop(true, false);
		SurfaceSpline->UpdateSpline();
	}

	TArray<FVector> Vertices;
	if (!BuildOutlineVertices(Vertices)) { return; }
	SanitizeOutlineVertices(Vertices);
	if (!IsOutlineSimple(Vertices))
	{
		GenerationDiagnostic = TEXT("轮廓自交、重叠或折返，请调整样条控制点。"); return;
	}

	TArray<int32> Triangles;
	if (!TriangulatePolygon(Vertices, Triangles))
	{
		GenerationDiagnostic = TEXT("轮廓无法三角化，请检查重复控制点或零面积边缘。");
		return;
	}
	OutlineEnd = FPlatformTime::Seconds();
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
	const FTransform GeneratedSurfaceTransform(GeneratedSurfaceRotation, GeneratedSurfaceLocationMeters * 100.0, SafeSurfaceScale);
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
	SurfaceMesh->SetCollisionEnabled(bEnableCollision && !bSurfacePreview ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	if (bEnableCollision && !bSurfacePreview)
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
	MeshEnd = FPlatformTime::Seconds();
	if (bEnableCollision && !bSurfacePreview)
	{
		BuildStableCollision(Vertices, Triangles);
	}

	SurfaceMesh->SetMaterial(0, SurfaceMaterial);
	GeneratedVertexCount = Vertices.Num();
	GeneratedTriangleCount = Triangles.Num() / 3;
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
			if (!bPreserveSplineHeights) { SampleLocation.Z = 0.0f; }
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
	GeneratedVertexCount = GeneratedTriangleCount = 0;
	if (IsValid(SurfaceMesh))
	{
		SurfaceMesh->ClearAllMeshSections();
		SurfaceMesh->ClearCollisionConvexMeshes();
	}
 if (OptimizedMesh) { OptimizedMesh->MarkSourceDirty(); OptimizedMesh->SetVisibility(false); }
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

bool ALxSplineSurfaceActor::BuildOutlineVertices(TArray<FVector>& OutVertices)
{
	OutVertices.Reset();

	const double SafeSampleLength = 100.0 * (bSurfacePreview
		? FMath::Max(EdgeSampleSpacingMeters, PreviewSampleSpacingMeters) : EdgeSampleSpacingMeters);
	const int32 SplinePointCount = SurfaceSpline->GetNumberOfSplinePoints();
	const float TotalSplineLength = SurfaceSpline->GetSplineLength();
	const int32 Budget = bSurfacePreview ? FMath::Max(256, SplinePointCount) : 2048;
	TArray<int32> Subdivisions;
	int32 RequestedSamples = 0;
	for (int32 Section = 0; Section < SplinePointCount; ++Section)
	{
		const double Start = SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(Section);
		const double End = Section + 1 < SplinePointCount
			? SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(Section + 1) : TotalSplineLength;
		if (!FMath::IsFinite(End - Start) || End < Start)
		{
			GenerationDiagnostic = TEXT("轮廓长度无效，请检查样条坐标和切线。"); return false;
		}
		// 直线段直接连接原控制点，避免采样后再逐点删除共线顶点。
		const double Wanted = SurfaceSpline->GetSplinePointType(Section) == ESplinePointType::Linear
			? 1.0 : FMath::Max(1.0, FMath::CeilToDouble((End - Start) / SafeSampleLength));
		if (!bSurfacePreview && Wanted > Budget)
		{
			GenerationDiagnostic = TEXT("完整轮廓超过2048点预算，请增大采样间距或分片铺设。"); return false;
		}
		const int32 Count = static_cast<int32>(FMath::Min(Wanted, static_cast<double>(Budget)));
		Subdivisions.Add(Count); RequestedSamples += Count;
	}
	if (RequestedSamples > Budget)
	{
		if (!bSurfacePreview)
		{
			GenerationDiagnostic = TEXT("完整轮廓超过2048点预算，请增大采样间距或分片铺设。"); return false;
		}
		// 按弧长分配剩余采样预算，所有原控制点始终保留，不截断多边形。
		const double Ratio = static_cast<double>(Budget - SplinePointCount) / (RequestedSamples - SplinePointCount);
		for (int32& Count : Subdivisions) { Count = 1 + FMath::FloorToInt((Count - 1) * Ratio); }
	}
	for (int32 SplineSectionIndex = 0; SplineSectionIndex < SplinePointCount; ++SplineSectionIndex)
	{
		const float SectionStartDistance = SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex);
		const float SectionEndDistance = SplineSectionIndex + 1 < SplinePointCount
			? SurfaceSpline->GetDistanceAlongSplineAtSplinePoint(SplineSectionIndex + 1)
			: TotalSplineLength;
		const int32 SubdivisionCount = Subdivisions[SplineSectionIndex];

		for (int32 SubdivisionIndex = 0; SubdivisionIndex < SubdivisionCount; ++SubdivisionIndex)
		{
			const float Alpha = static_cast<float>(SubdivisionIndex) / static_cast<float>(SubdivisionCount);
			const float SampleDistance = FMath::Lerp(SectionStartDistance, SectionEndDistance, Alpha);
			FVector SamplePosition = SubdivisionIndex == 0
				? SurfaceSpline->GetLocationAtSplinePoint(SplineSectionIndex, ESplineCoordinateSpace::Local)
				: SurfaceSpline->GetLocationAtDistanceAlongSpline(SampleDistance, ESplineCoordinateSpace::Local);
			SamplePosition.Z = (bPreserveSplineHeights ? SamplePosition.Z : 0.0f) + HeightOffsetMeters * 100.0f;
			OutVertices.Add(SamplePosition);
		}
	}
	return true;
}

bool ALxSplineSurfaceActor::IsOutlineSimple(const TArray<FVector>& Vertices) const
{
	if (Vertices.Num() < 3) { return false; }
	/** 判断点与有向边的位置关系，采用厘米坐标下的面积容差。 */
	const auto Side = [](const FVector2D& A, const FVector2D& B, const FVector2D& P)
	{
		const double Cross = (B.X-A.X)*(P.Y-A.Y)-(B.Y-A.Y)*(P.X-A.X);
		return FMath::Abs(Cross) <= 0.001 ? 0 : (Cross > 0 ? 1 : -1);
	};
	/** 共线时仅把落在边界范围内的点视为重叠。 */
	const auto OnEdge = [](const FVector2D& A, const FVector2D& B, const FVector2D& P)
	{
		return P.X >= FMath::Min(A.X,B.X)-0.001 && P.X <= FMath::Max(A.X,B.X)+0.001
			&& P.Y >= FMath::Min(A.Y,B.Y)-0.001 && P.Y <= FMath::Max(A.Y,B.Y)+0.001;
	};
	for (int32 I = 0; I < Vertices.Num(); ++I)
	{
		const FVector2D A(Vertices[I]), B(Vertices[(I+1)%Vertices.Num()]);
		for (int32 J = I+2; J < Vertices.Num(); ++J)
		{
			if (I == 0 && J == Vertices.Num()-1) { continue; }
			const FVector2D C(Vertices[J]), D(Vertices[(J+1)%Vertices.Num()]);
			const int32 AC = Side(A,B,C), AD = Side(A,B,D), CA = Side(C,D,A), CB = Side(C,D,B);
			if ((AC*AD < 0 && CA*CB < 0) || (!AC && OnEdge(A,B,C)) || (!AD && OnEdge(A,B,D))
				|| (!CA && OnEdge(C,D,A)) || (!CB && OnEdge(C,D,B))) { return false; }
		}
	}
	return true;
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
			if (!bPreserveSplineHeights && FMath::Abs(CrossProduct) <= CollinearTolerance && FVector2D::DotProduct(Incoming, Outgoing) >= 0.0f)
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

	const float SafeCollisionThickness = CollisionThicknessMeters * 100.0f;
	const FVector BottomOffset(0.0f, 0.0f, -SafeCollisionThickness);
	TArray<TArray<FVector>> CollisionConvexMeshes;
	bool bConvex = true;
	int32 TurnSign = 0;
	for (int32 I = 0; I < Vertices.Num(); ++I)
	{
		const FVector A = Vertices[(I+1)%Vertices.Num()] - Vertices[I];
		const FVector B = Vertices[(I+2)%Vertices.Num()] - Vertices[(I+1)%Vertices.Num()];
		const double Cross = A.X*B.Y-A.Y*B.X;
		if (FMath::Abs(Cross) <= 0.001) { continue; }
		const int32 Sign = Cross > 0 ? 1 : -1;
		if (TurnSign && TurnSign != Sign) { bConvex = false; break; }
		TurnSign = Sign;
	}
	// 平整凸地面合并为一个凸体；保留高度的非平面地面仍按三角形挤出。
	if (bConvex && !bPreserveSplineHeights && FMath::IsNearlyZero(GeneratedSurfaceRotation.Pitch)
		&& FMath::IsNearlyZero(GeneratedSurfaceRotation.Roll))
	{
		TArray<FVector> Prism = Vertices;
		for (const FVector& Vertex : Vertices) { Prism.Add(Vertex + BottomOffset); }
		CollisionConvexMeshes.Add(MoveTemp(Prism));
		SurfaceMesh->SetCollisionConvexMeshes(CollisionConvexMeshes);
		return;
	}
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
