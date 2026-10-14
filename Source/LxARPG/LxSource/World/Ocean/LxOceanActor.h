#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/BlendableInterface.h"
#include "LxOceanActor.generated.h"
class UBoxComponent;
class USplineComponent;
class UProceduralMeshComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UTexture2D;

/** 与海面材质一致的波浪数学，单位为世界厘米与秒。 */
namespace LxOceanMath
{
    /** 矩形内部距离为零，外部使用到边界的欧氏距离。 */
    LXARPG_API double CoastDistance(FVector2D Point, FVector2D Min, FVector2D Max);
    /** 五次平滑衰减，到上限精确归零。 */
    LXARPG_API double Attenuation(double Distance, double FadeDistance);
    /** 权重和为一的三组波，绝对偏移不超过振幅。 */
    LXARPG_API double Wave(FVector2D Point, double Time, double Amplitude, double Length, double Speed, double Direction);
}
/** 非阻挡海面重叠通知，命中点位于平均海平面。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FLxOceanOverlapEvent, AActor*, OtherActor, UPrimitiveComponent*, OtherComponent, FVector, SurfacePoint);

/** 地形海岸、程序化海浪、碰撞浪花及水下滤镜；Actor 的 Z 是平均海平面。 */
UCLASS(Blueprintable, meta=(DisplayName="程序化海洋"))
class LXARPG_API ALxOceanActor : public AActor, public IBlendableInterface
{
    GENERATED_BODY()
public:
    /** 创建海面、查询碰撞、后处理组件。 */
    ALxOceanActor();
    /** 编辑参数后排队刷新轻量预览，游戏世界生成完整海面。 */
    virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
    /** 详情面板修改点数后同步多边形控制点，随后执行常规构造更新。 */
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    /** 保存关卡或世界分区外部 Actor 前提交最新完整海面。 */
    virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
    /** 合并连续编辑请求，每零点零五秒最多刷新一次保留真实海岸轮廓的预览。 */
    void RequestOceanPreview();
    /** 保存和运行入口使用，取消排队预览并按最新参数生成完整海面；未修改时复用。 */
    void FinalizeOcean();
    /** 游戏开始时应用已保存的海岸范围。 */
    virtual void BeginPlay() override;
    /** 同步波浪时钟并分帧更新浪花。 */
    virtual void Tick(float DeltaSeconds) override;
    /** 允许编辑器实时预览。 */
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    /** 按独立视图判断海岸排除区，仅向有效海域视图提交水下材质。 */
    virtual void OverrideBlendableSettings(FSceneView& View, float Weight) const override;
    /** 使用局部平面坐标（米，保留两位小数）设置闭合折线并重建海洋，至少三个点。 */
    UFUNCTION(BlueprintCallable, Category="海洋|海岸线", meta=(DisplayName="设置闭合海岸线", AutoCreateRefTerm="LocalPointsMeters"))
    bool SetClosedCoastline(const TArray<FVector2D>& LocalPointsMeters);
    /** 调整多边形控制点数；增加时细分最长边，减少时优先移除对轮廓影响最小的点。 */
    UFUNCTION(BlueprintCallable, Category="海洋|海岸线", meta=(DisplayName="设置海岸线控制点数量"))
    void SetCoastlinePointCount(int32 NewPointCount);
    /** 查询世界位置的平面投影是否落在闭环内部或边界上。 */
    UFUNCTION(BlueprintPure, Category="海洋|海岸线", meta=(DisplayName="是否位于海岸排除区"))
    bool IsInsideCoastline(FVector WorldPoint) const;
    /** 查询该世界位置是否存在海面，不受位置高度影响。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="该位置是否有海面"))
    bool HasOceanAtLocation(FVector WorldPoint) const;
    /** 自动收集地形完整范围；世界分区在编辑器包含未加载代理。 */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="海洋|生成", meta=(DisplayName="重新检测海岸"))
    bool DetectCoast();
    /** 蓝图修改参数后调用；编辑器生成轻量预览，游戏生成完整网格。 */
    UFUNCTION(BlueprintCallable, CallInEditor, Category="海洋|生成", meta=(DisplayName="重建海洋"))
    void RebuildOcean();
    /** 获取可见波动水面高度，不含浮力模拟。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="获取波动水面高度"))
    double GetSurfaceHeight(FVector WorldPoint) const;
    /** 查询海浪衰减权重。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="获取海浪衰减"))
    double GetWaveAttenuation(FVector WorldPoint) const;
    /** 查询点是否位于有限海洋范围和波动水面下方。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="是否位于水下"))
    bool IsUnderwater(FVector WorldPoint) const;
    /** 技能碰撞使用平均海平面，将位置投影到该平面。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="投影到海平面"))
    FVector ProjectToSeaLevel(FVector WorldPoint) const;
    /** 查询指定碰撞体与水面采样点的接触权重，用于调试碰撞浪花。 */
    UFUNCTION(BlueprintPure, Category="海洋|查询", meta=(DisplayName="查询碰撞浪花权重"))
    float SampleCollisionFoam(UPrimitiveComponent* Component, FVector Position) const;

    /** 在视口编辑的闭合海岸线；只使用世界 XY 投影，控制点以直线连接为多边形，禁止自相交。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海洋|组件", meta=(DisplayName="闭合海岸线"))
    TObjectPtr<USplineComponent> CoastlineSpline;
    /** 默认关闭以兼容现有关卡；启用后闭环内部不生成海面且不计算海洋效果。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海岸线", meta=(DisplayName="启用海岸排除区"))
    bool bEnableCoastlineExclusion = false;
    /** 详情面板直接输入点数；也支持在视口增删顶点，蓝图通过设置接口调整。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="海洋|海岸线", meta=(DisplayName="海岸线控制点数量", ClampMin="3", ClampMax="2048"))
    int32 CoastlinePointCount = 4;
    /** 只影响非游戏视图；关闭后海面保持在平均海平面，运行游戏仍使用原海浪参数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="编辑时显示海浪"))
    bool bPreviewWavesInEditor = true;
    /** 可见海面不产生物理阻挡。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海洋|组件", meta=(DisplayName="海面网格"))
    TObjectPtr<UProceduralMeshComponent> OceanMesh;
    /** 顶面对应 Actor Z 的薄层查询碰撞。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海洋|组件", meta=(DisplayName="海面重叠层"))
    TObjectPtr<UBoxComponent> SurfaceTrigger;
    /** 材质为每个摄像机分别判断水下状态。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海洋|组件", meta=(DisplayName="水下后处理"))
    TObjectPtr<UPostProcessComponent> UnderwaterPostProcess;
    /** 编辑器自动更新，游戏优先使用保存值避免流送缩小海岸。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海岸", meta=(DisplayName="自动检测地形"))
    bool bAutoDetectLandscape = true;
    /** 世界坐标最小角，无地形时也可手动设置。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海岸", meta=(DisplayName="海岸最小点（厘米）"))
    FVector2D CoastMin = FVector2D(-5000, -5000);
    /** 世界坐标最大角。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海岸", meta=(DisplayName="海岸最大点（厘米）"))
    FVector2D CoastMax = FVector2D(5000, 5000);
    /** 海面向海岸外延伸的距离，至少覆盖衰减区。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海岸", meta=(DisplayName="海面外延距离（厘米）", ClampMin="1000", ClampMax="1000000"))
    float OceanExtend = 50000;
    /** 偏离平均海面的最大高度，非峰谷高度差。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="近海最大振幅（厘米）", ClampMin="0", ClampMax="2000"))
    float MaxWaveAmplitude = 60;
    /** 从矩形向外计算，到此距离完全平静。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="海浪衰减距离（厘米）", ClampMin="1", ClampMax="1000000"))
    float WaveFadeDistance = 15000;
    /** 主波波长。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="海浪波长（厘米）", ClampMin="100", ClampMax="100000"))
    float WaveLength = 2400;
    /** 主波速度，零值冻结波形。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="海浪速度（厘米每秒）", ClampMin="0", ClampMax="10000"))
    float WaveSpeed = 180;
    /** 世界 XY 传播角，零度为 X 正方向。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|海浪", meta=(DisplayName="海浪方向（度）"))
    float WaveDirection = 25;
    /** 有浪区域期望网格间距。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|生成", meta=(DisplayName="网格间距（厘米）", ClampMin="50", ClampMax="5000"))
    float GridSpacing = 200;
    /** 每轴网格预算，避免大地形产生失控顶点数量。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|生成", meta=(DisplayName="每轴最大分段", ClampMin="32", ClampMax="512"))
    int32 MaxGridSegments = 256;
    /** 接触泡沫的世界宽度。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="浪花宽度（厘米）", ClampMin="5", ClampMax="1000"))
    float FoamWidth = 100;
    /** 开启时检测所有启用碰撞的实体。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="启用碰撞浪花"))
    bool bEnableFoam = true;
    /** 观察点附近检测半径，避免每帧查询整个海域。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="浪花检测半径（厘米）", ClampMin="500", ClampMax="50000"))
    float FoamCaptureRadius = 4000;
    /** 小构件需要较高分辨率或较小检测半径。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="浪花遮罩分辨率", ClampMin="32", ClampMax="256"))
    int32 FoamResolution = 128;
    /** 每帧最大碰撞采样数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="每帧浪花采样预算", ClampMin="128", ClampMax="16384"))
    int32 FoamSamplesPerFrame = 2048;
    /** 碰撞采样每帧的主线程时间预算；单次物理查询不能中断，剩余任务顺延。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|性能", meta=(DisplayName="每帧浪花耗时预算", ClampMin="0.1", ClampMax="4", Units="ms"))
    float FoamTimeBudgetMs = 0.5f;
    /** 最近一帧真正执行的碰撞采样数。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海洋|性能", meta=(DisplayName="本帧浪花采样数"))
    int32 LastFoamSampleCount = 0;
    /** 最近一帧浪花更新的主线程耗时，包含候选收集与纹理提交。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海洋|性能", meta=(DisplayName="本帧浪花耗时", Units="ms"))
    float LastFoamUpdateMilliseconds = 0;
    /** 已完成并上传的浪花遮罩批次数，用于确认分帧任务仍持续提交。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海洋|性能", meta=(DisplayName="已提交浪花批次"))
    int32 CompletedFoamCaptures = 0;
    /** 完整一轮采样后的最小等待秒数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="浪花更新间隔（秒）", ClampMin="0.05", ClampMax="5"))
    float FoamUpdateInterval = 0.15f;
    /** 默认跟随本地摄像机，编辑器或特殊视角可以指定此目标。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|浪花", meta=(DisplayName="浪花观察目标"))
    TObjectPtr<AActor> FoamObserver;
    /** 水下滤镜开关。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|水下", meta=(DisplayName="启用水下滤镜"))
    bool bEnableUnderwater = true;
    /** 下潜至该深度时滤镜完全生效，水面过渡平滑。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|水下", meta=(DisplayName="水下过渡深度（厘米）", ClampMin="1", ClampMax="2000"))
    float UnderwaterTransitionDepth = 80;
    /** 水下距离雾密度，单位为厘米的倒数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|水下", meta=(DisplayName="水下雾密度", ClampMin="0", ClampMax="0.01"))
    float UnderwaterFogDensity = 0.00035f;
    /** 明亮海蓝水色。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|材质", meta=(DisplayName="海水颜色"))
    FLinearColor WaterColor = FLinearColor(0.025f, 0.28f, 0.38f);
    /** 配套工具建立的程序化海面材质。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|材质", meta=(DisplayName="海面材质"))
    TObjectPtr<UMaterialInterface> OceanMaterial;
    /** 按摄像机位置计算的水下屏幕材质。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|材质", meta=(DisplayName="水下滤镜材质"))
    TObjectPtr<UMaterialInterface> UnderwaterMaterial;
    /** 平均海面下方的重叠层厚度，不阻挡移动。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海洋|交互", meta=(DisplayName="海面重叠层厚度（厘米）", ClampMin="1", ClampMax="100"))
    float SurfaceTriggerThickness = 10;
    /** 现有技能检测可自动将同一重叠转为场景命中。 */
    UPROPERTY(BlueprintAssignable, Category="海洋|交互", meta=(DisplayName="物体进入海面"))
    FLxOceanOverlapEvent OnSeaSurfaceOverlap;
    /** 范围和网格预算提示。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海洋|生成", meta=(DisplayName="生成提示"))
    FString GenerationDiagnostic;
    /** 编辑预览尚未提交完整网格，保存和运行前会自动完成。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海洋|生成", meta=(DisplayName="等待生成完整海面"))
    bool bPendingFullMeshBuild = true;
    /** 当前网格使用编辑预览密度；真实海岸轮廓保持不变。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海洋|生成", meta=(DisplayName="正在显示轻量海面预览"))
    bool bUsingEditorPreview = false;
    /** 已提交的轻量预览次数，用于检查连续编辑的合并效果。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, AdvancedDisplay, Category="海洋|性能", meta=(DisplayName="海面预览生成次数"))
    int32 PreviewMeshBuildCount = 0;
    /** 完整网格生成次数，用于确认拖动期间没有执行完整生成。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, AdvancedDisplay, Category="海洋|性能", meta=(DisplayName="完整海面生成次数"))
    int32 FullMeshBuildCount = 0;

private:
    /** 共用生成逻辑，预览限制每轴分段且省略海岸边界细分和碰撞浪花采样。 */
    void BuildOcean(bool bPreview);
    /** 已排队的刷新由弱引用定时器提交，保存或显式重建时取消。 */
    bool bOceanPreviewQueued = false;
    /** 请求序号使保存前遗留的定时器无法提前提交保存后的新编辑。 */
    uint64 OceanPreviewRequestSerial = 0;
    /** 上次完整生成的参数与控制点快照，跳过未修改对象的重复保存。 */
    TArray<uint8> CompletedOceanInputs;
    /** 调整样条组件中的多边形顶点，保留既有位置并标记为可保存的实例编辑。 */
    void ResizeCoastlinePoints(int32 NewPointCount);
    /** 自动闭合并强制直线连接，同时使数量参数与视口编辑结果一致。 */
    void NormalizeCoastlinePolygon();
    /** 判断当前世界是否需要动态海浪；游戏世界不受编辑器预览开关影响。 */
    bool ShouldAnimateWaves() const;
    /** 缓存海岸线世界平面多边形，检查闭环、退化与自相交。 */
    bool CacheCoastline();
    /** 生成带闭合海岸孔洞的约束三角网格，保留外侧网格密度。 */
    bool BuildCoastlineMesh(const TArray<double>& X, const TArray<double>& Y, TArray<FVector2D>& Points, TArray<int32>& Indices, bool bPreview) const;
    /** 重建时缓存的海岸折线，不随地形流送变化。 */
    TArray<FVector2D> CoastlinePolygon;
    /** 重建后的海岸外接范围，用于快速跳过陆地判定。 */
    FBox2D CoastlineBounds = FBox2D(ForceInit);
    /** 无效海岸线时暂停海洋，避免失败后重新淹没陆地区域。 */
    bool bCoastlineBuildValid = true;
    /** 转发原生重叠事件。 */
    UFUNCTION(Category="海洋|交互", meta=(DisplayName="处理海面重叠"))
    void HandleSurfaceOverlap(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bSweep, const FHitResult& Hit);
    /** 同步两个材质的海岸、海浪及水下参数。 */
    void UpdateMaterialParameters();
    /** 按固定预算更新碰撞遮罩。 */
    void UpdateCollisionFoam(float DeltaSeconds);
    /** 创建观察点附近的新采样批次。 */
    void BeginFoamCapture();
    /** 上传已完成遮罩，同时切换纹理世界坐标。 */
    void CommitFoamCapture();
    /** 海面运行时材质。 */
    UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay, Category="海洋|运行时", meta=(DisplayName="海面动态材质")) TObjectPtr<UMaterialInstanceDynamic> OceanMID;
    /** 水下运行时材质。 */
    UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay, Category="海洋|运行时", meta=(DisplayName="水下动态材质")) TObjectPtr<UMaterialInstanceDynamic> UnderwaterMID;
    /** 自动生成的碰撞泡沫纹理。 */
    UPROPERTY(Transient, VisibleInstanceOnly, AdvancedDisplay, Category="海洋|运行时", meta=(DisplayName="碰撞浪花纹理")) TObjectPtr<UTexture2D> FoamTexture;
    /** 编辑器保存的完整海岸标记。 */
    UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category="海洋|运行时", meta=(DisplayName="已保存完整海岸")) bool bHasDetectedCoast = false;
    /** 有限海洋世界矩形。 */
    FBox2D OceanBounds;
    /** CPU 与材质共享时钟。 */
    double WaveTime = 0;
    /** 浪花批次之间的等待秒数。 */
    float FoamWait = 0;
    /** 本批次碰撞候选对象。 */
    TArray<TWeakObjectPtr<UPrimitiveComponent>> FoamCandidates;
    /** 上传前暂存像素。 */
    TArray<FColor> FoamPixels;
    /** 本批次遮罩最小角。 */
    FVector2D CaptureMin = FVector2D::ZeroVector;
    /** 候选索引；负一表示当前空闲。 */
    int32 CaptureComponentIndex = -1;
    /** 当前候选包围盒中的像素游标。 */
    int32 CapturePixelIndex = 0;
};
