#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Algo/Reverse.h"
#include "SceneView.h"
#include "SceneTypes.h"
#include "Components/SphereComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/UnrealType.h"
#include "ProceduralMeshComponent.h"
#include "LxOceanActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillDetectionComponent.h"

/** 验证矩形距离、平滑端点、角点衰减和振幅严格上限。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanMathTest,"LxARPG.World.Ocean.Waves",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanMathTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("矩形内部为零"),LxOceanMath::CoastDistance({0,0},{-100,-200},{100,200}),0.0);
    TestEqual(TEXT("沿边向外距离"),LxOceanMath::CoastDistance({150,0},{-100,-200},{100,200}),50.0);
    TestEqual(TEXT("角点欧氏距离"),LxOceanMath::CoastDistance({130,240},{-100,-200},{100,200}),50.0);
    TestEqual(TEXT("近岸保持最大权重"),LxOceanMath::Attenuation(0,1000),1.0);
    TestEqual(TEXT("半程衰减一半"),LxOceanMath::Attenuation(500,1000),0.5);
    TestEqual(TEXT("终点精确零"),LxOceanMath::Attenuation(1000,1000),0.0);
    TestEqual(TEXT("远海精确零"),LxOceanMath::Attenuation(3000,1000),0.0);
    TestTrue(TEXT("起点平滑"),FMath::Abs(LxOceanMath::Attenuation(0.1,1000)-1)<1.e-8);
    TestTrue(TEXT("终点平滑"),FMath::Abs(LxOceanMath::Attenuation(999.9,1000))<1.e-8);
    double Previous=1;
    for(int32 I=0;I<=1000;++I)
    {
        const double Weight=LxOceanMath::Attenuation(I,1000);
        if(Weight>Previous+1.e-12) AddError(TEXT("衰减必须单调"));
        Previous=Weight;
        const double H=LxOceanMath::Wave({I*19.3,I*-7.2},I*.13,80,1700,240,35);
        if(FMath::Abs(H)>80.00001) AddError(TEXT("合成波超出最大振幅"));
    }
    TestEqual(TEXT("零振幅完全静止"),LxOceanMath::Wave({10,10},20,0,1000,100,0),0.0);
    return true;
}

/** 验证生成、水平变换、水下范围、碰撞浪花以及真正的技能重叠命中链路。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanIntegrationTest,"LxARPG.World.Ocean.Integration",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanIntegrationTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    World->InitializeActorsForPlay(FURL());
    // 独立世界没有 GameMode；引擎仅在世界已开始游戏时广播真实重叠事件。
    World->SetBegunPlay(true);
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false;
    Ocean->CoastMin={-1000,-1000}; Ocean->CoastMax={1000,1000};
    Ocean->MaxWaveAmplitude=0; Ocean->WaveFadeDistance=1000; Ocean->OceanExtend=2000;
    Ocean->SetActorLocation(FVector(100,200,300));
    Ocean->SetActorRotation(FRotator(10,30,20));
    Ocean->RebuildOcean();
    TestEqual(TEXT("仅一个海面网格"),Ocean->OceanMesh->GetNumSections(),1);
    const int32 Vertices=Ocean->OceanMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num();
    Ocean->RebuildOcean();
    TestEqual(TEXT("重建不叠加顶点"),Ocean->OceanMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),Vertices);
    TestTrue(TEXT("海面始终水平"),Ocean->OceanMesh->GetComponentRotation().IsNearlyZero());
    TestEqual(TEXT("海面高度来自Actor"),Ocean->GetSurfaceHeight(FVector::ZeroVector),300.0);
    TestTrue(TEXT("摄像机水下"),Ocean->IsUnderwater({0,0,299}));
    TestFalse(TEXT("摄像机水上"),Ocean->IsUnderwater({0,0,301}));
    TestFalse(TEXT("有限海域外无滤镜"),Ocean->IsUnderwater({100000,0,-500}));
    TestEqual(TEXT("海面不阻挡"),Ocean->SurfaceTrigger->GetCollisionResponseToChannel(ECC_WorldDynamic),ECR_Overlap);
    TestTrue(TEXT("海面重叠开启"),Ocean->SurfaceTrigger->GetGenerateOverlapEvents());

    AActor* Obstacle=World->SpawnActor<AActor>();
    UBoxComponent* Box=NewObject<UBoxComponent>(Obstacle);
    Obstacle->SetRootComponent(Box); Box->SetBoxExtent({100,100,300});
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent(); Obstacle->SetActorLocation({500,0,300});
    TestTrue(TEXT("无模型碰撞体也产生浪花"),Ocean->SampleCollisionFoam(Box,{605,0,300})>0.9f);
    TestEqual(TEXT("远离碰撞体无浪花"),Ocean->SampleCollisionFoam(Box,{900,0,300}),0.f);
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestEqual(TEXT("关闭碰撞不产生浪花"),Ocean->SampleCollisionFoam(Box,{605,0,300}),0.f);

    AActor* Skill=World->SpawnActor<AActor>();
    USphereComponent* Sphere=NewObject<USphereComponent>(Skill);
    Skill->SetRootComponent(Sphere); Sphere->SetSphereRadius(5);
    Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Sphere->SetCollisionObjectType(ECC_WorldDynamic); Sphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    Sphere->SetGenerateOverlapEvents(true); Sphere->RegisterComponent();
    Skill->SetActorLocation({0,0,500});
    ULxSkillDetectionComponent* Detection=NewObject<ULxSkillDetectionComponent>(Skill);
    Detection->RegisterComponent(); Detection->SetTriggerCollisionComponent(Sphere);
    // 即便普通场景命中关闭，海洋重叠仍按用户约定触发技能命中。
    Detection->SetPublishWorldHit(false); Detection->StartDetection();
    Ocean->DispatchBeginPlay();
    Skill->DispatchBeginPlay();
    int32 HitCount=0;
    Detection->OnDetectionResult.AddLambda([this,Ocean,&HitCount](const FLxSkillDetectionResult& Result)
    {
        if(Result.HitActor==Ocean)
        {
            ++HitCount;
            TestTrue(TEXT("重叠转换为场景命中"),Result.bHitWorld);
            TestEqual(TEXT("命中类型"),Result.EventType,ELxSkillDetectionEventType::HitWorld);
            TestEqual(TEXT("命中点位于海平面"),Result.HitLocation.Z,300.0);
            TestTrue(TEXT("海面法线向上"),Result.HitNormal.Equals(FVector::UpVector));
        }
    });
    FHitResult Sweep;
    Skill->SetActorLocation({0,0,250},true,&Sweep);
    TestFalse(TEXT("技能穿过海面不被实体阻挡"),Sweep.bBlockingHit);
    TestEqual(TEXT("高速跨海面触发一次"),HitCount,1);
    Skill->SetActorLocation({100,200,500});
    // 平移但不旋转的海岸包围技能落点，验证真实碰撞事件也遵循陆地排除。
    Ocean->SetActorRotation(FRotator::ZeroRotator);
    TestTrue(TEXT("技能测试海岸创建成功"),Ocean->SetClosedCoastline({{-10,-10},{10,-10},{10,10},{-10,10}}));
    Skill->SetActorLocation({100,200,250},true,&Sweep);
    TestEqual(TEXT("闭环内部不产生技能海面命中"),HitCount,1);
    Skill->SetActorLocation({2000,200,500});
    Skill->SetActorLocation({2000,200,250},true,&Sweep);
    TestEqual(TEXT("闭环外侧仍产生技能海面命中"),HitCount,2);
    Detection->StopDetection();
    return true;
}

/** 验证密集浪花分帧执行、最终仍提交遮罩，以及无水下视图时不启用整屏后处理。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanBudgetTest,"LxARPG.World.Ocean.RuntimeBudget",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanBudgetTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("浪花预算测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false;
    Ocean->MaxGridSegments=32;
    Ocean->MaxWaveAmplitude=0;
    Ocean->OceanMaterial=UMaterial::GetDefaultMaterial(MD_Surface);
    Ocean->FoamSamplesPerFrame=16384;
    Ocean->FoamTimeBudgetMs=0.1f;
    Ocean->RebuildOcean();
    AActor* Obstacle=World->SpawnActor<AActor>();
    UBoxComponent* Box=NewObject<UBoxComponent>(Obstacle);
    Obstacle->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(5000,5000,100));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->RegisterComponent();
    Ocean->Tick(1.f/60.f);
    TestTrue(TEXT("高密度采样在预算到达后提前让出主线程"),Ocean->LastFoamSampleCount<Ocean->FoamSamplesPerFrame);
    TestEqual(TEXT("没有水下游戏摄像机时关闭整屏滤镜"),Ocean->UnderwaterPostProcess->BlendWeight,0.f);
    const double Deadline=FPlatformTime::Seconds()+5;
    while(Ocean->CompletedFoamCaptures==0 && FPlatformTime::Seconds()<Deadline) { Ocean->Tick(1.f/60.f); }
    TestTrue(TEXT("受预算限制的浪花最终仍上传完成"),Ocean->CompletedFoamCaptures>0);
    const int32 Completed=Ocean->CompletedFoamCaptures;
    Ocean->bEnableFoam=false;
    Ocean->Tick(1.f/60.f);
    TestEqual(TEXT("关闭浪花后不进行碰撞采样"),Ocean->LastFoamSampleCount,0);
    TestEqual(TEXT("关闭浪花后不上传遮罩"),Ocean->CompletedFoamCaptures,Completed);
    return true;
}

/** 验证凹形海岸孔洞面积、边界、反向绕序、坐标变换、关闭恢复及无效输入保护。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanCoastlineTest,"LxARPG.World.Ocean.ClosedCoastline",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanCoastlineTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("海岸测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false;
    Ocean->CoastMin={-1000,-1000}; Ocean->CoastMax={1000,1000};
    Ocean->MaxWaveAmplitude=0; Ocean->WaveFadeDistance=1000; Ocean->OceanExtend=2000;
    Ocean->MaxGridSegments=32;
    TArray<FVector2D> Outline={{-10,-10},{10,-10},{10,-2},{-2,-2},{-2,10},{-10,10}};
    if(!TestTrue(TEXT("凹形闭环成功生成"),Ocean->SetClosedCoastline(Outline))) return false;
    TestTrue(TEXT("内部属于排除区"),Ocean->IsInsideCoastline({-500,0,0}));
    TestFalse(TEXT("凹入海湾仍有海水"),Ocean->IsInsideCoastline({0,0,0}));
    TestTrue(TEXT("边界属于排除区"),Ocean->IsInsideCoastline({-200,0,0}));
    TestFalse(TEXT("陆地下方不触发水下效果"),Ocean->IsUnderwater({-500,0,-1000}));
    TestTrue(TEXT("凹形海湾水下效果保留"),Ocean->IsUnderwater({0,0,-1000}));
    TestFalse(TEXT("陆地无海面查询命中"),Ocean->HasOceanAtLocation({-500,0,100}));
    const FProcMeshSection* Section=Ocean->OceanMesh->GetProcMeshSection(0);
    if(!TestNotNull(TEXT("孔洞外侧网格"),Section)) return false;
    double Area=0;
    for(int32 I=0;I<Section->ProcIndexBuffer.Num();I+=3)
    {
        const FVector A=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I]].Position;
        const FVector B=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+1]].Position;
        const FVector C=Section->ProcVertexBuffer[Section->ProcIndexBuffer[I+2]].Position;
        const double Cross=FVector::CrossProduct(B-A,C-A).Z;
        if(Cross>=0) { AddError(TEXT("海面三角形绕序错误")); break; }
        Area+=FMath::Abs(Cross)*0.5;
        const FVector Center=Ocean->OceanMesh->GetComponentTransform().TransformPosition((A+B+C)/3);
        if(Ocean->IsInsideCoastline(Center)) { AddError(TEXT("孔洞内仍有海面三角形")); break; }
    }
    TestTrue(TEXT("海面面积等于外矩形减去凹形陆地，不能只按三角形中心粗略剔除"),FMath::Abs(Area-(6000.0*6000.0-256.0*10000.0))<10.0);
    const int32 VertexCount=Section->ProcVertexBuffer.Num();
    Ocean->RebuildOcean();
    TestEqual(TEXT("重复重建保持顶点数"),Ocean->OceanMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),VertexCount);
    AActor* Obstacle=World->SpawnActor<AActor>();
    UBoxComponent* Box=NewObject<UBoxComponent>(Obstacle);
    Obstacle->SetRootComponent(Box); Box->SetBoxExtent(FVector(1000,1000,100));
    Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Box->RegisterComponent();
    TestEqual(TEXT("陆地碰撞体不生成海洋浪花"),Ocean->SampleCollisionFoam(Box,{-500,0,0}),0.f);
    TestTrue(TEXT("海湾碰撞浪花保留"),Ocean->SampleCollisionFoam(Box,{0,0,0})>0.9f);
    Algo::Reverse(Outline);
    TestTrue(TEXT("反向绘制同样支持"),Ocean->SetClosedCoastline(Outline));
    TestTrue(TEXT("反向内部判定一致"),Ocean->IsInsideCoastline({-500,0,0}));
    Ocean->SetActorLocation(FVector(10000,20000,300));
    Ocean->RebuildOcean();
    TestTrue(TEXT("海岸跟随Actor移动"),Ocean->IsInsideCoastline({9500,20000,300}));
    TestFalse(TEXT("原陆地位置已释放"),Ocean->IsInsideCoastline({-500,0,0}));
    Ocean->SetActorLocation(FVector::ZeroVector);
    TestFalse(TEXT("自相交闭环拒绝生成"),Ocean->SetClosedCoastline({{-10,-10},{10,10},{-10,10},{10,-10}}));
    TestEqual(TEXT("无效闭环不遗留旧海面"),Ocean->OceanMesh->GetNumSections(),0);
    TestFalse(TEXT("无效闭环不误报水下"),Ocean->IsUnderwater({0,0,-100}));
    Ocean->bEnableCoastlineExclusion=false; Ocean->RebuildOcean();
    TestTrue(TEXT("关闭排除恢复完整海面"),Ocean->HasOceanAtLocation({-500,0,0}));
    TestTrue(TEXT("重新设定合法海岸恢复"),Ocean->SetClosedCoastline(Outline));
    Ocean->CoastlineSpline->SetClosedLoop(false); Ocean->RebuildOcean();
    TestTrue(TEXT("多边形重建时自动恢复闭环"),Ocean->CoastlineSpline->IsClosedLoop());
    TestTrue(TEXT("自动闭合后海湾仍有海面"),Ocean->HasOceanAtLocation({0,0,0}));
    return true;
}

/** 使用真实视图混合入口检查水下材质只提交给海域中的视角。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanCoastlineViewsTest,"LxARPG.World.Ocean.CoastlineViews",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanCoastlineViewsTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("水下视角测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false;
    Ocean->MaxGridSegments=32;
    Ocean->MaxWaveAmplitude=0;
    Ocean->UnderwaterMaterial=UMaterial::GetDefaultMaterial(MD_PostProcess);
    if(!TestTrue(TEXT("视角测试海岸创建成功"),Ocean->SetClosedCoastline({{-10,-10},{10,-10},{10,10},{-10,10}}))) return false;
    FSceneViewStateReference ViewState;
    ViewState.Allocate(GMaxRHIFeatureLevel);
    /** 经引擎视图接口检查实际进入后处理队列的材质，不依赖水下查询返回值。 */
    auto HasUnderwaterMaterial=[Ocean,&ViewState](const FVector& Location)
    {
        FSceneViewInitOptions Options;
        Options.SetViewRectangle(FIntRect(0,0,64,64));
        Options.ViewRotationMatrix=FMatrix::Identity;
        Options.ProjectionMatrix=FMatrix::Identity;
        Options.ViewOrigin=Location;
        Options.ViewLocation=Location;
        Options.SceneViewStateInterface=ViewState.GetReference();
        FSceneView View(Options);
        Ocean->OverrideBlendableSettings(View,1.f);
        FBlendableEntry* Iterator=nullptr;
        return View.FinalPostProcessSettings.BlendableManager.IterateBlendables<FPostProcessMaterialNode>(Iterator)!=nullptr;
    };
    TestFalse(TEXT("陆地下方视角不提交水下材质"),HasUnderwaterMaterial({0,0,-100}));
    TestTrue(TEXT("外侧海域水下视角提交材质"),HasUnderwaterMaterial({2000,0,-100}));
    TestFalse(TEXT("另一陆地视角不会继承海域滤镜"),HasUnderwaterMaterial({0,0,-100}));
    TestFalse(TEXT("水上视角不提交滤镜"),HasUnderwaterMaterial({2000,0,100}));
    TestFalse(TEXT("有限海域外不提交滤镜"),HasUnderwaterMaterial({100000,0,-100}));
    return true;
}

#if WITH_EDITOR
/** 验证详情点数输入、视口顶点编辑、直线闭合以及构造重跑后轮廓保留。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanPolygonEditingTest,"LxARPG.World.Ocean.PolygonEditing",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanPolygonEditingTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
    if(!TestNotNull(TEXT("多边形编辑测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false; Ocean->MaxGridSegments=32;
    const TArray<FVector2D> Outline={{-10,-10},{10,-10},{10,10},{-10,10}};
    TestTrue(TEXT("矩形海岸生成成功"),Ocean->SetClosedCoastline(Outline));
    Ocean->SetCoastlinePointCount(7);
    TestEqual(TEXT("增加到七个可编辑点"),Ocean->CoastlineSpline->GetNumberOfSplinePoints(),7);
    TestEqual(TEXT("数量属性保持同步"),Ocean->CoastlinePointCount,7);
    for(const FVector2D& Corner:Outline)
    {
        bool bFound=false;
        for(int32 I=0;I<Ocean->CoastlinePointCount;++I)
            bFound|=Ocean->CoastlineSpline->GetLocationAtSplinePoint(I,ESplineCoordinateSpace::Local).Equals(FVector(Corner*100.0,0),0.01);
        TestTrue(TEXT("加点保留原有角点位置"),bFound);
    }
    TestTrue(TEXT("加点后内部排除保持"),Ocean->IsInsideCoastline({900,900,0}));
    Ocean->SetCoastlinePointCount(4);
    TestEqual(TEXT("减点后恢复四个角点"),Ocean->CoastlinePointCount,4);
    TestTrue(TEXT("优先移除共线点保留矩形"),Ocean->IsInsideCoastline({900,900,0}));
    FProperty* CountProperty=FindFProperty<FProperty>(ALxOceanActor::StaticClass(),GET_MEMBER_NAME_CHECKED(ALxOceanActor,CoastlinePointCount));
    if(!TestNotNull(TEXT("可输入的点数属性"),CountProperty)) return false;
    Ocean->PreEditChange(CountProperty);
    Ocean->CoastlinePointCount=6;
    FPropertyChangedEvent ChangedEvent(CountProperty,EPropertyChangeType::ValueSet);
    Ocean->PostEditChangeProperty(ChangedEvent);
    TestEqual(TEXT("详情面板输入直接改变控制点数量"),Ocean->CoastlineSpline->GetNumberOfSplinePoints(),6);
    Ocean->RerunConstructionScripts();
    TestEqual(TEXT("重新构造不丢失已编辑点数"),Ocean->CoastlinePointCount,6);
    TestTrue(TEXT("重新构造保留轮廓"),Ocean->IsInsideCoastline({900,900,0}));
    Ocean->SetClosedCoastline(Outline);
    Ocean->CoastlineSpline->AddSplinePoint(FVector(-1000,0,0),ESplineCoordinateSpace::Local);
    Ocean->CoastlineSpline->SetSplinePointType(0,ESplinePointType::Curve);
    Ocean->CoastlineSpline->SetClosedLoop(false);
    Ocean->RebuildOcean();
    TestEqual(TEXT("视口增加顶点后数量同步"),Ocean->CoastlinePointCount,5);
    TestTrue(TEXT("海岸始终闭合"),Ocean->CoastlineSpline->IsClosedLoop());
    for(int32 I=0;I<Ocean->CoastlinePointCount;++I)
        TestEqual(TEXT("每条边均为直线"),Ocean->CoastlineSpline->GetSplinePointType(I),ESplinePointType::Linear);
    Ocean->SetCoastlinePointCount(1);
    TestEqual(TEXT("点数下限为三"),Ocean->CoastlinePointCount,3);
    return true;
}

/** 验证编辑器开关同时控制材质时钟和水面高度，游戏世界不受影响。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanEditorWavePreviewTest,"LxARPG.World.Ocean.EditorWavePreview",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanEditorWavePreviewTest::RunTest(const FString& Parameters)
{
    UWorld* EditorWorld=UWorld::CreateWorld(EWorldType::Editor,false);
    if(!TestNotNull(TEXT("海浪预览测试世界"),EditorWorld)) return false;
    ON_SCOPE_EXIT { EditorWorld->DestroyWorld(false); };
    ALxOceanActor* Ocean=EditorWorld->SpawnActor<ALxOceanActor>();
    Ocean->bAutoDetectLandscape=false; Ocean->MaxGridSegments=32; Ocean->bEnableFoam=false;
    Ocean->OceanMaterial=UMaterial::GetDefaultMaterial(MD_Surface);
    Ocean->SetActorLocation(FVector(0,0,123)); Ocean->RebuildOcean();
    UMaterialInstanceDynamic* Material=Cast<UMaterialInstanceDynamic>(Ocean->OceanMesh->GetMaterial(0));
    if(!TestNotNull(TEXT("海面动态材质"),Material)) return false;
    const int32 VertexCount=Ocean->OceanMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num();
    Ocean->Tick(0.25f);
    const float InitialTime=Material->K2_GetScalarParameterValue(TEXT("WaveTime"));
    TestTrue(TEXT("开启预览时海浪时钟推进"),InitialTime>0);
    Ocean->bPreviewWavesInEditor=false;
    Ocean->Tick(1.f);
    TestEqual(TEXT("关闭预览时材质振幅为零"),Material->K2_GetVectorParameterValue(TEXT("WaveSettings")).R,0.f);
    TestEqual(TEXT("关闭预览时材质时钟静止"),Material->K2_GetScalarParameterValue(TEXT("WaveTime")),0.f);
    TestEqual(TEXT("水面查询为平均海平面"),Ocean->GetSurfaceHeight({100,200,0}),123.0);
    TestTrue(TEXT("海平面继续显示"),Ocean->OceanMesh->IsVisible());
    TestEqual(TEXT("关闭预览不移除海面网格"),Ocean->OceanMesh->GetProcMeshSection(0)->ProcVertexBuffer.Num(),VertexCount);
    TestEqual(TEXT("原海浪振幅参数保留"),Ocean->MaxWaveAmplitude,60.f);
    Ocean->Tick(1.f);
    TestEqual(TEXT("连续帧仍为静止海平面"),Ocean->GetSurfaceHeight({100,200,0}),123.0);
    Ocean->bPreviewWavesInEditor=true; Ocean->Tick(0.25f);
    TestEqual(TEXT("重新开启恢复材质振幅"),Material->K2_GetVectorParameterValue(TEXT("WaveSettings")).R,60.f);
    TestTrue(TEXT("关闭期间不推进海浪时钟"),FMath::IsNearlyEqual(Material->K2_GetScalarParameterValue(TEXT("WaveTime")),InitialTime+0.25f));
    UWorld* GameWorld=UWorld::CreateWorld(EWorldType::Game,false);
    if(!TestNotNull(TEXT("游戏海浪测试世界"),GameWorld)) return false;
    ON_SCOPE_EXIT { GameWorld->DestroyWorld(false); };
    ALxOceanActor* GameOcean=GameWorld->SpawnActor<ALxOceanActor>();
    GameOcean->bAutoDetectLandscape=false; GameOcean->MaxGridSegments=32; GameOcean->bEnableFoam=false;
    GameOcean->bPreviewWavesInEditor=false;
    GameOcean->OceanMaterial=UMaterial::GetDefaultMaterial(MD_Surface); GameOcean->RebuildOcean();
    UMaterialInstanceDynamic* GameMaterial=Cast<UMaterialInstanceDynamic>(GameOcean->OceanMesh->GetMaterial(0));
    if(!TestNotNull(TEXT("游戏海面动态材质"),GameMaterial)) return false;
    const double Before=GameOcean->GetSurfaceHeight({100,200,0});
    GameOcean->Tick(0.25f);
    TestEqual(TEXT("游戏仍有原海浪振幅"),GameMaterial->K2_GetVectorParameterValue(TEXT("WaveSettings")).R,60.f);
    TestTrue(TEXT("游戏时钟继续推进"),GameMaterial->K2_GetScalarParameterValue(TEXT("WaveTime"))>0);
    TestTrue(TEXT("游戏水面继续运动"),FMath::Abs(GameOcean->GetSurfaceHeight({100,200,0})-Before)>0.001);
    return true;
}
#endif

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeInfo.h"
/** 导入微型真实地形，验证自动外接矩形及地形接触泡沫。 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLxOceanLandscapeTest,"LxARPG.World.Ocean.Landscape",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FLxOceanLandscapeTest::RunTest(const FString& Parameters)
{
    UWorld* World=UWorld::CreateWorld(EWorldType::Editor,false);
    if(!TestNotNull(TEXT("地形测试世界"),World)) return false;
    ON_SCOPE_EXIT { World->DestroyWorld(false); };
    ALandscape* Land=World->SpawnActor<ALandscape>(FVector(-1000,-2000,100),FRotator::ZeroRotator);
    Land->SetActorScale3D({100,100,100});
    TArray<uint16> Heights; Heights.Init(32768,64*64);
    TMap<FGuid,TArray<uint16>> HeightLayers; HeightLayers.Add(FGuid(),MoveTemp(Heights));
    TMap<FGuid,TArray<FLandscapeImportLayerInfo>> Layers; Layers.Add(FGuid(),{});
    Land->Import(FGuid::NewGuid(),0,0,63,63,1,63,HeightLayers,TEXT(""),Layers,
        ELandscapeImportAlphamapType::Additive,TArrayView<const FLandscapeLayer>());
    ALxOceanActor* Ocean=World->SpawnActor<ALxOceanActor>();
    TestTrue(TEXT("自动找到真实地形"),Ocean->DetectCoast());
    TestTrue(TEXT("外接范围包含地形最小角"),Ocean->CoastMin.Equals({-1000,-2000},1));
    TestTrue(TEXT("外接范围包含地形最大角"),Ocean->CoastMax.Equals({5300,4300},1));
    TInlineComponentArray<ULandscapeHeightfieldCollisionComponent*> Components;
    Land->GetComponents(Components);
    TestTrue(TEXT("原生地形碰撞已生成"),Components.Num()>0);
    if(Components.Num()>0)
        TestTrue(TEXT("地形与水相交产生浪花"),Ocean->SampleCollisionFoam(Components[0],{100,100,100})>0.8f);
    return true;
}
#endif

#endif
