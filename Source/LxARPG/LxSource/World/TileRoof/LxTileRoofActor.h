#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxTileRoofActor.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** 屋顶坡面类型，双坡共用一条屋脊和一个合并网格。 */
UENUM(BlueprintType, meta=(DisplayName="屋顶类型"))
enum class ELxRoofType : uint8
{
 /** 沿本地正 Y 方向升高的单坡。 */
 SingleSlope UMETA(DisplayName="单坡屋顶"),
 /** 两个坡面在水平进深中央相交。 */
 Gable UMETA(DisplayName="双坡屋顶")
};

/** 蓝灰叠瓦单坡与双坡屋顶，左右石沿与坡顶压顶可分别关闭，不生成木质顶棚。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动瓦屋顶"))
class LXARPG_API ALxTileRoofActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一渲染和物理组件。 */
 ALxTileRoofActor();
 /** 调参后更新屋顶，整体移动保持几何缓存。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 加载或重新注册后恢复临时网格。 */
 virtual void PostRegisterAllComponents() override;
 /** 游戏中恢复生成几何与斜面碰撞。 */
 virtual void BeginPlay() override;
 /** 销毁时清理延迟更新。 */
 virtual void BeginDestroy() override;
#if WITH_EDITOR
 /** 连续拖动尺寸时保留当前预览。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 合并手柄和滑块的交互更新。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 松手时提交几何，整体移动不重新铺瓦。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销操作后恢复几何。 */
 virtual void PostEditUndo() override;
#endif
 /** 批量设置参数后立即生成。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="瓦屋顶|生成", meta=(DisplayName="重新生成屋顶"))
 void RebuildRoof();
 /** 保存包含简单斜面碰撞的独立静态网格，同名自动编号。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="瓦屋顶|资产", meta=(DisplayName="保存屋顶静态网格"))
 void SaveStaticMeshAsset();
 /** 瓦片、石沿共用的唯一静态网格与物理组件。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|组件", meta=(DisplayName="屋顶静态网格"))
 TObjectPtr<UStaticMeshComponent> RoofMesh;
 /** 单坡或完整双坡；尺寸手柄始终表示整个屋顶的水平宽深。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|尺寸", meta=(DisplayName="屋顶类型"))
 ELxRoofType RoofType = ELxRoofType::SingleSlope;
 /** 随左右石沿开关生成向下封闭侧面的石墙，双坡时组成完整山墙。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|侧墙", meta=(DisplayName="石沿向下封闭侧面"))
 bool bCloseSides = true;
 /** 侧墙外表面相对石沿缩进的距离，两面均缩进以保留压顶轮廓。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|侧墙", meta=(DisplayName="侧墙内凹距离", ClampMin="1", ClampMax="20", Units="cm", EditCondition="bCloseSides"))
 float SideWallInset = 4;
 /** 侧墙底部低于檐口背衬的距离，可向下延伸与建筑外墙相接。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|侧墙", meta=(DisplayName="侧墙向下延伸", ClampMin="0", ClampMax="500", Units="cm", EditCondition="bCloseSides"))
 float SideWallDrop = 0;
 /** 双坡屋脊超出坡面最高点的高度，坡顶石沿开关同时控制屋脊。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="双坡屋脊高度", ClampMin="6", ClampMax="40", Units="cm"))
 float RidgeHeight = 12;
 /** X 调整檐口宽度，Y 调整水平进深；Z 自动归零，单坡沿本地正 Y 升高，双坡在进深中点汇合。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|尺寸", meta=(DisplayName="宽深手柄", MakeEditWidget=true, Units="cm"))
 FVector SizeHandle = FVector(900,600,0);
 /** 屋面相对水平面的角度，两方向宽度保持水平投影尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|尺寸", meta=(DisplayName="屋顶坡度", ClampMin="10", ClampMax="50", Units="deg"))
 float SlopeAngle = 25;
 /** 位于瓦片下方的薄瓦面背衬厚度，不生成木板或横梁，侧墙由石沿选项控制。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|尺寸", meta=(DisplayName="瓦面背衬厚度", ClampMin="1", ClampMax="12", Units="cm"))
 float BackingThickness = 3;
 /** 控制横向瓦片数量，实际宽度自动均分。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|瓦片", meta=(DisplayName="瓦片目标宽度", ClampMin="20", ClampMax="100", Units="cm"))
 float TileWidth = 55;
 /** 沿坡面的瓦片总长度目标，包含被上一排遮住的搭接部分。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|瓦片", meta=(DisplayName="瓦片目标长度", ClampMin="30", ClampMax="120", Units="cm"))
 float TileLength = 65;
 /** 上排瓦片覆盖下排的长度，直接生成真实搭接几何。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|瓦片", meta=(DisplayName="瓦片搭接长度", ClampMin="2", ClampMax="25", Units="cm"))
 float Overlap = 9;
 /** 瓦片垂直厚度，低坡度和密铺时自动限制以保持顺坡排水。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|瓦片", meta=(DisplayName="瓦片厚度", ClampMin="0.8", ClampMax="5", Units="cm"))
 float TileThickness = 2.2f;
 /** 横向相邻瓦片的细缝，背衬保证下方不透空。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|瓦片", meta=(DisplayName="瓦片细缝", ClampMin="0.1", ClampMax="1", Units="cm"))
 float JointWidth = .28f;
 /** 是否生成左侧随坡度上升的灰白石沿。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="启用左端石沿"))
 bool bLeftStone = true;
 /** 是否生成右侧随坡度上升的灰白石沿。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="启用右端石沿"))
 bool bRightStone = true;
 /** 是否生成坡面高端的凸起石墙和水平压顶。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="启用坡顶石沿"))
 bool bTopStone = true;
 /** 左右石沿在屋顶宽度内占用的宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="两端石沿宽度", ClampMin="8", ClampMax="50", Units="cm"))
 float EdgeWidth = 20;
 /** 两侧石沿从瓦面基准向上的高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="两端石沿高度", ClampMin="8", ClampMax="35", Units="cm"))
 float EdgeHeight = 12;
 /** 坡顶石墙沿水平进深方向的厚度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="坡顶石沿厚度", ClampMin="12", ClampMax="60", Units="cm", EditCondition="bTopStone"))
 float TopDepth = 24;
 /** 坡顶压顶最高处超出坡面高端的高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="坡顶石沿高度", ClampMin="20", ClampMax="100", Units="cm", EditCondition="bTopStone"))
 float TopHeight = 42;
 /** 石沿沿长度方向自动分块的目标尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|石沿", meta=(DisplayName="石沿分块长度", ClampMin="40", ClampMax="160", Units="cm"))
 float StoneLength = 85;
 /** 瓦片和石沿的自然低对比色差。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|外观", meta=(DisplayName="构件色差", ClampMin="0", ClampMax="0.2"))
 float ColorVariation = .1f;
 /** 固定种子控制每块瓦片和石料纹理相位，整体移动时不变。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|外观", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 1009;
 /** 与参考图一致的低饱和蓝灰石瓦材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|材质", meta=(DisplayName="蓝灰瓦片材质"))
 TObjectPtr<UMaterialInterface> TileMaterial;
 /** 与参考图一致的微暖灰白石质包边材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|材质", meta=(DisplayName="灰白石沿材质"))
 TObjectPtr<UMaterialInterface> StoneMaterial;
 /** 一个物理组件内使用斜薄板与已启用石沿的简单形状，下方房间保持通行。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 静态网格完整资产包路径，同名自动编号。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="瓦屋顶|资产", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/瓦屋顶/静态网格/瓦屋顶");
 /** 最近保存成功的静态网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|资产", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 当前瓦片数量，不包含背衬和石沿。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|统计", meta=(DisplayName="瓦片数量"))
 int32 GeneratedTileCount = 0;
 /** 瓦片沿坡面铺设的行数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|统计", meta=(DisplayName="瓦片排数"))
 int32 GeneratedRowCount = 0;
 /** 实际生成的石沿石块数量。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|统计", meta=(DisplayName="石沿块数"))
 int32 GeneratedStoneCount = 0;
 /** 有效屋顶始终只有一个渲染网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|统计", meta=(DisplayName="渲染网格数量"))
 int32 GeneratedMeshCount = 0;
 /** 整个屋顶的三角形数量。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 本会话实际几何构建次数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="瓦屋顶|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 无效输入、预算或资产保存结果的中文提示。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="瓦屋顶|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
 /** 编辑器生成 Nanite 数据；关闭后使用完整几何的普通静态网格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite虚拟化几何"))
 bool bEnableNanite = true;
private:
 /** 网格随关卡保存并参与实例复制，避免加载和进入游戏时重新构建。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已生成几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 只在形状参数变化时生成。 */
 void UpdateRoofIfNeeded();
 /** 返回全部影响几何与石纹坐标的精确参数。 */
 TArray<double> GetGeometrySettings() const;
 /** 不重建几何地刷新材质和碰撞开关。 */
 void RefreshMaterialsAndCollision();
 /** 恢复被编辑器操作注销的网格组件。 */
 void RestoreRoofRegistration();
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
 /** 合并宽深拖动与连续滑块操作的全局计时器。 */
 FTSTicker::FDelegateHandle DeferredUpdateHandle;
};
