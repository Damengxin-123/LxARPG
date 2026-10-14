#include "LxOceanActor.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxCompactProceduralMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Components/SplineComponent.h"
#include "CompGeom/Delaunay2.h"
#include "SegmentTypes.h"
#include "SceneView.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LandscapeProxy.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "RHI.h"
#include "Misc/ScopeExit.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Containers/Ticker.h"
#include "LxARPG/LxSource/World/GeneratedMesh/LxGeneratedMeshBuild.h"
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#include "UObject/UnrealType.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"
#endif

double LxOceanMath::CoastDistance(FVector2D P, FVector2D Min, FVector2D Max)
{
    const FVector2D D(FMath::Max3(Min.X-P.X, 0.0, P.X-Max.X), FMath::Max3(Min.Y-P.Y, 0.0, P.Y-Max.Y));
    return D.Size();
}
double LxOceanMath::Attenuation(double Distance, double FadeDistance)
{
    const double T=FMath::Clamp(Distance/FMath::Max(1.0,FadeDistance),0.0,1.0);
    return 1.0-T*T*T*(T*(T*6.0-15.0)+10.0);
}
double LxOceanMath::Wave(FVector2D P, double Time, double Amplitude, double Length, double Speed, double Direction)
{
    const double A=FMath::DegreesToRadians(Direction);
    const double K=2.0*PI/FMath::Max(100.0,Length);
    const double U=P.X*FMath::Cos(A)+P.Y*FMath::Sin(A);
    const double V=P.X*FMath::Cos(A+1.1)+P.Y*FMath::Sin(A+1.1);
    const double W=P.X*FMath::Cos(A-0.7)+P.Y*FMath::Sin(A-0.7);
    return Amplitude*(0.6*FMath::Sin(K*(U-Speed*Time))
        +0.25*FMath::Sin(K*1.7*(V-Speed*0.8*Time)+1.2)
        +0.15*FMath::Sin(K*2.3*(W-Speed*1.1*Time)+2.4));
}

ALxOceanActor::ALxOceanActor()
{
    PrimaryActorTick.bCanEverTick=true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("OceanRoot")));
    CoastlineSpline=CreateDefaultSubobject<USplineComponent>(TEXT("CoastlineSpline"));
    CoastlineSpline->SetupAttachment(RootComponent);
    CoastlineSpline->SetSplinePoints({FVector(-5000,-5000,0),FVector(5000,-5000,0),
        FVector(5000,5000,0),FVector(-5000,5000,0)},ESplineCoordinateSpace::Local,false);
    for(int32 I=0;I<4;++I) CoastlineSpline->SetSplinePointType(I,ESplinePointType::Linear,false);
    CoastlineSpline->SetClosedLoop(true);
    CoastlineSpline->bInputSplinePointsToConstructionScript=true;
    OceanMesh=CreateDefaultSubobject<ULxCompactProceduralMeshComponent>(TEXT("OceanMesh"));
    OceanMesh->SetupAttachment(RootComponent);
    OceanMesh->SetAbsolute(true,true,true);
    OceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    OceanMesh->SetGenerateOverlapEvents(false);
    OceanMesh->SetCastShadow(false);
    OceanMesh->SetCanEverAffectNavigation(false);
    SurfaceTrigger=CreateDefaultSubobject<UBoxComponent>(TEXT("SurfaceTrigger"));
    SurfaceTrigger->SetupAttachment(RootComponent);
    SurfaceTrigger->SetAbsolute(true,true,true);
    SurfaceTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SurfaceTrigger->SetCollisionObjectType(ECC_WorldStatic);
    SurfaceTrigger->SetCollisionResponseToAllChannels(ECR_Overlap);
    SurfaceTrigger->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    SurfaceTrigger->SetCollisionResponseToChannel(ECC_Camera,ECR_Ignore);
    SurfaceTrigger->SetGenerateOverlapEvents(true);
    SurfaceTrigger->SetCanEverAffectNavigation(false);
    SurfaceTrigger->OnComponentBeginOverlap.AddDynamic(this,&ALxOceanActor::HandleSurfaceOverlap);
    UnderwaterPostProcess=CreateDefaultSubobject<UPostProcessComponent>(TEXT("UnderwaterPostProcess"));
    UnderwaterPostProcess->SetupAttachment(RootComponent);
    UnderwaterPostProcess->bUnbound=true;
    UnderwaterPostProcess->Priority=10;
}

void ALxOceanActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
#if WITH_EDITOR
    if(GetWorld() && !GetWorld()->IsGameWorld()) { RequestOceanPreview(); return; }
#endif
    RebuildOcean();
}
void ALxOceanActor::RequestOceanPreview()
{
#if WITH_EDITOR
    if(IsTemplate() || !GetWorld() || GetWorld()->IsGameWorld()) return;
    bPendingFullMeshBuild=true;
    // 首次放置立即显示预览，后续构造、样条拖动及详情输入合并到同一刷新窗口。
    if(PreviewMeshBuildCount+FullMeshBuildCount==0) { RebuildOcean(); return; }
    if(bOceanPreviewQueued) return;
    bOceanPreviewQueued=true;
    const uint64 RequestSerial=++OceanPreviewRequestSerial;
    const TWeakObjectPtr<ALxOceanActor> WeakOcean(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakOcean,RequestSerial](float)
    {
        if(WeakOcean.IsValid() && !WeakOcean->IsActorBeingDestroyed() && WeakOcean->bOceanPreviewQueued
            && WeakOcean->OceanPreviewRequestSerial==RequestSerial)
            WeakOcean->RebuildOcean();
        return false;
    }),0.05f);
#endif
}

void ALxOceanActor::FinalizeOcean()
{
    bOceanPreviewQueued=false;
    ++OceanPreviewRequestSerial;
    if(IsTemplate() || !GetWorld()) return;
    const TArray<uint8> Inputs=LxGeneratedMeshBuild::CaptureInputs(this);
    if(!bPendingFullMeshBuild && !bUsingEditorPreview && bCoastlineBuildValid
        && !CompletedOceanInputs.IsEmpty() && CompletedOceanInputs==Inputs) return;
    const double BuildStart=FPlatformTime::Seconds();
    BuildOcean(false);
    UE_LOG(LogLxGeneratedMeshSave,Display,TEXT("完整海面生成 %s 耗时=%.2f秒 分区=%d 成功=%d"),
        *GetPathName(),FPlatformTime::Seconds()-BuildStart,OceanMesh->GetNumSections(),bCoastlineBuildValid);
    if(bCoastlineBuildValid && OceanMesh->GetNumSections()>0)
        CompletedOceanInputs=LxGeneratedMeshBuild::CaptureInputs(this);
}
#if WITH_EDITOR
void ALxOceanActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if(PropertyChangedEvent.GetMemberPropertyName()==GET_MEMBER_NAME_CHECKED(ALxOceanActor,CoastlinePointCount))
        ResizeCoastlinePoints(CoastlinePointCount);
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RequestOceanPreview();
}
void ALxOceanActor::PreSave(FObjectPreSaveContext SaveContext)
{
    FinalizeOcean();
    Super::PreSave(SaveContext);
}
#endif

void ALxOceanActor::BeginPlay()
{
    Super::BeginPlay();
    FinalizeOcean();
}

bool ALxOceanActor::DetectCoast()
{
    FBox Bounds(ForceInit);
    if(!GetWorld()) return false;
    for(TActorIterator<ALandscapeProxy> It(GetWorld());It;++It)
    {
        FBox LandscapeBounds=It->GetComponentsBoundingBox(true);
#if WITH_EDITOR
        if(ULandscapeInfo* Info=It->GetLandscapeInfo())
        {
            // 包含世界分区中未加载的地形代理，而非仅使用当前可见部分。
            const FBox Complete=Info->GetCompleteBounds();
            if(Complete.IsValid) LandscapeBounds=Complete;
        }
#endif
        if(LandscapeBounds.IsValid) Bounds+=LandscapeBounds;
    }
#if WITH_EDITOR
    // 根地形也未加载时，仍从世界分区描述符获取全部地形的边界。
    if(UWorldPartition* Partition=GetWorld()->GetWorldPartition())
    {
        for(UWorldPartition::TIterator<ALandscapeProxy> It(Partition);It;++It)
        {
            const FBox DescriptorBounds=It->GetEditorBounds();
            if(DescriptorBounds.IsValid) Bounds+=DescriptorBounds;
        }
    }
#endif
    if(!Bounds.IsValid || Bounds.GetSize().X<1 || Bounds.GetSize().Y<1)
    {
        GenerationDiagnostic=TEXT("未发现地形，使用手动海岸矩形。");
        return false;
    }
    CoastMin=FVector2D(Bounds.Min);
    CoastMax=FVector2D(Bounds.Max);
    bHasDetectedCoast=true;
    return true;
}

bool ALxOceanActor::SetClosedCoastline(const TArray<FVector2D>& LocalPointsMeters)
{
    if(LocalPointsMeters.Num()<3 || LocalPointsMeters.Num()>2048 || !CoastlineSpline) return false;
    TArray<FVector> Points;
    for(const FVector2D& P:LocalPointsMeters)
    {
        if(P.ContainsNaN()) return false;
        // 对外使用米且保留两位小数，仅在引擎组件边界转换为厘米。
        Points.Add(FVector(FMath::RoundToDouble(P.X*100.0),FMath::RoundToDouble(P.Y*100.0),0));
    }
    CoastlineSpline->Modify();
    CoastlineSpline->bSplineHasBeenEdited=true;
    CoastlineSpline->SetSplinePoints(Points,ESplineCoordinateSpace::Local,false);
    for(int32 I=0;I<Points.Num();++I) CoastlineSpline->SetSplinePointType(I,ESplinePointType::Linear,false);
    CoastlineSpline->SetClosedLoop(true);
    bEnableCoastlineExclusion=true;
    RebuildOcean();
    return bCoastlineBuildValid;
}

void ALxOceanActor::SetCoastlinePointCount(int32 NewPointCount)
{
    ResizeCoastlinePoints(NewPointCount);
    RebuildOcean();
}

void ALxOceanActor::ResizeCoastlinePoints(int32 NewPointCount)
{
    if(!CoastlineSpline) return;
    NewPointCount=FMath::Clamp(NewPointCount,3,2048);
    TArray<FVector> Points;
    for(int32 I=0;I<CoastlineSpline->GetNumberOfSplinePoints();++I)
        Points.Add(CoastlineSpline->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::Local));
    if(Points.Num()<3)
    {
        // 已被手动删空或只剩线段时，以半径五十米的正多边形恢复可编辑闭环。
        Points.Reset();
        for(int32 I=0;I<NewPointCount;++I)
        {
            const double Angle=2.0*PI*I/NewPointCount;
            Points.Add(FVector(FMath::RoundToDouble(5000.0*FMath::Cos(Angle)),FMath::RoundToDouble(5000.0*FMath::Sin(Angle)),0));
        }
    }
    while(Points.Num()<NewPointCount)
    {
        int32 Longest=0;
        double LongestSquared=-1;
        for(int32 I=0;I<Points.Num();++I)
        {
            const double LengthSquared=FVector::DistSquared2D(Points[I],Points[(I+1)%Points.Num()]);
            if(LengthSquared>LongestSquared) { LongestSquared=LengthSquared; Longest=I; }
        }
        const FVector Midpoint=(Points[Longest]+Points[(Longest+1)%Points.Num()])*0.5;
        Points.Insert(Midpoint,Longest+1);
    }
    while(Points.Num()>NewPointCount)
    {
        int32 RemoveIndex=0;
        double SmallestArea=TNumericLimits<double>::Max();
        for(int32 I=0;I<Points.Num();++I)
        {
            const FVector Before=Points[(I+Points.Num()-1)%Points.Num()]-Points[I];
            const FVector After=Points[(I+1)%Points.Num()]-Points[I];
            const double Area=FMath::Abs(FVector::CrossProduct(Before,After).Z);
            if(Area<SmallestArea) { SmallestArea=Area; RemoveIndex=I; }
        }
        Points.RemoveAt(RemoveIndex);
    }
    CoastlineSpline->Modify();
    CoastlineSpline->bSplineHasBeenEdited=true;
    CoastlineSpline->SetSplinePoints(Points,ESplineCoordinateSpace::Local,false);
    NormalizeCoastlinePolygon();
}

void ALxOceanActor::NormalizeCoastlinePolygon()
{
    if(!CoastlineSpline) return;
    CoastlinePointCount=CoastlineSpline->GetNumberOfSplinePoints();
    for(int32 I=0;I<CoastlinePointCount;++I)
        CoastlineSpline->SetSplinePointType(I,ESplinePointType::Linear,false);
    CoastlineSpline->SetClosedLoop(true,false);
    CoastlineSpline->UpdateSpline();
}

bool ALxOceanActor::ShouldAnimateWaves() const
{
    return bPreviewWavesInEditor || (GetWorld() && GetWorld()->IsGameWorld());
}

bool ALxOceanActor::CacheCoastline()
{
    CoastlinePolygon.Reset();
    CoastlineBounds=FBox2D(ForceInit);
    if(!bEnableCoastlineExclusion) return true;
    if(!CoastlineSpline || !CoastlineSpline->IsClosedLoop() || CoastlineSpline->GetNumberOfSplinePoints()<3
        || CoastlineSpline->GetNumberOfSplinePoints()>2048)
    {
        GenerationDiagnostic+=TEXT(" 海岸线必须闭合并包含三至二千零四十八个控制点，已暂停海洋生成。");
        return false;
    }
    const int32 Count=CoastlineSpline->GetNumberOfSplinePoints();
    // 查询与自相交检查只使用真实控制点，避免对海浪细分点执行平方复杂度检查。
    for(int32 I=0;I<Count;++I)
    {
        const FVector2D P(CoastlineSpline->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::World));
        if(P.ContainsNaN()) return false;
        if(CoastlinePolygon.IsEmpty() || !CoastlinePolygon.Last().Equals(P,0.01)) CoastlinePolygon.Add(P);
    }
    if(CoastlinePolygon.Num()>1 && CoastlinePolygon[0].Equals(CoastlinePolygon.Last(),0.01)) CoastlinePolygon.Pop();
    if(CoastlinePolygon.Num()<3 || CoastlinePolygon.Num()>2048) return false;
    double Area=0;
    for(int32 I=0;I<CoastlinePolygon.Num();++I)
    {
        const FVector2D A=CoastlinePolygon[I],B=CoastlinePolygon[(I+1)%CoastlinePolygon.Num()];
        Area+=FVector2D::CrossProduct(A-CoastlinePolygon[0],B-CoastlinePolygon[0]);
        CoastlineBounds+=A;
        const UE::Geometry::FSegment2d Edge(A,B);
        for(int32 J=I+2;J<CoastlinePolygon.Num();++J)
        {
            if(I==0 && J==CoastlinePolygon.Num()-1) continue;
            if(Edge.Intersects(UE::Geometry::FSegment2d(CoastlinePolygon[J],CoastlinePolygon[(J+1)%CoastlinePolygon.Num()])))
            {
                GenerationDiagnostic+=TEXT(" 海岸线存在自相交或重叠，请调整控制点；已暂停海洋生成。");
                return false;
            }
        }
    }
    if(FMath::Abs(Area)<1)
    {
        GenerationDiagnostic+=TEXT(" 海岸线面积为零，请使用至少三个不共线的点。");
        return false;
    }
    return true;
}

bool ALxOceanActor::IsInsideCoastline(FVector WorldPoint) const
{
    if(!bEnableCoastlineExclusion || !bCoastlineBuildValid || CoastlinePolygon.Num()<3) return false;
    const FVector2D P(WorldPoint);
    if(!CoastlineBounds.ExpandBy(0.01).IsInsideOrOn(P)) return false;
    bool bInside=false;
    for(int32 I=0,J=CoastlinePolygon.Num()-1;I<CoastlinePolygon.Num();J=I++)
    {
        const FVector2D A=CoastlinePolygon[J],B=CoastlinePolygon[I];
        // 边界归入排除区；同时避免网格点落在待约束边的中间。
        if(UE::Geometry::FSegment2d(A,B).DistanceSquared(P)<=0.0001) return true;
        if((A.Y>P.Y)!=(B.Y>P.Y) && P.X<(B.X-A.X)*(P.Y-A.Y)/(B.Y-A.Y)+A.X) bInside=!bInside;
    }
    return bInside;
}

bool ALxOceanActor::HasOceanAtLocation(FVector WorldPoint) const
{
    return bCoastlineBuildValid && !WorldPoint.ContainsNaN()
        && OceanBounds.IsInsideOrOn(FVector2D(WorldPoint)) && !IsInsideCoastline(WorldPoint);
}

bool ALxOceanActor::BuildCoastlineMesh(const TArray<double>& X,const TArray<double>& Y,
    TArray<FVector2D>& Points,TArray<int32>& Indices,bool bPreview) const
{
    using namespace UE::Geometry;
    Points=CoastlinePolygon;
    if(!bPreview)
    {
        // 完整网格才沿直线细分海岸以支撑波浪；预览直接保留全部原始拐点。
        double Perimeter=0;
        for(int32 I=0;I<CoastlinePolygon.Num();++I)
            Perimeter+=(CoastlinePolygon[(I+1)%CoastlinePolygon.Num()]-CoastlinePolygon[I]).Size();
        const double Spacing=FMath::Max(FMath::Min(double(GridSpacing),WaveLength/10.0),
            Perimeter/FMath::Max(1,2048-CoastlinePolygon.Num()));
        Points.Reset();
        for(int32 I=0;I<CoastlinePolygon.Num();++I)
        {
            const FVector2D A=CoastlinePolygon[I],B=CoastlinePolygon[(I+1)%CoastlinePolygon.Num()];
            const int32 Steps=FMath::Clamp(FMath::CeilToInt((B-A).Size()/Spacing),1,2048);
            for(int32 J=0;J<Steps;++J) Points.Add(FMath::Lerp(A,B,double(J)/Steps));
        }
    }
    TArray<FIndex2i> Edges;
    for(int32 I=0;I<Points.Num();++I) Edges.Add(FIndex2i(I,(I+1)%Points.Num()));
    for(double Py:Y) for(double Px:X)
        if(!IsInsideCoastline(FVector(Px,Py,0))) Points.Add(FVector2D(Px,Py));
    FDelaunay2 Triangulation;
    if(!Triangulation.Triangulate(Points,Edges)) return false;
    for(const FIndex3i& T:Triangulation.GetTriangles())
    {
        const FVector2D Center=(Points[T.A]+Points[T.B]+Points[T.C])/3.0;
        if(IsInsideCoastline(FVector(Center,0))) continue;
        const double Cross=FVector2D::CrossProduct(Points[T.B]-Points[T.A],Points[T.C]-Points[T.A]);
        if(FMath::Abs(Cross)>UE_DOUBLE_SMALL_NUMBER)
            Indices.Append({T.A,Cross>0?T.C:T.B,Cross>0?T.B:T.C});
    }
    // 仅上传实际使用的顶点，孔洞内不保留海面几何资源。
    TArray<int32> Remap; Remap.Init(INDEX_NONE,Points.Num());
    TArray<FVector2D> Used;
    for(int32& Index:Indices)
    {
        if(Remap[Index]==INDEX_NONE) Remap[Index]=Used.Add(Points[Index]);
        Index=Remap[Index];
    }
    Points=MoveTemp(Used);
    return !Indices.IsEmpty();
}

void ALxOceanActor::OverrideBlendableSettings(FSceneView& View,float Weight) const
{
    // 每个编辑器、场景捕获和分屏视图独立过滤，陆地区域不提交整屏水下材质。
    if(bEnableUnderwater && UnderwaterMID && IsUnderwater(View.ViewLocation-FVector(0,0,10)))
        if(IBlendableInterface* Blendable=Cast<IBlendableInterface>(UnderwaterMID.Get()))
            Blendable->OverrideBlendableSettings(View,Weight);
}

void ALxOceanActor::RebuildOcean()
{
    bOceanPreviewQueued=false;
    ++OceanPreviewRequestSerial;
    BuildOcean(GetWorld() && !GetWorld()->IsGameWorld());
    if(!bUsingEditorPreview && bCoastlineBuildValid && OceanMesh->GetNumSections()>0)
        CompletedOceanInputs=LxGeneratedMeshBuild::CaptureInputs(this);
}

void ALxOceanActor::BuildOcean(bool bPreview)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(LxOcean_BuildMesh);
    GenerationDiagnostic.Reset();
    if(!GetWorld() || !OceanMesh) return;
    bUsingEditorPreview=bPreview;
    bPendingFullMeshBuild=bPreview;
    if(bPreview) { ++PreviewMeshBuildCount; GenerationDiagnostic=TEXT("轻量海面预览；保存和运行前自动生成完整网格。"); }
    else { ++FullMeshBuildCount; }
    NormalizeCoastlinePolygon();
    bCoastlineBuildValid=CacheCoastline();
    OceanMesh->SetVisibility(bCoastlineBuildValid);
    SurfaceTrigger->SetCollisionEnabled(bCoastlineBuildValid?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
    if(!bCoastlineBuildValid)
    {
        OceanMesh->ClearAllMeshSections();
        UnderwaterPostProcess->BlendWeight=0;
        CaptureComponentIndex=-1;
        FoamCandidates.Reset();
        GenerationDiagnostic+=TEXT(" 请修正海岸线后重建海洋。");
        return;
    }
    if(bAutoDetectLandscape && (!GetWorld()->IsGameWorld() || !bHasDetectedCoast)) DetectCoast();
    if(CoastMin.ContainsNaN() || CoastMax.ContainsNaN()) { GenerationDiagnostic=TEXT("海岸坐标无效。"); return; }
    const FVector2D OldMin=CoastMin;
    CoastMin=FVector2D(FMath::Min(OldMin.X,CoastMax.X),FMath::Min(OldMin.Y,CoastMax.Y));
    CoastMax=FVector2D(FMath::Max(OldMin.X,CoastMax.X),FMath::Max(OldMin.Y,CoastMax.Y));
    CoastMax.X=FMath::Max(CoastMax.X,CoastMin.X+1);
    CoastMax.Y=FMath::Max(CoastMax.Y,CoastMin.Y+1);
    // 蓝图赋值同样经过约束，避免除零、非法尺寸或无限分配。
    MaxWaveAmplitude=FMath::Clamp(FMath::IsFinite(MaxWaveAmplitude)?MaxWaveAmplitude:0.f,0.f,2000.f);
    WaveFadeDistance=FMath::Clamp(FMath::IsFinite(WaveFadeDistance)?WaveFadeDistance:15000.f,1.f,1000000.f);
    WaveLength=FMath::Clamp(FMath::IsFinite(WaveLength)?WaveLength:2400.f,100.f,100000.f);
    WaveSpeed=FMath::Clamp(FMath::IsFinite(WaveSpeed)?WaveSpeed:0.f,0.f,10000.f);
    WaveDirection=FMath::IsFinite(WaveDirection)?FMath::Fmod(WaveDirection,360.f):0.f;
    OceanExtend=FMath::Max(WaveFadeDistance,FMath::Clamp(FMath::IsFinite(OceanExtend)?OceanExtend:50000.f,1000.f,1000000.f));
    GridSpacing=FMath::Clamp(FMath::IsFinite(GridSpacing)?GridSpacing:200.f,50.f,5000.f);
    MaxGridSegments=FMath::Clamp(MaxGridSegments,32,512);
    FoamResolution=FMath::Clamp(FoamResolution,32,256);
    FoamCaptureRadius=FMath::Clamp(FMath::IsFinite(FoamCaptureRadius)?FoamCaptureRadius:4000.f,500.f,50000.f);
    FoamWidth=FMath::Clamp(FMath::IsFinite(FoamWidth)?FoamWidth:100.f,5.f,1000.f);
    SurfaceTriggerThickness=FMath::Clamp(FMath::IsFinite(SurfaceTriggerThickness)?SurfaceTriggerThickness:10.f,1.f,100.f);
    UnderwaterTransitionDepth=FMath::Max(1.f,UnderwaterTransitionDepth);
    // 闭环即使超出现有地形矩形也完整包含在海域内，不裁断海岸孔洞。
    FBox2D MeshCoastBounds(CoastMin,CoastMax);
    if(bEnableCoastlineExclusion) MeshCoastBounds+=CoastlineBounds;
    OceanBounds=MeshCoastBounds.ExpandBy(OceanExtend);
    const FVector2D Center=OceanBounds.GetCenter();
    const FVector Origin(Center,GetActorLocation().Z);
    OceanMesh->SetWorldLocationAndRotation(Origin,FRotator::ZeroRotator);
    OceanMesh->SetWorldScale3D(FVector::OneVector);
    OceanMesh->ClearAllMeshSections();
    // 海岸及衰减带细分，外侧完全平静的区域只增加边界行。
    auto MakeAxis=[this,bPreview](double Min,double Max)
    {
        TArray<double> Result;
        const double Start=Min-WaveFadeDistance, End=Max+WaveFadeDistance;
        const double Desired=FMath::Min(double(GridSpacing),WaveLength/10.0);
        const int32 SegmentBudget=bPreview?FMath::Min(MaxGridSegments,24):MaxGridSegments;
        const int32 Count=FMath::Clamp(FMath::CeilToInt((End-Start)/Desired),2,SegmentBudget-2);
        if(!bPreview && (End-Start)/Count>WaveLength/6.0)
            GenerationDiagnostic+=TEXT(" 网格预算较低，建议提高每轴最大分段或增大波长。");
        if(OceanExtend>WaveFadeDistance) Result.Add(Min-OceanExtend);
        for(int32 I=0;I<=Count;++I) Result.Add(FMath::Lerp(Start,End,double(I)/Count));
        if(OceanExtend>WaveFadeDistance) Result.Add(Max+OceanExtend);
        return Result;
    };
    const TArray<double> X=MakeAxis(MeshCoastBounds.Min.X,MeshCoastBounds.Max.X),Y=MakeAxis(MeshCoastBounds.Min.Y,MeshCoastBounds.Max.Y);
    TArray<FVector> Vertices,Normals;
    TArray<FVector2D> UVs;
    TArray<int32> Indices;
    TArray<FProcMeshTangent> Tangents;
    TArray<FVector2D> MeshPoints;
    if(bEnableCoastlineExclusion)
    {
        if(!BuildCoastlineMesh(X,Y,MeshPoints,Indices,bPreview))
        {
            bCoastlineBuildValid=false;
            SurfaceTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            UnderwaterPostProcess->BlendWeight=0;
            GenerationDiagnostic+=TEXT(" 海岸约束网格生成失败，已暂停海洋；请检查重合点或过窄海湾。");
            return;
        }
    }
    else
    {
        for(double Py:Y) for(double Px:X) MeshPoints.Add(FVector2D(Px,Py));
        for(int32 J=0;J<Y.Num()-1;++J) for(int32 I=0;I<X.Num()-1;++I)
        {
            const int32 A=J*X.Num()+I,B=A+1,C=A+X.Num(),D=C+1;
            Indices.Append({A,C,B,B,C,D});
        }
    }
    for(const FVector2D& P:MeshPoints)
    {
        Vertices.Add(FVector(P-Center,0));
        Normals.Add(FVector::UpVector);
        UVs.Add(P/1000.0);
        Tangents.Add(FProcMeshTangent(1,0,0));
    }
    OceanMesh->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UVs,TArray<FLinearColor>(),Tangents,false);
    // 额外包围盒厚度避免 GPU 波峰被视锥裁剪。
    OceanMesh->SetBoundsScale(1.1f);
    SurfaceTrigger->SetWorldLocationAndRotation(Origin-FVector(0,0,SurfaceTriggerThickness*0.5),FRotator::ZeroRotator);
    SurfaceTrigger->SetWorldScale3D(FVector::OneVector);
    const FVector2D Extent=OceanBounds.GetExtent();
    SurfaceTrigger->SetBoxExtent(FVector(Extent,SurfaceTriggerThickness*0.5),true);
    CaptureComponentIndex=-1;
    FoamCandidates.Reset();
    FoamWait=0;
    // 编辑预览不分配碰撞浪花纹理；完整生成只在分辨率变化时创建新纹理。
    if(!bPreview && GetNetMode()!=NM_DedicatedServer
        && (!FoamTexture || FoamTexture->GetSizeX()!=FoamResolution || FoamTexture->GetSizeY()!=FoamResolution))
    {
        FoamPixels.Init(FColor::Black,FoamResolution*FoamResolution);
        FoamTexture=UTexture2D::CreateTransient(FoamResolution,FoamResolution,PF_B8G8R8A8,NAME_None,
            TConstArrayView64<uint8>(reinterpret_cast<const uint8*>(FoamPixels.GetData()),FoamPixels.Num()*sizeof(FColor)));
        FoamTexture->SRGB=false;
        FoamTexture->Filter=TF_Bilinear;
        FoamTexture->AddressX=TA_Clamp;
        FoamTexture->AddressY=TA_Clamp;
        FoamTexture->UpdateResource();
    }
    // 连续拖动复用动态材质，只有用户替换材质资产时才创建新实例。
    if(!OceanMaterial) OceanMID=nullptr;
    else if(!OceanMID || OceanMID->Parent!=OceanMaterial) OceanMID=UMaterialInstanceDynamic::Create(OceanMaterial,this);
    OceanMesh->SetMaterial(0,OceanMID?static_cast<UMaterialInterface*>(OceanMID.Get()):OceanMaterial.Get());
    if(!UnderwaterMaterial) UnderwaterMID=nullptr;
    else if(!UnderwaterMID || UnderwaterMID->Parent!=UnderwaterMaterial) UnderwaterMID=UMaterialInstanceDynamic::Create(UnderwaterMaterial,this);
    UnderwaterPostProcess->Settings.WeightedBlendables.Array.Reset();
    if(UnderwaterMID) UnderwaterPostProcess->Settings.AddBlendable(this,1.f);
    if(OceanMID && FoamTexture) OceanMID->SetTextureParameterValue(TEXT("CollisionFoam"),FoamTexture);
    UpdateMaterialParameters();
}
void ALxOceanActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(ShouldAnimateWaves()) WaveTime+=FMath::Max(0.f,DeltaSeconds);
    UpdateMaterialParameters();
    if(GetNetMode()!=NM_DedicatedServer) UpdateCollisionFoam(DeltaSeconds);
}
double ALxOceanActor::GetWaveAttenuation(FVector P) const
{
    if(!bCoastlineBuildValid || IsInsideCoastline(P)) return 0;
    return LxOceanMath::Attenuation(LxOceanMath::CoastDistance(FVector2D(P),CoastMin,CoastMax),WaveFadeDistance);
}
double ALxOceanActor::GetSurfaceHeight(FVector P) const
{
    if(!ShouldAnimateWaves()) return GetActorLocation().Z;
    return GetActorLocation().Z+GetWaveAttenuation(P)*LxOceanMath::Wave(FVector2D(P)-CoastMin,WaveTime,
        MaxWaveAmplitude,WaveLength,WaveSpeed,WaveDirection);
}
bool ALxOceanActor::IsUnderwater(FVector P) const
{
    return HasOceanAtLocation(P) && P.Z<GetSurfaceHeight(P);
}
FVector ALxOceanActor::ProjectToSeaLevel(FVector P) const
{
    P.Z=GetActorLocation().Z;
    return P;
}
void ALxOceanActor::HandleSurfaceOverlap(UPrimitiveComponent* Component,AActor* Other,UPrimitiveComponent* OtherComponent,int32 BodyIndex,bool bSweep,const FHitResult& Hit)
{
    if(!Other || Other==this) return;
    const FVector Point=ProjectToSeaLevel(bSweep?FVector(Hit.ImpactPoint):
        (OtherComponent?OtherComponent->GetComponentLocation():Other->GetActorLocation()));
    if(HasOceanAtLocation(Point)) OnSeaSurfaceOverlap.Broadcast(Other,OtherComponent,Point);
}
void ALxOceanActor::UpdateMaterialParameters()
{
    // 水上摄像机无需执行整屏水下材质；分屏中任一摄像机入水时仍由材质逐视图判断。
    bool bUnderwaterView=bCoastlineBuildValid && bEnableUnderwater && GetWorld() && !GetWorld()->IsGameWorld();
    if(bCoastlineBuildValid && bEnableUnderwater && GetWorld() && GetWorld()->IsGameWorld())
    {
        for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)
        {
            const APlayerController* PC=It->Get();
            if(PC && PC->IsLocalController() && PC->PlayerCameraManager
                && IsUnderwater(PC->PlayerCameraManager->GetCameraLocation()-FVector(0,0,10)))
            { bUnderwaterView=true; break; }
        }
    }
    UnderwaterPostProcess->BlendWeight=bUnderwaterView?1.f:0.f;
    for(UMaterialInstanceDynamic* MID:{OceanMID.Get(),UnderwaterMID.Get()})
    {
        if(!MID) continue;
        MID->SetVectorParameterValue(TEXT("CoastRect"),FLinearColor(CoastMin.X,CoastMin.Y,CoastMax.X,CoastMax.Y));
        MID->SetVectorParameterValue(TEXT("OceanRect"),FLinearColor(OceanBounds.Min.X,OceanBounds.Min.Y,OceanBounds.Max.X,OceanBounds.Max.Y));
        MID->SetVectorParameterValue(TEXT("WaveSettings"),FLinearColor(ShouldAnimateWaves()?MaxWaveAmplitude:0.f,WaveLength,ShouldAnimateWaves()?WaveSpeed:0.f,FMath::DegreesToRadians(WaveDirection)));
        MID->SetVectorParameterValue(TEXT("WaterColor"),WaterColor);
        MID->SetScalarParameterValue(TEXT("WaveTime"),ShouldAnimateWaves()?WaveTime:0.0);
        MID->SetScalarParameterValue(TEXT("FadeDistance"),WaveFadeDistance);
        MID->SetScalarParameterValue(TEXT("SeaLevel"),GetActorLocation().Z);
        MID->SetScalarParameterValue(TEXT("FoamEnabled"),bEnableFoam && !bUsingEditorPreview && !bPendingFullMeshBuild?1.f:0.f);
        MID->SetScalarParameterValue(TEXT("FoamWidth"),FoamWidth);
        MID->SetScalarParameterValue(TEXT("TransitionDepth"),UnderwaterTransitionDepth);
        MID->SetScalarParameterValue(TEXT("FogDensity"),UnderwaterFogDensity);
    }
}
void ALxOceanActor::BeginFoamCapture()
{
    FVector Center=GetActorLocation();
    if(IsValid(FoamObserver)) Center=FoamObserver->GetActorLocation();
    else if(APlayerController* PC=GetWorld()->GetFirstPlayerController())
    {
        if(PC->PlayerCameraManager) Center=PC->PlayerCameraManager->GetCameraLocation();
    }
    const double PixelSize=2.0*FoamCaptureRadius/FoamResolution;
    CaptureMin=FVector2D(FMath::GridSnap(Center.X-FoamCaptureRadius,PixelSize),FMath::GridSnap(Center.Y-FoamCaptureRadius,PixelSize));
    FoamCandidates.Reset();
    FoamPixels.Init(FColor::Black,FoamResolution*FoamResolution);
    const FBox Region(FVector(CaptureMin,GetActorLocation().Z-MaxWaveAmplitude-FoamWidth),
        FVector(CaptureMin+FVector2D(2*FoamCaptureRadius),GetActorLocation().Z+MaxWaveAmplitude+FoamWidth));
    for(TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if(It->IsA<ALxOceanActor>()) continue;
        TInlineComponentArray<UPrimitiveComponent*> Components;
        It->GetComponents(Components);
        for(UPrimitiveComponent* C:Components)
            if(C && C->IsRegistered() && C->IsCollisionEnabled() && C->Bounds.GetBox().ExpandBy(FoamWidth).Intersect(Region))
                FoamCandidates.Add(C);
    }
    CaptureComponentIndex=0;
    CapturePixelIndex=0;
}
float ALxOceanActor::SampleCollisionFoam(UPrimitiveComponent* C,FVector Position) const
{
    if(!C || !C->IsCollisionEnabled() || !HasOceanAtLocation(Position)) return 0;
    FVector Closest;
    double Distance=FoamWidth;
    // 简单碰撞涵盖球、胶囊、盒、凸体，包括不可见碰撞体与 PhysicsOnly 物体。
    const float ClosestDistance=C->GetClosestPointOnCollision(Position,Closest);
    if(ClosestDistance>=0) Distance=FMath::Min(Distance,double(ClosestDistance));
    // 复杂碰撞和地形没有可用凸体，直接查询组件三角形，不依赖 Visibility 通道。
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OceanContactFoam),true,this);
    if(C->LineTraceComponent(Hit,Position+FVector(0,0,FoamWidth),Position-FVector(0,0,FoamWidth),Params))
        Distance=FMath::Min(Distance,FVector::Distance(Position,Hit.ImpactPoint));
    if(ClosestDistance<0)
    {
        // 纯复杂碰撞的垂直墙面与竖直射线平行，补充水平双向射线。
        for(const FVector Axis:{FVector(FoamWidth,0,0),FVector(0,FoamWidth,0)})
        {
            if(C->LineTraceComponent(Hit,Position+Axis,Position-Axis,Params))
                Distance=FMath::Min(Distance,FVector::Distance(Position,Hit.ImpactPoint));
            if(C->LineTraceComponent(Hit,Position-Axis,Position+Axis,Params))
                Distance=FMath::Min(Distance,FVector::Distance(Position,Hit.ImpactPoint));
        }
    }
    const float T=FMath::Clamp(1.0-Distance/FoamWidth,0.0,1.0);
    return T*T*(3-2*T);
}
void ALxOceanActor::UpdateCollisionFoam(float DeltaSeconds)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(LxOcean_CollisionFoam);
    LastFoamSampleCount=0;
    LastFoamUpdateMilliseconds=0;
    if(bUsingEditorPreview || bPendingFullMeshBuild || !bCoastlineBuildValid || !bEnableFoam || !OceanMID || !FoamTexture || !GetWorld()) return;
    const double StartTime=FPlatformTime::Seconds();
    ON_SCOPE_EXIT { LastFoamUpdateMilliseconds=(FPlatformTime::Seconds()-StartTime)*1000; };
    const double Deadline=StartTime+FMath::Clamp(FMath::IsFinite(FoamTimeBudgetMs)?FoamTimeBudgetMs:0.5f,0.1f,4.f)*0.001;
    if(CaptureComponentIndex<0)
    {
        FoamWait-=DeltaSeconds;
        if(FoamWait>0) return;
        BeginFoamCapture();
    }
    int32 Budget=FMath::Clamp(FoamSamplesPerFrame,128,16384);
    const double PixelSize=2.0*FoamCaptureRadius/FoamResolution;
    while(CaptureComponentIndex<FoamCandidates.Num() && Budget>0 && FPlatformTime::Seconds()<Deadline)
    {
        UPrimitiveComponent* C=FoamCandidates[CaptureComponentIndex].Get();
        if(!C || !C->IsCollisionEnabled()) { ++CaptureComponentIndex; CapturePixelIndex=0; continue; }
        const FBox Bounds=C->Bounds.GetBox().ExpandBy(FoamWidth);
        const int32 X0=FMath::Clamp(FMath::FloorToInt((Bounds.Min.X-CaptureMin.X)/PixelSize),0,FoamResolution-1);
        const int32 Y0=FMath::Clamp(FMath::FloorToInt((Bounds.Min.Y-CaptureMin.Y)/PixelSize),0,FoamResolution-1);
        const int32 X1=FMath::Clamp(FMath::CeilToInt((Bounds.Max.X-CaptureMin.X)/PixelSize),0,FoamResolution-1);
        const int32 Y1=FMath::Clamp(FMath::CeilToInt((Bounds.Max.Y-CaptureMin.Y)/PixelSize),0,FoamResolution-1);
        const int32 Width=X1-X0+1,Count=Width*(Y1-Y0+1);
        while(CapturePixelIndex<Count && Budget>0 && FPlatformTime::Seconds()<Deadline)
        {
            const int32 X=X0+CapturePixelIndex%Width,Y=Y0+CapturePixelIndex/Width;
            FVector P(CaptureMin+FVector2D((X+0.5)*PixelSize,(Y+0.5)*PixelSize),0);
            ++CapturePixelIndex; --Budget;
            if(!HasOceanAtLocation(P)) continue;
            P.Z=GetSurfaceHeight(P);
            const uint8 Value=FMath::RoundToInt(255*SampleCollisionFoam(C,P));
            FColor& Pixel=FoamPixels[Y*FoamResolution+X];
            Pixel=FColor(FMath::Max(Pixel.R,Value),0,0,255);
            ++LastFoamSampleCount;
        }
        if(CapturePixelIndex>=Count) { ++CaptureComponentIndex; CapturePixelIndex=0; }
    }
    if(CaptureComponentIndex>=FoamCandidates.Num())
    {
        CommitFoamCapture();
        CaptureComponentIndex=-1;
        FoamWait=FMath::Clamp(FoamUpdateInterval,0.05f,5.f);
    }
}
void ALxOceanActor::CommitFoamCapture()
{
    ++CompletedFoamCaptures;
    const int32 Bytes=FoamPixels.Num()*sizeof(FColor);
    uint8* Data=new uint8[Bytes];
    FMemory::Memcpy(Data,FoamPixels.GetData(),Bytes);
    FUpdateTextureRegion2D* Region=new FUpdateTextureRegion2D(0,0,0,0,FoamResolution,FoamResolution);
    FoamTexture->UpdateTextureRegions(0,1,Region,FoamResolution*sizeof(FColor),sizeof(FColor),Data,
        [](uint8* Buffer,const FUpdateTextureRegion2D* UpdateRegion){ delete[] Buffer; delete UpdateRegion; });
    OceanMID->SetVectorParameterValue(TEXT("FoamRect"),FLinearColor(CaptureMin.X,CaptureMin.Y,2*FoamCaptureRadius,2*FoamCaptureRadius));
}
