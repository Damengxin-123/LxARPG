#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxStoneRailingActor.generated.h"

class USplineComponent;
class UProceduralMeshComponent;
class ULxGeneratedMeshRenderComponent;
class UMaterialInterface;

/** 同时控制外展柱头轮廓和双面浅浮雕的海滨石雕预设。 */
UENUM(BlueprintType, meta=(DisplayName="石护栏风格"))
enum class ELxStoneRailingStyle : uint8
{
 Harbour UMETA(DisplayName="海港菱纹"),
 Floral UMETA(DisplayName="庭园花叶"),
 Wave UMETA(DisplayName="海岸波纹")
};

/** 根据可编辑样条生成连续倒角顶梁及收腰、外展柱头的 T 形石护栏。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动石护栏", Category="海港城市"))
class LXARPG_API ALxStoneRailingActor : public AActor
{
 GENERATED_BODY()
public:
 /** 初始化控制线、碰撞网格与持久化渲染缓存。 */
 ALxStoneRailingActor();
 /** 编辑路径或参数后更新；未变化时直接复用几何。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 加载旧关卡时升级拐角几何，避免继续显示修复前的持久化缓存。 */
 virtual void PostRegisterAllComponents() override;
 /** 蓝图批量修改属性或控制点后提交全部几何。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石护栏|生成", meta=(DisplayName="重新生成护栏"))
 void RebuildRailing();
 /** 可在视口拖动、Alt 拖动加点、右键调整点类型的样条。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|路径", meta=(DisplayName="护栏控制线"))
 TObjectPtr<USplineComponent> RailingSpline;
 /** 统一保存顶梁、石柱、雕花和简单碰撞的几何源。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|组件", meta=(DisplayName="护栏几何"))
 TObjectPtr<UProceduralMeshComponent> RailingMesh;
 /** 编辑时快速预览，保存或运行前自动完成静态渲染缓存。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|组件", meta=(DisplayName="优化渲染网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> OptimizedMesh;
 /** 预设只改变造型及花纹，不改变控制点。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|外观", meta=(DisplayName="预设风格"))
 ELxStoneRailingStyle Style = ELxStoneRailingStyle::Harbour;
 /** 控制线到顶梁顶面的总高度，单位厘米，立柱保持竖直。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|尺寸", meta=(DisplayName="护栏高度", ClampMin="40", ClampMax="300", Units="cm"))
 float Height = 110;
 /** 每十米的目标立柱间隔数量；端点和转角额外保证有柱。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|尺寸", meta=(DisplayName="立柱密度（每十米）", ClampMin="2", ClampMax="30"))
 float Density = 8;
 /** 顶梁宽度，柱脚、柱身与雕花随宽度同步缩放。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|尺寸", meta=(DisplayName="护栏宽度", ClampMin="20", ClampMax="100", Units="cm"))
 float Width = 40;
 /** 首尾无缝连接，闭合点不重复生成立柱。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|路径", meta=(DisplayName="闭合护栏"))
 bool bClosedLoop = false;
 /** 开启真实浅浮雕；关闭后仍保留石材倒角与装饰线脚。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|外观", meta=(DisplayName="显示雕花"))
 bool bOrnaments = true;
 /** 明亮灰白主体石材，可在蓝图预设中指定。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|材质", meta=(DisplayName="主体石材"))
 TObjectPtr<UMaterialInterface> StoneMaterial;
 /** 顶梁与浅浮雕使用略偏冷的细磨石材。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|材质", meta=(DisplayName="雕花石材"))
 TObjectPtr<UMaterialInterface> OrnamentMaterial;
 /** 用分段凸包阻挡角色，保留立柱间的实际空隙。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石护栏|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 包含端点及转角的实际立柱数量。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|统计", meta=(DisplayName="立柱数量"))
 int32 GeneratedPostCount = 0;
 /** 当前生成几何的三角形总数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 本次会话真正构建几何的次数。 */
 UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="石护栏|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 无效路径或过密参数的中文说明；无效路径会清空旧几何。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石护栏|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
private:
 /** 已生成几何的算法版本；旧实例默认零，重新生成后随关卡保存。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="石护栏|统计", meta=(DisplayName="几何版本"))
 int32 GeneratedGeometryVersion = 0;
};
