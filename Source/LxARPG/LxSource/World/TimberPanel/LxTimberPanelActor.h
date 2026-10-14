#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxTimberPanelActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;

/** 水平矩形木地板与天花板：连续木纹、深色接头横梁、单网格及单盒碰撞。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动木质拼板"))
class LXARPG_API ALxTimberPanelActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一渲染网格和唯一碰撞盒。 */
 ALxTimberPanelActor();
 /** 参数修改后更新拼板，整体移动复用缓存。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 加载关卡和重新注册时优先复用已保存的静态网格。 */
 virtual void PostRegisterAllComponents() override;
 /** 游戏开始时保证碰撞和几何完整。 */
 virtual void BeginPlay() override;
 /** 对象销毁时移除延迟更新。 */
 virtual void BeginDestroy() override;
#if WITH_EDITOR
 /** 连续调节尺寸时保留原有预览注册。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 合并尺寸手柄与滑块事件，空闲后提交一次。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 结束拖动后提交尺寸，整体移动不重新拼板。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后恢复对应尺寸、网格和碰撞。 */
 virtual void PostEditUndo() override;
#endif
 /** 蓝图批量设置参数后立即刷新整块拼板。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="木质拼板|生成", meta=(DisplayName="重新生成木板"))
 void RebuildPanel();
 /** 保存带一个简化碰撞盒的独立静态网格，同名自动编号。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="木质拼板|资产", meta=(DisplayName="保存木板静态网格"))
 void SaveStaticMeshAsset();
 /** 木板与深色构件共用一个静态网格、两个材质槽。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|组件", meta=(DisplayName="整板静态网格"))
 TObjectPtr<UStaticMeshComponent> PanelMesh;
 /** 覆盖整块板外包围范围的唯一盒形碰撞，不逐块木板生成碰撞。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|组件", meta=(DisplayName="整板碰撞盒"))
 TObjectPtr<UBoxComponent> PanelCollision;
 /** XY 控制水平矩形长宽；原点位于顶面一角，Z 自动归零。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|尺寸", meta=(DisplayName="长宽手柄", MakeEditWidget=true, Units="cm"))
 FVector SizeHandle = FVector(600,400,0);
 /** 从顶面向下的实心板厚度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|尺寸", meta=(DisplayName="板体厚度", ClampMin="3", ClampMax="80", Units="cm"))
 float Thickness = 12;
 /** 开启后横梁朝下突出，作为可从下方观察的天花板。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|构造", meta=(DisplayName="天花板模式"))
 bool bCeiling = false;
 /** 天花板横梁向板底外侧突出多少；地板模式的横梁保持平齐。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|构造", meta=(DisplayName="横梁下凸高度", ClampMin="0", ClampMax="60", Units="cm", EditCondition="bCeiling"))
 float BeamDrop = 10;
 /** 关闭时去除四周深色包边，内部木板端头横梁仍然保留。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|构造", meta=(DisplayName="启用深色包边"))
 bool bBorder = true;
 /** 包边在矩形内占用的宽度，不增加长宽手柄控制的外轮廓。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|构造", meta=(DisplayName="包边宽度", ClampMin="2", ClampMax="40", Units="cm", EditCondition="bBorder"))
 float BorderWidth = 12;
 /** 旋转九十度后沿本地 Y 方向铺板；木纹与横梁同步换向。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|拼接", meta=(DisplayName="木板沿Y方向"))
 bool bAlongY = false;
 /** 一段木板的目标长度，端头必定由自动横梁覆盖；实际长度均分以消除碎片。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|拼接", meta=(DisplayName="木板目标长度", ClampMin="50", ClampMax="600", Units="cm"))
 float PlankLength = 180;
 /** 相邻木板的目标宽度，自动均分整个矩形。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|拼接", meta=(DisplayName="木板目标宽度", ClampMin="8", ClampMax="80", Units="cm"))
 float PlankWidth = 24;
 /** 自动生成的横向接头梁宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|拼接", meta=(DisplayName="接头横梁宽度", ClampMin="2", ClampMax="30", Units="cm"))
 float BeamWidth = 8;
 /** 顺木纹长边之间的窄缝，缝内有深色衬底，不会穿透。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|拼接", meta=(DisplayName="木板细缝宽度", ClampMin="0.05", ClampMax="1", Units="cm"))
 float JointWidth = 0.18f;
 /** 轻微真实倒角，保持干净的新木构件轮廓。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|外观", meta=(DisplayName="木边倒角", ClampMin="0.02", ClampMax="0.8", Units="cm"))
 float Bevel = 0.16f;
 /** 每根构件全表面共享的温和明暗变化。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|外观", meta=(DisplayName="木板色差", ClampMin="0", ClampMax="0.2"))
 float ColorVariation = 0.06f;
 /** 固定种子保证移动、加载和重复生成时纹理与颜色不闪变。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|外观", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 417;
 /** 浅暖木纹材质，UV0 与 UV1 携带各木构件的连续局部三维坐标。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|材质", meta=(DisplayName="木板材质"))
 TObjectPtr<UMaterialInterface> PlankMaterial;
 /** 深棕色包边、接头横梁和细缝衬底使用同一材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|材质", meta=(DisplayName="横梁包边材质"))
 TObjectPtr<UMaterialInterface> BeamMaterial;
 /** 启用时整板始终只有一个参与查询与物理的盒形碰撞。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 独立资产保存的完整包路径，不包含文件扩展名。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木质拼板|资产", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/木板/静态网格/木板");
 /** 最近成功保存的可复用资产。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|资产", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 当前有效木板块数，不代表网格对象数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|统计", meta=(DisplayName="木板数量"))
 int32 GeneratedPlankCount = 0;
 /** 横梁和包边的构件数，不包括隐藏衬底。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|统计", meta=(DisplayName="梁边构件数量"))
 int32 GeneratedBeamCount = 0;
 /** 有效拼板固定为一个渲染网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|统计", meta=(DisplayName="渲染网格数量"))
 int32 GeneratedMeshCount = 0;
 /** 实际上传的三角形数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 只统计本会话实际重新生成几何的次数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="木质拼板|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 生成失败、预算超限或资产保存结果。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木质拼板|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
 /** 编辑器生成 Nanite 数据；关闭后使用完整几何的普通静态网格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite虚拟化几何"))
 bool bEnableNanite = true;
private:
 /** 网格随关卡保存并参与实例复制，避免加载和进入游戏时重新构建。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已生成几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 只在几何与外观参数变化时重建。 */
 void UpdatePanelIfNeeded();
 /** 精确缓存形状参数，不把世界变换或材质算入。 */
 TArray<double> GetGeometrySettings() const;
 /** 更新两个材质槽和唯一碰撞盒，不重新生成木板。 */
 void RefreshMaterialsAndCollision();
 /** 恢复外部编辑器操作注销的组件。 */
 void RestorePanelRegistration();
 /** 移除尚未执行的编辑器合并更新。 */
 void CancelDeferredUpdate();
 /** 上次成功构建时的全部形状参数。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 TArray<double> CachedGeometrySettings;
 /** 单一盒体的本地范围。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 FBox CachedBounds = FBox(ForceInit);
 /** 是否存在可复用的几何缓存。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 bool bGeometryCached = false;
 /** 最后一次交互事件时间。 */
 double LastEditTime = 0;
 /** 合并交互更新的全局时钟委托。 */
 FTSTicker::FDelegateHandle DeferredUpdateHandle;
};
