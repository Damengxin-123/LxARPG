#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Containers/Ticker.h"
#include "LxRoundBuildingActor.generated.h"
class UProceduralMeshComponent;
class ULxGeneratedMeshRenderComponent;
class ALxWoodenDoorActor;

/** 圆墙上的矩形洞口，门洞底部跟随地板，窗洞允许上下拖动。 */
USTRUCT(BlueprintType, meta=(DisplayName="圆形建筑洞口"))
struct FLxRoundBuildingOpening
{
 GENERATED_BODY()
 /** 详情列表名称。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="洞口", meta=(DisplayName="名称"))
 FString Name=TEXT("门洞");
 /** 门洞固定落地，窗洞使用手柄高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="洞口", meta=(DisplayName="门洞（否则为窗洞）"))
 bool bDoor=true;
 /** 底部中心手柄；水平拖动调整圆周位置，窗洞可沿 Z 调高低。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="洞口", meta=(DisplayName="位置手柄", MakeEditWidget))
 FVector Position=FVector(500,0,20);
 /** 切口的直线净宽，不是圆弧长度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="洞口", meta=(DisplayName="净宽", ClampMin="10", Units="cm"))
 float Width=134;
 /** 矩形切口净高。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="洞口", meta=(DisplayName="净高", ClampMin="10", Units="cm"))
 float Height=244;
};

/** 带矩形门窗、内向楼板、城垛及内嵌高塔楼板开孔的圆柱形建筑。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="圆形建筑"))
class LXARPG_API ALxRoundBuildingActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建几何组件和延迟优化渲染组件。 */
 ALxRoundBuildingActor();
 /** 更新本体及嵌套建筑。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 禁止优化渲染组件被单独拖离建筑。 */
 virtual void PostRegisterAllComponents() override;
 /** 删除后恢复邻居楼板。 */
 virtual void Destroyed() override;
 /** 清理延迟任务。 */
 virtual void BeginDestroy() override;
 /** 重建当前建筑和受影响的嵌套建筑。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="圆形建筑|生成", meta=(DisplayName="重建圆形建筑"))
 void RebuildBuilding();
 /** 添加默认适配现有木门的洞口。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="圆形建筑|洞口", meta=(DisplayName="添加门洞"))
 void AddDoorOpening();
 /** 添加可沿墙面及高度拖动的窗洞。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="圆形建筑|洞口", meta=(DisplayName="添加窗洞"))
 void AddWindowOpening();
 /** 按木门外框加安装余量设置指定门洞。 */
 UFUNCTION(BlueprintCallable, Category="圆形建筑|洞口", meta=(DisplayName="按木门调整门洞"))
 bool FitOpeningToDoor(int32 OpeningIndex, ALxWoodenDoorActor* Door, float Clearance=2);
 /** 获取门底中央安装变换，X 沿门宽方向，可用于放置现有木门。 */
 UFUNCTION(BlueprintPure, Category="圆形建筑|洞口", meta=(DisplayName="获取洞口安装变换"))
 FTransform GetOpeningTransform(int32 OpeningIndex) const;
#if WITH_EDITOR
 /** 连续拖动时防抖，避免逐次布尔运算。 */
 virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
 /** 整体移动结束后更新包含关系。 */
 virtual void PostEditMove(bool bFinished) override;
 /** 撤销后更新几何。 */
 virtual void PostEditUndo() override;
#endif
 /** 本地原点在墙体底面中心。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="圆形建筑|组件", meta=(DisplayName="建筑网格"))
 TObjectPtr<UProceduralMeshComponent> BuildingMesh;
 /** 编辑预览与保存、运行前的静态网格优化。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="生成优化", meta=(DisplayName="优化渲染网格"))
 TObjectPtr<ULxGeneratedMeshRenderComponent> OptimizedMesh;
 /** 外墙半径。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|尺寸", meta=(DisplayName="外墙半径", ClampMin="100", Units="cm"))
 float Radius=500;
 /** 厚度向房间内侧增长。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|尺寸", meta=(DisplayName="墙体厚度", ClampMin="5", Units="cm"))
 float WallThickness=30;
 /** 包括城垛在内的最高高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|尺寸", meta=(DisplayName="墙体总高度", ClampMin="100", Units="cm"))
 float WallHeight=400;
 /** 是否生成地板。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="生成地板"))
 bool bFloor=true;
 /** 地板从底面向上加厚。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="地板厚度", ClampMin="1", Units="cm", EditCondition="bFloor"))
 float FloorThickness=20;
 /** 是否生成天花板。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="生成天花板"))
 bool bCeiling=true;
 /** 调整墙高时自动保持墙顶与天花板的距离。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="天花板跟随墙高"))
 bool bCeilingFollowsWall=true;
 /** 天花板以上预留的护墙或城垛高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="墙顶预留高度", ClampMin="0", Units="cm", EditCondition="bCeilingFollowsWall"))
 float CeilingTopOffset=100;
 /** 手动模式的天花板顶面高度，自动限制在墙顶以下。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="天花板顶面高度", ClampMin="10", Units="cm", EditCondition="!bCeilingFollowsWall"))
 float CeilingHeight=300;
 /** 天花板从顶面向下加厚。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|楼板", meta=(DisplayName="天花板厚度", ClampMin="1", Units="cm", EditCondition="bCeiling"))
 float CeilingThickness=20;
 /** 无固定数量限制，可在列表增删并拖动手柄定位。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|洞口", meta=(DisplayName="门窗洞口", TitleProperty="Name"))
 TArray<FLxRoundBuildingOpening> Openings;
 /** 在门窗外侧生成矩形石质装饰，不侵占净洞口。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|洞口装饰", meta=(DisplayName="生成门窗装饰"))
 bool bOpeningTrim=true;
 /** 两侧石框及顶梁的宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|洞口装饰", meta=(DisplayName="装饰边框宽度", ClampMin="1", Units="cm", EditCondition="bOpeningTrim"))
 float OpeningTrimWidth=12;
 /** 石框超出外墙的距离，窗台额外外挑。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|洞口装饰", meta=(DisplayName="装饰凸起厚度", ClampMin="1", Units="cm", EditCondition="bOpeningTrim"))
 float OpeningTrimDepth=8;
 /** 门窗石框使用独立的略深色材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|材质", meta=(DisplayName="门窗装饰材质"))
 TObjectPtr<UMaterialInterface> OpeningTrimMaterial;
 /** 仅处理包含关系，在内嵌建筑的净空范围给楼板开孔。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|组合", meta=(DisplayName="内嵌建筑楼板开孔"))
 bool bNestedFloorHoles=true;
 /** 在天花板以上生成高低交替的城垛。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|城垛", meta=(DisplayName="生成城垛"))
 bool bBattlements=false;
 /** 圆周高墙数量。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|城垛", meta=(DisplayName="城垛数量", ClampMin="3", ClampMax="64", EditCondition="bBattlements"))
 int32 BattlementCount=16;
 /** 凹口深度，自动限制为天花板以上可用高度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|城垛", meta=(DisplayName="城垛凹口深度", ClampMin="0", Units="cm", EditCondition="bBattlements"))
 float NotchDepth=50;
 /** 每个周期高墙所占比例。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|城垛", meta=(DisplayName="城垛宽度比例", ClampMin="0.1", ClampMax="0.9", EditCondition="bBattlements"))
 float BattlementRatio=0.5f;
 /** 圆周质量，门窗使用平面切割。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|质量", meta=(DisplayName="圆周分段数", ClampMin="24", ClampMax="256"))
 int32 CircleSegments=96;
 /** 墙体和洞口内侧材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|材质", meta=(DisplayName="墙体材质"))
 TObjectPtr<UMaterialInterface> WallMaterial;
 /** 地板材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|材质", meta=(DisplayName="地板材质"))
 TObjectPtr<UMaterialInterface> FloorMaterial;
 /** 天花板材质。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|材质", meta=(DisplayName="天花板材质"))
 TObjectPtr<UMaterialInterface> CeilingMaterial;
 /** 按厘米计算的纹理重复尺寸。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|材质", meta=(DisplayName="贴图覆盖尺寸", ClampMin="1", Units="cm"))
 float TextureSize=200;
 /** 为最终表面创建角色与查询碰撞。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="圆形建筑|碰撞", meta=(DisplayName="启用碰撞"))
 bool bEnableCollision=true;
 /** 几何生成失败原因。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="圆形建筑|诊断", meta=(DisplayName="生成诊断"))
 FString GenerationDiagnostic;
private:
 /** 旧邻居用于移动、删除后的楼板恢复。 */
 UPROPERTY(VisibleAnywhere, AdvancedDisplay, meta=(DisplayName="上次嵌套邻居", Category="生成优化"))
 TArray<TWeakObjectPtr<ALxRoundBuildingActor>> PreviousNeighbors;
 /** 编辑防抖句柄。 */
 FTSTicker::FDelegateHandle DeferredUpdate;
 /** 最后一次连续编辑时间。 */
 double LastEditTime=0;
 /** 清理编辑防抖。 */
 void CancelDeferredUpdate();
 /** 生成当前几何，不递归刷新邻居。 */
 void BuildGeometry(const TArray<ALxRoundBuildingActor*>& Neighbors);
 /** 查找存在高度交集的包含或被包含建筑。 */
 TArray<ALxRoundBuildingActor*> FindNeighbors() const;
};
