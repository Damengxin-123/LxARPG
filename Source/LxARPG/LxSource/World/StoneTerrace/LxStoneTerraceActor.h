#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxStoneTerraceActor.generated.h"
class UProceduralMeshComponent;
class ULxGeneratedMeshRenderComponent;
class UMaterialInterface;
class ALxSplineRoadActor;
class ULxStoneTerraceHandleComponent;

/** 矩形台地的连接开口；边编号从下边开始逆时针排列。 */
USTRUCT(meta=(DisplayName="台地连接", Category="石质台地"))
struct FLxTerraceOpening
{
 GENERATED_BODY()
 /** 连接对象，支持台地和道路派生类型。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地", meta=(DisplayName="连接对象"))
 TObjectPtr<AActor> Other=nullptr;
 /** 开口所在的矩形边。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地", meta=(DisplayName="连接边"))
 int32 Side=0;
 /** 从该边起点量取的开口范围。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地", meta=(DisplayName="开口范围"))
 FVector2D Interval=FVector2D::ZeroVector;
 /** 连接持有方的四点填缝轮廓，采用局部坐标。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地", meta=(DisplayName="连接轮廓"))
 TArray<FVector> Bridge;
 /** 比较连接几何以跳过无变化对象的事务。 */
 bool operator==(const FLxTerraceOpening& R) const { return Other==R.Other && Side==R.Side && Interval==R.Interval && Bridge==R.Bridge; }
};

/** 仅用长宽高编辑的矩形石砖台地，自动去除拼接边并连接道路。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动石质台地", Category="海港城市"))
class LXARPG_API ALxStoneTerraceActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建程序化网格、碰撞和渲染缓存。 */
 ALxStoneTerraceActor();
 /** 参数变化后同步连接与几何。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 删除后恢复相邻对象包边。 */
 virtual void Destroyed() override;
#if WITH_EDITOR
 /** 交互输入保留组件注册，避免每次拖动重跑蓝图。 */
 virtual void PreEditChange(FProperty* Property) override;
 /** 属性编辑时定时更新轻量台地。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 移动期间定时重新求解连接。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后恢复连接状态。 */
 virtual void PostEditUndo() override;
#endif
 /** 重新检查相邻道路及台地。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石质台地|生成", meta=(DisplayName="重建台地"))
 void RebuildTerrace();
 /** 调整矩形长宽高，底面保持在演员原点。 */
 UFUNCTION(BlueprintCallable, Category="石质台地|尺寸", meta=(DisplayName="设置台地尺寸"))
 void SetTerraceDimensions(float Length, float Width, float Height);
 /** 沿局部X轴的总长度，视口红色手柄与此值同步；单位米。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|尺寸", meta=(DisplayName="台地长度", ClampMin="1.00", Units="m", MaxFractionalDigits="2"))
 float LengthMeters=20.00f;
 /** 沿局部Y轴的总宽度，视口绿色手柄与此值同步；单位米。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|尺寸", meta=(DisplayName="台地宽度", ClampMin="1.00", Units="m", MaxFractionalDigits="2"))
 float WidthMeters=12.00f;
 /** 以米设置矩形长宽，自动量化到两位小数并保持中心不变。 */
 UFUNCTION(BlueprintCallable, Category="石质台地|尺寸", meta=(DisplayName="设置台地长宽（米）"))
 void SetFootprintMeters(float Length, float Width);
 /** 兼容旧关卡及旧接口的厘米尺寸缓存，不再作为详情面板编辑入口。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="石质台地|兼容", meta=(DisplayName="旧版台地长宽（厘米）", EditCondition="false", EditConditionHides))
 FVector2D Size=FVector2D(2000,1200);
 /** 顶面相对于演员底部的高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|尺寸", meta=(DisplayName="台地高度", ClampMin="1", Units="cm"))
 float TerraceHeight=100;
 /** 外角半径，拼接到角点时自动使用直角。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|尺寸", meta=(DisplayName="拐角圆角半径", ClampMin="0", Units="cm"))
 float CornerRadius=0;
 /** 包边向矩形内部占用的宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石块包边", meta=(DisplayName="边石宽度", ClampMin="5", Units="cm"))
 float BorderWidth=45;
 /** 包边高于铺砖顶面的尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石块包边", meta=(DisplayName="边石凸起高度", ClampMin="0", Units="cm"))
 float BorderHeight=12;
 /** 单块边石的目标长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石块包边", meta=(DisplayName="边石长度", ClampMin="30", Units="cm"))
 float BorderLength=150;
 /** 大边石顶面周圈的真实倒角，单位米；编辑预览省略微小倒角。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石块包边", meta=(DisplayName="边石倒角", ClampMin="0.00", ClampMax="0.10", Units="m", MaxFractionalDigits="2"))
 float BorderBevelMeters=0.02f;
 /** 错缝铺装石砖的长宽。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石砖铺装", meta=(DisplayName="石砖长宽", ClampMin="20", Units="cm"))
 FVector2D BrickSize=FVector2D(100,60);
 /** 浅灰砖缝的宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|石砖铺装", meta=(DisplayName="砖缝宽度", ClampMin="0", Units="cm"))
 float JointWidth=3.0f;
 /** 自动拼接台地直边与道路端点。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|自动连接", meta=(DisplayName="自动连接"))
 bool bAutoConnect=true;
 /** 自动填补边缘间隙的最大距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|自动连接", meta=(DisplayName="自动连接范围", ClampMin="0", Units="cm"))
 float AutoConnectDistance=100;
 /** 道路允许的高差；台地之间要求顶面平齐。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|自动连接", meta=(DisplayName="道路最大连接高差", ClampMin="0", Units="cm"))
 float AutoConnectHeight=30;
 /** 自动维护的连接记录。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地|自动连接", meta=(DisplayName="台地连接"))
 TArray<FLxTerraceOpening> Connections;
 /** 支持顶点颜色的浅灰石砖材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|材质", meta=(DisplayName="石砖材质"))
 TObjectPtr<UMaterialInterface> PavingMaterial;
 /** 灰白台身与边石材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|材质", meta=(DisplayName="台身与边石材质"))
 TObjectPtr<UMaterialInterface> StoneMaterial;
 /** 顶面、边石和连接片的简单碰撞。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质台地|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision=true;
 /** 非法尺寸或不支持的连接变换说明。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="石质台地|诊断", meta=(DisplayName="生成说明"))
 FString GenerationDiagnostic;
 /** 程序化源网格与碰撞组件。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质台地|组件", meta=(DisplayName="台地网格"))
 TObjectPtr<UProceduralMeshComponent> TerraceMesh;
 /** 保存时完成静态渲染优化。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质台地|组件", meta=(DisplayName="优化渲染网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> OptimizedMesh;
 /** 道路求解后检查台地；回调检查或占用道路端口。 */
 static void SolveConnections(AActor& Changed, bool bRemoving, TFunctionRef<bool(ALxSplineRoadActor&,int32,bool,float)> RoadPort);
 /** 供编辑器绘制两轴手柄，不参与渲染和碰撞。 */
 UPROPERTY(VisibleAnywhere, Category="石质台地|组件", meta=(DisplayName="台地尺寸手柄"))
 TObjectPtr<ULxStoneTerraceHandleComponent> DimensionHandles;
private:
 /** 同步米制长宽和旧厘米缓存，保留已有台地大小。 */
 void SynchronizeDimensions();
 /** 上次同步的厘米尺寸，用于识别旧接口主动修改。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已同步台地尺寸"))
 FVector2D SynchronizedSize=FVector2D::ZeroVector;
 /** 生成台身、错缝砖面、分块包边及连接片。 */
 void BuildGeometry();
 /** 上次几何输入，跳过未变化的邻居。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="台地形态快照"))
 TArray<uint8> GeometryInputs;
};
