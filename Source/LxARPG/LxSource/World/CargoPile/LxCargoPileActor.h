#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameFramework/Actor.h"
#include "LxCargoPileActor.generated.h"
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;

/** 海港日常运输货物，尺寸按厘米定义。 */
UENUM(BlueprintType, meta=(DisplayName="海运货物种类"))
enum class ELxCargoKind : uint8
{
 /** 加固大木箱。 */
 LargeCrate UMETA(DisplayName="大木箱"),
 /** 小型杂货箱。 */
 SmallCrate UMETA(DisplayName="小木箱"),
 /** 工具或长柄器具运输箱。 */
 LongCrate UMETA(DisplayName="长条木箱"),
 /** 盛放饮水或酒类的铁箍木桶。 */
 Barrel UMETA(DisplayName="铁箍木桶"),
 /** 饱满矮粮袋。 */
 GrainSack UMETA(DisplayName="粮食麻袋"),
 /** 扎口细长货袋。 */
 TallSack UMETA(DisplayName="长形麻袋"),
 /** 横放的矮扁袋。 */
 FlatSack UMETA(DisplayName="扁平麻袋"),
 /** 绳索捆扎的加工木材。 */
 Timber UMETA(DisplayName="捆扎木料"),
 /** 亚麻包装的布匹货包。 */
 ClothBale UMETA(DisplayName="捆扎布包")
};

/** 已生成货物的摆放与支撑数据，便于蓝图查询和验收。 */
USTRUCT(BlueprintType, meta=(DisplayName="货物摆放数据"))
struct FLxCargoPlacement
{
 GENERATED_BODY()
 /** 货物形态。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="货物种类"))
 ELxCargoKind Kind = ELxCargoKind::LargeCrate;
 /** 货物底面中心，相对堆放区原点。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="底部位置"))
 FVector Position = FVector::ZeroVector;
 /** 货物在自身坐标内的宽、深、高。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="货物尺寸"))
 FVector Size = FVector::ZeroVector;
 /** 围绕竖直方向旋转，货物保持稳定落地。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="水平朝向"))
 float Yaw = 0;
 /** 下方直接承重货物的索引，地面为 -1。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="承重货物索引"))
 int32 SupportIndex = INDEX_NONE;
 /** 当前货物在顶面能提供的安全支撑矩形尺寸。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="顶面支撑尺寸"))
 FVector2D SupportSize = FVector2D::ZeroVector;
 /** 转向后货物占据的水平包围矩形尺寸。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|摆放", meta=(DisplayName="占地包围尺寸"))
 FVector2D Footprint = FVector2D::ZeroVector;
};

/** 确定性矩形货物堆：货物有实际支撑，整体合并为一个静态网格。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动货物堆"))
class LXARPG_API ALxCargoPileActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建唯一网格和物理组件。 */
 ALxCargoPileActor();
 /** 编辑或生成时更新缓存。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 重载组件后恢复临时网格。 */
 virtual void PostRegisterAllComponents() override;
 /** 游戏开始时恢复网格。 */
 virtual void BeginPlay() override;
 /** 清理延迟更新回调。 */
 virtual void BeginDestroy() override;
#if WITH_EDITOR
 /** 拖动过程中保留预览。 */
 virtual void PreEditChange(FProperty* PropertyAboutToChange) override;
 /** 合并连续拖动更新。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 整体移动不重新随机分布货物。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后重新恢复几何。 */
 virtual void PostEditUndo() override;
#endif
 /** 按当前种子和尺寸立即重建。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="货物堆|生成", meta=(DisplayName="重新生成货物堆"))
 void RebuildPile();
 /** 增加种子并生成另一种摆放。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="货物堆|生成", meta=(DisplayName="换一批摆放"))
 void RerollPile();
 /** 保存可独立使用的合并静态网格与简单碰撞，同名自动编号。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="货物堆|资产", meta=(DisplayName="保存货物静态网格"))
 void SaveStaticMeshAsset();
 /** 所有货物的唯一网格组件。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|组件", meta=(DisplayName="货物合并网格"))
 TObjectPtr<UStaticMeshComponent> PileMesh;
 /** X、Y 拖动调整矩形，Z 自动归零；原点是范围左下角。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|尺寸", meta=(DisplayName="堆放范围手柄", MakeEditWidget=true, Units="cm"))
 FVector SizeHandle = FVector(1000,700,0);
 /** 货物最高点不超过此高度，不会强制填满不安全的空隙。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|尺寸", meta=(DisplayName="堆放高度上限", ClampMin="30", ClampMax="1500", Units="cm"))
 float StackHeight = 260;
 /** 控制地面堆位的占用比例，零密度清空货物。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|分布", meta=(DisplayName="堆放密度", ClampMin="0", ClampMax="1"))
 float Density = .85f;
 /** 零时按行列对齐；提高后增加水平转向和安全范围内偏移。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|分布", meta=(DisplayName="堆放混乱度", ClampMin="0", ClampMax="1"))
 float Disorder = .4f;
 /** 统一缩放货物尺寸，范围与高度仍使用世界厘米。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|分布", meta=(DisplayName="货物尺寸倍率", ClampMin="0.5", ClampMax="2"))
 float CargoScale = 1;
 /** 相同参数和种子生成相同货物，整体移动不会改变摆放。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|分布", meta=(DisplayName="随机种子"))
 int32 RandomSeed = 1009;
 /** 限制复杂度，最多生成 256 件货物。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|分布", meta=(DisplayName="货物数量上限", ClampMin="1", ClampMax="256"))
 int32 MaxCargoCount = 128;
 /** 生成大、小和长条三种加固木箱。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|货物", meta=(DisplayName="生成木箱"))
 bool bCrates = true;
 /** 生成有木板拼缝、桶盖与铁箍的木桶。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|货物", meta=(DisplayName="生成木桶"))
 bool bBarrels = true;
 /** 生成粮袋、长袋和扁袋，带扎口和缝线。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|货物", meta=(DisplayName="生成麻袋"))
 bool bSacks = true;
 /** 生成以粗绳捆扎的方木和板材。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|货物", meta=(DisplayName="生成木料"))
 bool bTimber = true;
 /** 生成亚麻包覆并以绳索捆扎的布匹包。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|货物", meta=(DisplayName="生成布包"))
 bool bCloth = true;
 /** 一个物理组件内每件货物一个简单形状，不封住整片空地。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision = true;
 /** 箱板、桶板和木料共用连续木纹。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|材质", meta=(DisplayName="暖木材质"))
 TObjectPtr<UMaterialInterface> WoodMaterial = nullptr;
 /** 用于桶箍和紧固件的克制深灰金属。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|材质", meta=(DisplayName="铁箍材质"))
 TObjectPtr<UMaterialInterface> MetalMaterial = nullptr;
 /** 麻袋与布包的粗织纹理。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|材质", meta=(DisplayName="麻布材质"))
 TObjectPtr<UMaterialInterface> ClothMaterial = nullptr;
 /** 海运货物的捆扎绳纹理。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|材质", meta=(DisplayName="绳索材质"))
 TObjectPtr<UMaterialInterface> RopeMaterial = nullptr;
 /** 同名资产自动增加编号，不覆盖已有资产。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="货物堆|资产", meta=(DisplayName="静态网格保存路径"))
 FString StaticMeshAssetPath = TEXT("/Game/项目内容/美术资产/海港城市/货物/静态网格/货物堆");
 /** 实际生成的所有货物及其承重关系。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="货物摆放列表"))
 TArray<FLxCargoPlacement> Placements;
 /** 当前货物总数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="货物数量"))
 int32 GeneratedCargoCount = 0;
 /** 合并网格的总三角形数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="三角形数量"))
 int32 GeneratedTriangleCount = 0;
 /** 本会话内的网格重建次数。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="实际重建次数"))
 int32 GeometryBuildCount = 0;
 /** 当前最高货物的高度。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="实际堆放高度"))
 float GeneratedHeight = 0;
 /** 预算、尺寸、无可用货物或导出结果的中文提示。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
 /** 最近成功保存的独立静态网格。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="货物堆|统计", meta=(DisplayName="已保存静态网格"))
 TObjectPtr<UStaticMesh> SavedStaticMesh;
 /** 编辑器生成 Nanite 数据；关闭后使用完整几何的普通静态网格。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="生成优化", meta=(DisplayName="启用Nanite虚拟化几何"))
 bool bEnableNanite = true;
private:
 /** 网格随关卡保存并参与实例复制，避免加载和进入游戏时重新构建。 */
 UPROPERTY(Instanced, VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="已生成几何缓存"))
 TObjectPtr<UStaticMesh> GeneratedMeshCache;
 /** 只在几何参数变化时重建。 */
 void UpdatePileIfNeeded();
 /** 收集影响布局和外观的参数。 */
 TArray<double> GetGeometrySettings() const;
 /** 不重建地更新材质和碰撞。 */
 void RefreshMaterialsAndCollision();
 /** 保持编辑器预览组件注册。 */
 void RestorePileRegistration();
 /** 取消未提交的延迟任务。 */
 void CancelDeferredUpdate();
 /** 上次成功生成的参数。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 TArray<double> CachedGeometrySettings;
 /** 是否已存在有效缓存。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category="生成优化", meta=(DisplayName="几何缓存状态"))
 bool bGeometryCached = false;
 /** 最近交互事件时间。 */
 double LastEditTime = 0;
 /** 连续拖动合并更新的计时器。 */
 FTSTicker::FDelegateHandle DeferredUpdateHandle;
};
