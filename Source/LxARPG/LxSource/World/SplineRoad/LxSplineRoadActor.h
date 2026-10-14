// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxRoadConnectionSolver.h"
#include "LxSplineRoadActor.generated.h"
class USplineComponent;
class UProceduralMeshComponent;
class ULxGeneratedMeshRenderComponent;
class UMaterialInterface;
class ALxSplineRoadActor;

/** 道路及其附属结构共用的采样截面，避免曲线和折点接缝错位。 */
struct FLxRoadSampleFrame
{
 /** 中心线局部位置。 */
 FVector Position;
 /** 含斜接补偿的横向向量。 */
 FVector Right;
 /** 中心线累计距离。 */
 float Distance;
 /** 路宽变化的横向补偿。 */
 float WidthOffset=0;
};

/** 同一连接在两条道路中显示并同步的参数。 */
USTRUCT(BlueprintType, meta=(DisplayName="道路连接参数"))
struct FLxRoadConnectionSettings
{
 GENERATED_BODY()
 /** 对方道路对象，用于查找共享连接。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="道路连接", meta=(DisplayName="连接道路"))
 TObjectPtr<ALxSplineRoadActor> OtherRoad=nullptr;
 /** 使用编辑器对象名标识连接。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="道路连接", meta=(DisplayName="对方道路名称"))
 FString RoadName;
 /** 当前连接是否为丁字路口。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="道路连接", meta=(DisplayName="丁字路口"))
 bool bJunction=false;
 /** 零按道路实际宽度衔接；非零调整连接片内部宽度，不改变道路端面。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="道路连接", meta=(DisplayName="连接路面宽度（零为自动）", ClampMin="0", Units="cm"))
 float Width=0;
 /** 丁字路口的圆角，零表示直角。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="道路连接", meta=(DisplayName="转角半径", ClampMin="0", Units="cm", EditCondition="bJunction"))
 float Radius=150;
 /** 连接片内的过渡长度上限；零自动利用可用间隙，不移动道路端点。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="道路连接", meta=(DisplayName="连接过渡长度（零为自动）", ClampMin="0", Units="cm", EditCondition="bJunction"))
 float Length=0;
 /** 本路连接端口：零一起终点，二三左右侧。 */
 UPROPERTY()
 int32 LocalPort=0;
 /** 对方连接端口。 */
 UPROPERTY()
 int32 OtherPort=0;
};

/** 沿三维中心样条生成路面和倒角路缘石；不依赖预制静态网格体。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="海港样条道路", Category="海港城市"))
class LXARPG_API ALxSplineRoadActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建道路中心线与程序化网格组件。 */
 ALxSplineRoadActor();
 /** 带堤身的道路优先持有端点连接片，以便同时生成连接下方支撑。 */
 virtual int32 GetRoadConnectionPriority() const { return 0; }
 /** 编辑参数后更新网格。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 组件注册后锁定内部网格变换，兼容旧关卡中已保存的偏移。 */
 virtual void PostRegisterAllComponents() override;
 /** 旧关卡首次运行时自动迁移已保存的过密碰撞，当前版本直接复用。 */
 virtual void BeginPlay() override;
 /** 销毁前移除尚未执行的编辑预览更新。 */
 virtual void BeginDestroy() override;
 /** 沿三维中心线重建道路，保留控制点高度。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="海港道路|生成", meta=(DisplayName="重建道路"))
 void RebuildRoad();
 /** 清空网格和碰撞，保留样条配置。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="海港道路|生成", meta=(DisplayName="清空道路"))
 virtual void ClearRoad();
 /** 是否参与邻近道路连接；关闭后恢复原始中心线生成结果。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|自动连接", meta=(DisplayName="自动连接邻近道路"))
 bool bAutoConnect=true;
 /** 两端点或端点到主路边缘的最大水平距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|自动连接", meta=(DisplayName="自动连接范围", ClampMin="0", Units="cm", EditCondition="bAutoConnect"))
 float AutoConnectDistance=200;
 /** 超过此高差不连接，避免上下层道路误接。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|自动连接", meta=(DisplayName="最大连接高差", ClampMin="0", Units="cm", EditCondition="bAutoConnect"))
 float AutoConnectHeight=30;
 /** 自动维护的共享连接；数组不可手动增删，可从任意道路修改尺寸。 */
 UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadOnly, Category="海港道路|自动连接", meta=(DisplayName="道路连接", TitleProperty="RoadName"))
 TArray<FLxRoadConnectionSettings> RoadConnections;
 /** 接近但不满足角度、空间或缩放要求时的说明。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海港道路|自动连接", meta=(DisplayName="连接说明"))
 FString ConnectionDiagnostic;
 /** 通过蓝图修改共享连接并重建双方。 */
 UFUNCTION(BlueprintCallable, Category="海港道路|自动连接", meta=(DisplayName="设置道路连接参数"))
 void SetRoadConnectionParameters(int32 ConnectionIndex, float Width, float Radius, float Length);
 /** 清除旧关卡独立生成的手动支路，使对应侧可以自动连接。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="海港道路|自动连接", meta=(DisplayName="清除旧版手动路口"))
 void ClearLegacyJunctions();
 /** 删除道路时恢复邻居的开口与端点。 */
 virtual void Destroyed() override;
 /** 编辑道路路径；推荐平滑曲线与演员单位缩放。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海港道路|组件", meta=(DisplayName="道路中心线"))
 TObjectPtr<USplineComponent> RoadSpline;
 /** 路面、左路缘石、右路缘石、端部路缘石分别使用零至三号材质槽。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海港道路|组件", meta=(DisplayName="道路网格"))
 TObjectPtr<UProceduralMeshComponent> RoadMesh;

 /** 保留原网格碰撞，使用可保存的 Nanite 几何承担显示与投影。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="生成优化", meta=(DisplayName="优化渲染网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> OptimizedMesh;
#if WITH_EDITOR
 /** 连续数值调整保留预览组件注册，避免每次编辑提前拆卸渲染状态。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 将位置手柄投影到中心线，随后通过构造流程更新道路。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 移动结束后更新邻接关系。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后重新求解连接。 */
 virtual void PostEditUndo() override;
#endif
 /** 是否生成左侧唯一支路，默认关闭。 */
 UPROPERTY()
 bool bLeftJunction = false;
 /** 左路口沿中心线的距离；左右位置相同形成十字路口。 */
 UPROPERTY()
 float LeftJunctionDistance = 1000.0f;
 /** 拖动以调整左路口位置，自动吸附道路。 */
 UPROPERTY()
 FVector LeftJunctionHandle = FVector(1000,-300,0);
 /** 从主路边缘向外延伸的长度；支路宽度继承主路。 */
 UPROPERTY()
 float LeftJunctionLength = 1000.0f;
 /** 左路口连接圆弧半径；零为直角，自动限制到可用空间。 */
 UPROPERTY()
 float LeftJunctionRadius = 150.0f;
 /** 是否生成右侧唯一支路，默认关闭。 */
 UPROPERTY()
 bool bRightJunction = false;
 /** 右路口沿中心线的独立距离。 */
 UPROPERTY()
 float RightJunctionDistance = 1000.0f;
 /** 拖动以调整右路口位置，自动吸附道路。 */
 UPROPERTY()
 FVector RightJunctionHandle = FVector(1000,300,0);
 /** 从主路边缘向外延伸的长度；支路宽度继承主路。 */
 UPROPERTY()
 float RightJunctionLength = 1000.0f;
 /** 右路口连接圆弧半径；零为直角，自动限制到可用空间。 */
 UPROPERTY()
 float RightJunctionRadius = 150.0f;
 /** 不含路缘石的通行宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|尺寸", meta=(DisplayName="路面宽度", ClampMin="50", Units="cm"))
 float RoadWidth = 600.0f;
 /** 路面向下挤出的厚度，同时用于碰撞。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|尺寸", meta=(DisplayName="路基厚度", ClampMin="1", Units="cm"))
 float RoadDepth = 20.0f;
 /** 中心线最大采样间距，曲线另按方向变化细分。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|质量", meta=(DisplayName="最大采样间距", ClampMin="10", Units="cm"))
 float SampleSpacing = 75.0f;
 /** 路缘石横向宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|路缘石", meta=(DisplayName="路缘石宽度", ClampMin="5", Units="cm"))
 float CurbWidth = 24.0f;
 /** 路缘石顶面高出路面的高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|路缘石", meta=(DisplayName="路缘石高度", ClampMin="1", Units="cm"))
 float CurbHeight = 15.0f;
 /** 路缘石顶部两侧的倒角尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|路缘石", meta=(DisplayName="顶部倒角", ClampMin="0", Units="cm"))
 float CurbBevel = 3.0f;
 /** 路面纹理单次覆盖的长度，UV按实际距离计算。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|材质", meta=(DisplayName="路面贴图覆盖尺寸", ClampMin="1", Units="cm"))
 float RoadTextureSize = 400.0f;
 /** 路缘石纹理单次覆盖的长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|材质", meta=(DisplayName="路缘石贴图覆盖尺寸", ClampMin="1", Units="cm"))
 float CurbTextureSize = 100.0f;
 /** 是否生成中心线左侧路缘石。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|路缘石", meta=(DisplayName="生成左路缘石"))
 bool bLeftCurb = true;
 /** 是否生成中心线右侧路缘石。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|路缘石", meta=(DisplayName="生成右路缘石"))
 bool bRightCurb = true;
 /** 起点未连接其他道路时封口；连接后自动让出通道，闭环样条忽略此项。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|端部路缘石", meta=(DisplayName="生成起点路缘石"))
 bool bStartCurb = false;
 /** 起点封口向外延伸的圆角半径；零为直角，最大限制为半路宽。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|端部路缘石", meta=(DisplayName="起点圆角半径", ClampMin="0", Units="cm", EditCondition="bStartCurb"))
 float StartCurbRadius = 100.0f;
 /** 终点未连接其他道路时封口；连接后自动让出通道，闭环样条忽略此项。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|端部路缘石", meta=(DisplayName="生成终点路缘石"))
 bool bEndCurb = false;
 /** 终点封口向外延伸的圆角半径；零为直角，最大限制为半路宽。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|端部路缘石", meta=(DisplayName="终点圆角半径", ClampMin="0", Units="cm", EditCondition="bEndCurb"))
 float EndCurbRadius = 100.0f;
 /** 是否创建简单凸碰撞供角色行走和物理查询使用。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 是否投射动态阴影。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|渲染", meta=(DisplayName="投射阴影"))
 bool bCastShadow = true;
 /** 路面材质，直接使用网格中已经计算物理尺寸的UV。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|材质", meta=(DisplayName="路面材质"))
 TObjectPtr<UMaterialInterface> RoadMaterial;
 /** 路缘石材质，直接使用网格中已经计算物理尺寸的UV。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|材质", meta=(DisplayName="路缘石材质"))
 TObjectPtr<UMaterialInterface> CurbMaterial;
 /** 不同实例采用不同偏移，降低纹理重复感。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海港道路|材质", meta=(DisplayName="纹理偏移"))
 FVector2D TextureOffset = FVector2D::ZeroVector;
 /** 无法生成时显示原因；过急转弯会拒绝生成以避免内侧折叠。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海港道路|诊断", meta=(DisplayName="生成诊断"))
 FString GenerationDiagnostic;
 /** 保存到关卡的碰撞算法版本；零表示需要迁移的旧数据。 */
 UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category="海港道路|诊断", meta=(DisplayName="碰撞数据版本"))
 int32 CollisionDataVersion = 0;
protected:
 /** 本地几何未改变时同步渲染输入，派生海堤同时确认堤身缓存。 */
 virtual void AcceptUnchangedRoadSource();
 /** 道路重建的所有出口均通知附属结构，包括邻路触发的连接变化；空路面必须清空附属网格。 */
 virtual void BuildRoadFoundation(const TArray<FLxRoadSampleFrame>& Frames, int32 MainVertexCount) {}
private:
 /** 本地形态与求解后的连接数据；整体移动且无连接时保持不变。 */
 TArray<uint8> CaptureRoadGeometryInputs() const;
 /** 已应用的形态快照，保存及复制后继续复用程序化源和碰撞。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="道路形态快照"))
 TArray<uint8> RoadGeometryInputs;
 /** 取消尚未提交的连续编辑预览。 */
 void CancelDeferredRoadUpdate();
 /** 连续属性编辑合并为一次预览，保存和运行前由构造入口立即提交。 */
 void ScheduleDeferredRoadUpdate();
 /** 最后一次连续编辑的时间。 */
 double LastRoadEditTime = 0;
 /** 编辑预览的延迟更新委托。 */
 FTSTicker::FDelegateHandle DeferredRoadUpdateHandle;
 /** 防止纠正组件变换时递归触发自身。 */
 bool bRestoringRoadComponents=false;
 /** 将程序化网格和渲染缓存保持在各自父组件原点。 */
 void RestoreRoadComponentTransforms();
 friend struct FLxRoadConnectionSolver;
 /** 求解后的起终点，保存后避免重载时误判全部道路连接发生变化。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="端点连接缓存", Category="生成优化"))
 FLxRoadEndpoint ConnectedEndpoints[2];
 /** 求解后的多个侧面开口与独立端点连接片。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="路口连接缓存", Category="生成优化"))
 TArray<FLxRoadAutoJunction> ConnectedJunctions;
 /** 使用当前连接解生成网格，不再次扫描邻居。 */
 void BuildRoadGeometry();
 /** 将已编辑的连接参数同步至对方记录。 */
 void SynchronizeConnectionSettings(int32 Index);
};
