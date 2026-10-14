#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxBrickWallActor.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UBoxComponent;
class UStaticMeshComponent;
class UStaticMesh;

/** 墙端的独立连接方式；左右以墙体本地正 X 方向为观察方向。 */
UENUM(BlueprintType, meta=(DisplayName="砖墙连接方式"))
enum class ELxWallConnection : uint8
{
 None UMETA(DisplayName="无连接"),
 Straight UMETA(DisplayName="直线连接"),
 Left UMETA(DisplayName="直角朝左（负Y）"),
 Right UMETA(DisplayName="直角朝右（正Y）")
};

/** 以完整倒角石砖生成垂直墙体，支持拖动长度、独立端柱及直角延伸。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动砖墙"))
class LXARPG_API ALxBrickWallActor : public AActor
{
 GENERATED_BODY()
public:
 /** 初始化固定数量的网格组件。 */
 ALxBrickWallActor();
 /** 编辑尺寸或拖动长度手柄后重建。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 关卡加载或组件重新注册后恢复临时静态网格，避免依赖构造脚本才显示。 */
 virtual void PostRegisterAllComponents() override;
 /** 游戏加载时恢复碰撞和网格。 */
 virtual void BeginPlay() override;
 /** 销毁对象时取消编辑器延迟更新。 */
 virtual void BeginDestroy() override;
#if WITH_EDITOR
 /** 几何参数与长度控件保留已注册的预览组件，避免延迟重建期间墙体消失。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 移动整个演员时复用网格，松手后处理尚未提交的尺寸。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 连续调参和长度手柄操作仅合并请求，不逐帧重建。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
 /** 撤销或重做后恢复与属性一致的几何和碰撞。 */
 virtual void PostEditUndo() override;
#endif
 /** 按当前参数重新生成，蓝图批量改参数后调用一次即可。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="砖墙|生成", meta=(DisplayName="重新生成砖墙"))
 void RebuildWall();
 /** 把当前整面墙保存成独立静态网格资产；同名时自动编号，不覆盖已有资产。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="砖墙|静态网格", meta=(DisplayName="保存整墙静态网格"))
 void SaveStaticMeshAsset();
 /** 默认把砖面、转角柱、侧墙和砂浆直接合并为一个静态网格体。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|静态网格", meta=(DisplayName="生成整墙静态网格"))
 bool bGenerateStaticMesh = true;
 /** 只有一个渲染组件，最多三个材质分区，不按砖块创建网格对象。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|组件", meta=(DisplayName="整墙静态网格"))
 TObjectPtr<UStaticMeshComponent> StaticWallMesh;
 /** 保存目标的完整资产包路径；保存按钮自动为重名资产追加编号。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|静态网格", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/砖墙/静态网格/砖墙");
 /** 最近保存的独立资产；保存后可直接拖入关卡或用于实例化，不再执行砌筑。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|静态网格", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 当前实际使用的渲染网格数量；有效墙体始终为一，失败时为零。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|性能", meta=(DisplayName="渲染网格数量"))
 int32 GeneratedMeshCount = 0;
 /** 返回指定端部的连接出口世界变换，正 X 指向下一段墙的延伸方向。 */
 UFUNCTION(BlueprintPure, Category="砖墙|连接", meta=(DisplayName="获取连接出口"))
 FTransform GetConnectionTransform(bool bEnd) const;
 /** 兼容旧墙的程序化组件与根节点；静态模式下清空几何，仅保留变换。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|组件", meta=(DisplayName="砖墙网格"))
 TObjectPtr<UProceduralMeshComponent> WallMesh;
 /** 最多五段盒形碰撞，与渲染网格分离，更新砖面不再触发凸体烹饪。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|碰撞", meta=(DisplayName="分段碰撞盒"))
 TArray<TObjectPtr<UBoxComponent>> CollisionBoxes;
 /** 裁去被内层砂浆完全覆盖的面，保留砖缝口和所有外侧可见面。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|性能", meta=(DisplayName="剔除内部隐藏面"))
 bool bCullInternalFaces = true;
 /** 只使用 X 分量；在关卡中拖动菱形手柄调整长度，Y、Z 自动归零。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|尺寸", meta=(DisplayName="长度手柄", MakeEditWidget=true, Units="cm"))
 FVector LengthHandle = FVector(600,0,0);
 /** 墙体沿本地 Y 的宽度，即实体厚度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|尺寸", meta=(DisplayName="墙面宽度（厚度）", ClampMin="10", ClampMax="500", Units="cm"))
 float WallWidth = 40;
 /** 墙体从底面到顶面的高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|尺寸", meta=(DisplayName="墙面高度", ClampMin="20", ClampMax="3000", Units="cm"))
 float WallHeight = 240;
 /** 起点的连接方式。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|连接", meta=(DisplayName="起点连接"))
 ELxWallConnection StartConnection = ELxWallConnection::Straight;
 /** 终点的连接方式。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|连接", meta=(DisplayName="终点连接"))
 ELxWallConnection EndConnection = ELxWallConnection::Straight;
 /** 连接柱在墙厚两侧各增加的尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|连接", meta=(DisplayName="连接处单侧加宽", ClampMin="2", ClampMax="100", Units="cm"))
 float ConnectionOverhang = 10;
 /** 起点为直角时生成的侧墙长度；零表示只保留转角柱和出口。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|连接", meta=(DisplayName="起点转角延伸长度", ClampMin="0", ClampMax="10000", Units="cm"))
 float StartReturnLength = 180;
 /** 终点为直角时生成的侧墙长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|连接", meta=(DisplayName="终点转角延伸长度", ClampMin="0", ClampMax="10000", Units="cm"))
 float EndReturnLength = 180;
 /** 正常石砖长度，奇偶层错开半砖。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|砌筑", meta=(DisplayName="砖块长度", ClampMin="15", ClampMax="200", Units="cm"))
 float BrickLength = 60;
 /** 目标砖高，自动均分整面墙的高度，避免顶部薄片。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|砌筑", meta=(DisplayName="砖块高度", ClampMin="8", ClampMax="100", Units="cm"))
 float BrickHeight = 24;
 /** 实体砖之间的砂浆缝宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|砌筑", meta=(DisplayName="砖缝宽度", ClampMin="0.2", ClampMax="5", Units="cm"))
 float JointWidth = 1.2f;
 /** 砂浆相对砖块外表面下沉的深度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|砌筑", meta=(DisplayName="砖缝凹陷深度", ClampMin="0.2", ClampMax="5", Units="cm"))
 float JointDepth = 1.5f;
 /** 倒角使同一块砖的正面、侧面和顶面自然衔接。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|砌筑", meta=(DisplayName="砖边倒角", ClampMin="0.1", ClampMax="3", Units="cm"))
 float Bevel = 0.7f;
 /** 每块砖宽度及表面中心的细微变化，不移动砖缝。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|外观", meta=(DisplayName="表面起伏", ClampMin="0", ClampMax="1.5", Units="cm"))
 float SurfaceRelief = 0.35f;
 /** 同一块砖的各面共享色差，确保拐角不会变成两块砖。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|外观", meta=(DisplayName="砖块色差", ClampMin="0", ClampMax="0.3"))
 float ColorVariation = 0.08f;
 /** 底部轻微积尘强度，保留明亮灰白石材风格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|外观", meta=(DisplayName="底部积尘", ClampMin="0", ClampMax="0.5"))
 float Weathering = 0.15f;
 /** 相同种子与尺寸生成相同表面。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|外观", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 1729;
 /** 支持顶点颜色与三维连续纹理的石砖材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|材质", meta=(DisplayName="墙面石材"))
 TObjectPtr<UMaterialInterface> WallMaterial;
 /** 独立的连接石材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|材质", meta=(DisplayName="连接处石材"))
 TObjectPtr<UMaterialInterface> ConnectionMaterial;
 /** 内凹接缝材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|材质", meta=(DisplayName="砖缝砂浆"))
 TObjectPtr<UMaterialInterface> MortarMaterial;
 /** 使用每段一个凸体的简化碰撞，避免逐砖碰撞的性能开销。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="砖墙|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 生成失败或超出预算时的中文诊断。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
 /** 实际生成的石砖数量，含连接石。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|生成", meta=(DisplayName="石砖数量"))
 int32 GeneratedBrickCount = 0;
 /** 本次几何实际上传的三角形数量，便于比较优化结果。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="砖墙|性能", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 当前会话真正执行的几何重建次数；普通移动不应增加。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="砖墙|性能", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;

/** 编辑器生成 Nanite 数据；关闭后使用保留完整几何的普通静态网格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite虚拟化几何"))
 bool bEnableNanite = true;
private:
 /** 网格随关卡保存并参与实例复制，避免加载和进入游戏时重新构建。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已生成几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 只在形状或外观参数发生变化时生成网格，材质和碰撞单独刷新。 */
 void UpdateWallIfNeeded();
 /** 读取影响几何的精确参数序列，排除世界变换、材质引用与碰撞开关。 */
 TArray<double> GetGeometrySettings() const;
 /** 只变更需要更新的材质和碰撞盒。 */
 void RefreshMaterialsAndCollision();
 /** 恢复被编辑器注销的组件；正常已注册状态不产生重复注册开销。 */
 void RestoreWallRegistration();
 /** 取消未执行的编辑器延迟请求。 */
 void CancelDeferredUpdate();
 /** 上次成功生成时的精确参数，避免散列碰撞或变换变化导致重建。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 TArray<double> CachedGeometrySettings;
 /** 上次生成的墙段与柱体包围盒；只用于盒形碰撞。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 TArray<FBox> CachedCollisionBounds;
 /** 是否已有与缓存对应的网格，复制、加载后首次生成重新建立缓存。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 bool bGeometryCached = false;
 /** 长度手柄最后一次变化的时间，连续操作合并到空闲后执行。 */
 double LastEditTime = 0;
 /** 编辑器全局时钟委托，视口不实时刷新时仍能提交最终尺寸。 */
 FTSTicker::FDelegateHandle DeferredUpdateHandle;
};
