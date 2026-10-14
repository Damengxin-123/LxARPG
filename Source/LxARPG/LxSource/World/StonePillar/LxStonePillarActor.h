#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxStonePillarActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** 柱体截面，方柱的半径表示半边长。 */
UENUM(BlueprintType, meta=(DisplayName="石柱形状"))
enum class ELxPillarShape : uint8
{
 Round UMETA(DisplayName="圆柱"),
 Square UMETA(DisplayName="方柱")
};

/** 三块独立整石组成的自动柱子：可选底墩、无拼缝柱身、可选顶托。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动石柱"))
class LXARPG_API ALxStonePillarActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一静态网格组件，简单碰撞与它共用一个物理体。 */
 ALxStonePillarActor();
 /** 构造或参数变更时生成，整体移动保持缓存。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 关卡加载或组件重新注册时恢复临时网格。 */
 virtual void PostRegisterAllComponents() override;
 /** 游戏加载时恢复几何与简单碰撞。 */
 virtual void BeginPlay() override;
 /** 销毁时取消编辑器延迟更新。 */
 virtual void BeginDestroy() override;
#if WITH_EDITOR
 /** 拖动高度手柄时保留旧预览。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 合并滑块与手柄的连续变化。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 松手时提交高度，整体移动不重建几何。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后恢复对应柱形。 */
 virtual void PostEditUndo() override;
#endif
 /** 蓝图改完参数后立即重建。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石柱|生成", meta=(DisplayName="重新生成石柱"))
 void RebuildPillar();
 /** 保存当前柱子为可复用静态网格，保留三块整石和分段简单碰撞。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石柱|资产", meta=(DisplayName="保存石柱静态网格"))
 void SaveStaticMeshAsset();
 /** 三块石头作为独立闭合几何合并在一个渲染组件内。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|组件", meta=(DisplayName="整柱静态网格"))
 TObjectPtr<UStaticMeshComponent> PillarMesh;
 /** 圆柱或有真实倒角的方柱。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|尺寸", meta=(DisplayName="柱子形状"))
 ELxPillarShape Shape = ELxPillarShape::Round;
 /** 在关卡拖动顶部菱形的 Z 轴；数值为包含底墩和顶托的总高度，XY 自动归零。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|尺寸", meta=(DisplayName="高度手柄", MakeEditWidget=true, Units="cm"))
 FVector HeightHandle = FVector(0,0,360);
 /** 圆柱半径；方柱为中心到平面的距离，即截面半边长。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|尺寸", meta=(DisplayName="柱身半径（方柱半边长）", ClampMin="5", ClampMax="200", Units="cm"))
 float Radius = 32;
 /** 是否生成下方独立整石底墩。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="启用底墩"))
 bool bBase = true;
 /** 是否生成上方独立整石顶托。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="启用顶托"))
 bool bCapital = true;
 /** 底墩目标高度；总高较小时按比例压缩端部以保留完整柱身。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="底墩高度", ClampMin="10", ClampMax="160", Units="cm", EditCondition="bBase"))
 float BaseHeight = 36;
 /** 顶部承托石的目标高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="顶托高度", ClampMin="10", ClampMax="160", Units="cm", EditCondition="bCapital"))
 float CapitalHeight = 42;
 /** 底墩最宽处相对柱身的尺寸倍率。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="底墩加宽倍率", ClampMin="1.1", ClampMax="2.2", EditCondition="bBase"))
 float BaseScale = 1.5f;
 /** 顶部承托面的尺寸倍率。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="顶托加宽倍率", ClampMin="1.1", ClampMax="2.2", EditCondition="bCapital"))
 float CapitalScale = 1.6f;
 /** 仅位于三块石头之间的细小接缝，柱身中间没有砌筑横缝。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|端部", meta=(DisplayName="整石接缝", ClampMin="0.1", ClampMax="2", Units="cm"))
 float JointWidth = 0.35f;
 /** 顶底边和方柱竖向边的细微倒角。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|外观", meta=(DisplayName="石边倒角", ClampMin="0.1", ClampMax="4", Units="cm"))
 float Bevel = 0.8f;
 /** 圆柱轴向凹槽深度，设为零得到平滑圆柱；凹槽与整根石材连续。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|外观", meta=(DisplayName="圆柱纵槽深度", ClampMin="0", ClampMax="2", Units="cm"))
 float FluteDepth = 0.45f;
 /** 圆柱截面的离散段数，越多越圆滑。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|外观", meta=(DisplayName="圆周精度", ClampMin="24", ClampMax="128"))
 int32 RoundSegments = 96;
 /** 每块整石的稳定颜色与石纹相位。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|外观", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 619;
 /** 灰白整根柱身材质，支持三维石纹与连续浅刻纹。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|材质", meta=(DisplayName="柱身石材"))
 TObjectPtr<UMaterialInterface> ShaftMaterial;
 /** 底墩与顶托使用略偏冷的石材及简洁装饰带。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|材质", meta=(DisplayName="端部石材"))
 TObjectPtr<UMaterialInterface> EndMaterial;
 /** 使用每块整石一个简单形状，避免宽底墩使整根柱身碰撞过粗。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 完整资产包路径；保存遇到同名资产自动编号。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石柱|资产", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/石柱/静态网格/石柱");
 /** 最近成功保存的独立静态网格资产。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|资产", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 当前石块数量，开启两端时为三块，柱身始终只有一块。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|统计", meta=(DisplayName="独立石块数量"))
 int32 GeneratedStoneCount = 0;
 /** 有效石柱始终只有一个渲染网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|统计", meta=(DisplayName="渲染网格数量"))
 int32 GeneratedMeshCount = 0;
 /** 当前网格的三角形总数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 排除两端与接缝之后的实际整根石条高度。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|统计", meta=(DisplayName="实际柱身高度", Units="cm"))
 float GeneratedShaftHeight = 0;
 /** 本会话实际几何构建次数，普通移动不应增加。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="石柱|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 无效参数或资产保存结果的中文提示。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石柱|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
 /** 编辑器生成 Nanite 数据；关闭后使用完整几何的普通静态网格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite虚拟化几何"))
 bool bEnableNanite = true;
private:
 /** 网格随关卡保存并参与实例复制，避免加载和进入游戏时重新构建。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已生成几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 只在形状参数变化时生成。 */
 void UpdatePillarIfNeeded();
 /** 返回全部影响几何与石纹坐标的精确参数。 */
 TArray<double> GetGeometrySettings() const;
 /** 不重建几何地刷新材质和碰撞开关。 */
 void RefreshMaterialsAndCollision();
 /** 恢复被编辑器操作注销的网格组件。 */
 void RestorePillarRegistration();
 /** 取消未提交的编辑器延迟更新。 */
 void CancelDeferredUpdate();
 /** 最近一次成功生成的参数序列。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 TArray<double> CachedGeometrySettings;
 /** 几何缓存是否有效。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 bool bGeometryCached = false;
 /** 最近一次交互事件的时间。 */
 double LastEditTime = 0;
 /** 合并高度拖动与连续滑块操作的全局计时器。 */
 FTSTicker::FDelegateHandle DeferredUpdateHandle;
};
