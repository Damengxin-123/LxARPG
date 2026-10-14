#pragma once
#include "CoreMinimal.h"
#include "../SplineRoad/LxSplineRoadActor.h"
#include "LxSeawallActor.generated.h"

/** 沿道路中心线生成非对称海堤：海侧斜铺厚石板、陆侧近直立砌石，顶部复用自动道路。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="海港自动海堤", Category="海港城市"))
class LXARPG_API ALxSeawallActor : public ALxSplineRoadActor
{
 GENERATED_BODY()
public:
 /** 创建堤身网格，默认右侧面海。 */
 ALxSeawallActor();
 /** 确保两套渲染缓存都随参数与样条变化刷新。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 同时清除顶部道路与堤身。 */
 virtual void ClearRoad() override;
 /** 海堤持有与普通道路之间的端点连接片及其支撑。 */
 virtual int32 GetRoadConnectionPriority() const override { return 1; }
 /** 蓝图可调用的完整重建入口，编辑器按钮以海堤命名。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="海堤|生成", meta=(DisplayName="重建海堤"))
 void RebuildSeawall();
 /** 从路面向下至坡脚的高度；样条始终表示路面标高。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|尺寸", meta=(DisplayName="海堤高度", ClampMin="50", ClampMax="5000", Units="cm"))
 float WallHeight=500;
 /** 从水平面量起的海侧坡角，越小越平缓、占地越宽。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|尺寸", meta=(DisplayName="海侧坡角（相对水平）", ClampMin="20", ClampMax="80", Units="deg"))
 float SeaSlopeAngle=45;
 /** 陆侧偏离垂直方向的角度，零为垂直墙。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|尺寸", meta=(DisplayName="陆侧收坡角（相对垂直）", ClampMin="0", ClampMax="10", Units="deg"))
 float LandBatterAngle=3;
 /** 沿样条起点朝终点观察，默认右侧面海；勾选后切换到左侧。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|尺寸", meta=(DisplayName="左侧面海"))
 bool bSeaOnLeft=false;
 /** 大块岩板沿海堤方向的长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|岩板", meta=(DisplayName="岩板长度", ClampMin="60", ClampMax="500", Units="cm"))
 float SlabLength=180;
 /** 大块岩板沿斜坡方向的行高，实际按坡长均分。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|岩板", meta=(DisplayName="岩板坡面行高", ClampMin="60", ClampMax="500", Units="cm"))
 float SlabRowHeight=130;
 /** 岩板凸出基层的厚度，包含克制的逐块起伏。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|岩板", meta=(DisplayName="岩板厚度", ClampMin="2", ClampMax="15", Units="cm"))
 float SlabThickness=6;
 /** 岩板之间的小缝隙，缝底由连续石质基层封闭。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|岩板", meta=(DisplayName="岩板缝宽", ClampMin="0.5", ClampMax="6", Units="cm"))
 float SlabGap=2;
 /** 固定随机种子保持编辑和保存后的岩板变化一致。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|岩板", meta=(DisplayName="石纹变化种子"))
 int32 StoneSeed=17;
 /** 海侧连续基层及厚岩板共用的粗石材。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|材质", meta=(DisplayName="海侧粗岩石板材质"))
 TObjectPtr<UMaterialInterface> SeaStoneMaterial;
 /** 陆侧及端头使用规整错缝砌石。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="海堤|材质", meta=(DisplayName="陆侧石砖材质"))
 TObjectPtr<UMaterialInterface> LandBrickMaterial;
 /** 程序化堤身承担碰撞，零号材质为砌石、一号为粗石板。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海堤|组件", meta=(DisplayName="堤身网格"))
 TObjectPtr<UProceduralMeshComponent> WallMesh;
 /** 可保存的堤身优化网格，与道路独立缓存。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="海堤|组件", meta=(DisplayName="堤身优化网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> WallOptimizedMesh;
 /** 生成上限及曲率不合法时的中文提示。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海堤|诊断", meta=(DisplayName="海堤生成说明"))
 FString SeawallDiagnostic;
 /** 便于检查复杂度的实际岩板数量。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="海堤|诊断", meta=(DisplayName="岩板数量"))
 int32 GeneratedSlabCount=0;
protected:
 /** 整体变换未改变几何时，同时复用顶部道路与堤身渲染缓存。 */
 virtual void AcceptUnchangedRoadSource() override;
 /** 与路面共用精确截面，并为本海堤持有的连接片补齐砌石支撑。 */
 virtual void BuildRoadFoundation(const TArray<FLxRoadSampleFrame>& Frames, int32 MainVertexCount) override;
};
