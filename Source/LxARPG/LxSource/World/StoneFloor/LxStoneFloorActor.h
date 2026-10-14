#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxStoneFloorActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UBoxComponent;
class UMaterialInterface;

/** 由大块正方形岩石板铺成的方形地板：单网格、单材质和单盒碰撞。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动石质地板"))
class LXARPG_API ALxStoneFloorActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一渲染网格和唯一碰撞盒。 */
 ALxStoneFloorActor();
 /** 参数修改后更新地板，整体移动复用缓存。 */
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
 /** 结束拖动后提交尺寸，整体移动不重新地板。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后恢复对应尺寸、网格和碰撞。 */
 virtual void PostEditUndo() override;
#endif
 /** 蓝图批量设置参数后立即刷新整块地板。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石质地板|生成", meta=(DisplayName="重新生成石地板"))
 void RebuildFloor();
 /** 保存带一个简化碰撞盒的独立静态网格，同名自动编号。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="石质地板|资产", meta=(DisplayName="保存石地板静态网格"))
 void SaveStaticMeshAsset();
 /** 三维岩石纹理和接缝共用唯一静态网格、唯一材质槽。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|组件", meta=(DisplayName="地板静态网格"))
 TObjectPtr<UStaticMeshComponent> PanelMesh;
 /** 覆盖全板的唯一碰撞盒，细缝不妨碍角色平稳行走。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|组件", meta=(DisplayName="地板碰撞盒"))
 TObjectPtr<UBoxComponent> PanelCollision;
 /** 拖动本地 X 轴调整正方形边长，Y 和 Z 自动归零；地板顶面位于本地 Z=0。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|尺寸", meta=(DisplayName="边长手柄", MakeEditWidget=true, Units="cm"))
 FVector SizeHandle = FVector(800,0,0);
 /** 从顶面向下的整体厚度，不改变铺装高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|尺寸", meta=(DisplayName="地板厚度", ClampMin="5", ClampMax="150", Units="cm"))
 float Thickness = 24;
 /** 单块正方形石板的目标边长，按整片边长均分，避免边缘窄条。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|铺装", meta=(DisplayName="石板目标边长", ClampMin="50", ClampMax="600", Units="cm"))
 float TileSize = 200;
 /** 相邻石板间的细缝宽度，缝内有低于顶面的同材质深色衬底。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|铺装", meta=(DisplayName="石板接缝宽度", ClampMin="0.1", ClampMax="5", Units="cm"))
 float JointWidth = 1.2f;
 /** 接缝衬底相对板面的下沉量，不会形成穿透地板的空隙。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|铺装", meta=(DisplayName="接缝下沉深度", ClampMin="0.2", ClampMax="5", Units="cm"))
 float JointDepth = 1.5f;
 /** 石板边缘的真实几何倒角，保持清楚而不过分锋利的轮廓。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|外观", meta=(DisplayName="石板倒角", ClampMin="0.1", ClampMax="3", Units="cm"))
 float Bevel = 0.65f;
 /** 每块整石全表面一致的柔和色差，不使用破败污渍。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|外观", meta=(DisplayName="石板色差", ClampMin="0", ClampMax="0.25"))
 float ColorVariation = 0.10f;
 /** 石板纹理相位与色差的固定种子，移动不会改变外观。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|外观", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 829;
 /** 使用 UV0 和 UV1 携带的连续三维坐标表现浅灰粗岩，避免倒角处纹理接缝。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|材质", meta=(DisplayName="粗岩石材质"))
 TObjectPtr<UMaterialInterface> StoneMaterial;
 /** 开启后整个地板只有一个查询和物理碰撞盒。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 独立静态网格的完整资产包路径，同名资产自动编号。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="石质地板|资产", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/石地板/静态网格/石地板");
 /** 最近成功保存的静态网格资产。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|资产", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 实际可见石板数量，不包含接缝衬底。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|统计", meta=(DisplayName="石板数量"))
 int32 GeneratedTileCount = 0;
 /** 均分后每块正方形石板的实际边长，不包含接缝。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|统计", meta=(DisplayName="实际石板边长", Units="cm"))
 float GeneratedTileSize = 0;
 /** 有效地板始终只生成一个网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|统计", meta=(DisplayName="渲染网格数量"))
 int32 GeneratedMeshCount = 0;
 /** 整个合并网格的三角形数量。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 当前实例实际重建次数，整体移动不增加。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="石质地板|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 无效参数、生成预算或保存结果的中文提示。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="石质地板|生成", meta=(DisplayName="生成提示"))
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
 /** 更新唯一材质槽和唯一碰撞盒，不重新生成石板。 */
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
