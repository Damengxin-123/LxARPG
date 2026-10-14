#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Interaction/DataType/LxInteractionEnum.h"
#include "LxWoodenDoorActor.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
class UBoxComponent;
class UMaterialInterface;
class USoundBase;
class ULxInteractableComponent;
class ULxMechanismOperationComponent;

/** 带拼板、包边、铰链和菱形花饰的自动木门，使用已有机关交互。 */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="自动木门"))
class LXARPG_API ALxWoodenDoorActor : public AActor
{
 GENERATED_BODY()
public:
 /** 创建门框、独立门扇、碰撞、交互和运行组件。 */
 ALxWoodenDoorActor();
 /** 修改尺寸时重建，移动时复用缓存。 */
 virtual void OnConstruction(const FTransform& Transform) override;
 /** 监听原有机关，立即恢复首次姿态。 */
 virtual void BeginPlay() override;
 /** 解除状态监听。 */
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
 /** 批量改参数后重新生成门体。 */
 UFUNCTION(BlueprintCallable, CallInEditor, Category="木门|生成", meta=(DisplayName="重新生成木门"))
 void RebuildDoor();
 /** 控制两扇门的表现，机关状态仍由原有交互系统维护。 */
 UFUNCTION(BlueprintCallable, Category="木门|机关", meta=(DisplayName="播放木门开关"))
 void AnimateDoor(UPARAM(DisplayName="开启") bool bOpen, UPARAM(DisplayName="立即恢复") bool bImmediate);
 /** 蓝图事件图将状态通知连接到播放接口，可扩展其他表现。 */
 UFUNCTION(BlueprintImplementableEvent, Category="木门|机关", meta=(DisplayName="木门机关状态更新"))
 void ReceiveDoorState(UPARAM(DisplayName="开启") bool bOpen, UPARAM(DisplayName="立即恢复") bool bImmediate);
 /** 原点在门底中央，X宽、Y厚、Z高。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="门体根组件"))
 TObjectPtr<USceneComponent> DoorRoot;
 /** 固定门框。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="固定门框"))
 TObjectPtr<UStaticMeshComponent> FrameMesh;
 /** 左铰链为局部原点。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="左门扇"))
 TObjectPtr<UStaticMeshComponent> LeftLeaf;
 /** 右铰链为局部原点，单扇时隐藏。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="右门扇"))
 TObjectPtr<UStaticMeshComponent> RightLeaf;
 /** 随左门旋转的唯一碰撞盒。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="左门碰撞"))
 TObjectPtr<UBoxComponent> LeftCollision;
 /** 随右门旋转的唯一碰撞盒。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="右门碰撞"))
 TObjectPtr<UBoxComponent> RightCollision;
 /** 两侧及顶梁碰撞，保留中央门洞。 */
 UPROPERTY(VisibleAnywhere, Category="木门|组件", meta=(DisplayName="门框碰撞"))
 TArray<TObjectPtr<UBoxComponent>> FrameCollisions;
 /** 复用交互树、状态存档和网络同步。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="可交互对象"))
 TObjectPtr<ULxInteractableComponent> Interactable;
 /** 通用缓入缓出旋转控制器。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|组件", meta=(DisplayName="机关运行组件"))
 TObjectPtr<ULxMechanismOperationComponent> Operation;
 /** 外框总宽，单位厘米。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|尺寸", meta=(DisplayName="整体宽度", ClampMin="70", ClampMax="800", Units="cm"))
 float Width = 130;
 /** 外框总高，单位厘米。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|尺寸", meta=(DisplayName="整体高度", ClampMin="100", ClampMax="800", Units="cm"))
 float Height = 240;
 /** 外框总厚，门扇按比例生成。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|尺寸", meta=(DisplayName="整体厚度", ClampMin="8", ClampMax="60", Units="cm"))
 float Depth = 18;
 /** 开启时生成镜像对开的两扇门。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|构造", meta=(DisplayName="双扇门"))
 bool bDoubleDoor = false;
 /** 两侧立柱与上横梁的宽度。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|构造", meta=(DisplayName="门框宽度", ClampMin="4", ClampMax="30", Units="cm"))
 float FrameWidth = 10;
 /** 每扇竖向木板数量。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|构造", meta=(DisplayName="每扇木板数", ClampMin="3", ClampMax="12"))
 int32 PlankCount = 5;
 /** 两面生成菱形花饰与铆钉。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|外观", meta=(DisplayName="启用花纹装饰"))
 bool bOrnaments = true;
 /** 相对关闭姿态的开门角度，负数反向开启。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|机关", meta=(DisplayName="开门角度", ClampMin="-170", ClampMax="170", Units="deg"))
 float OpenAngle = 100;
 /** 缓入缓出旋转的持续时间。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|机关", meta=(DisplayName="旋转时间", ClampMin="0", ClampMax="30", Units="s"))
 float RotationTime = 1.4f;
 /** 双门仅左扇播放声音，避免叠加。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|音效", meta=(DisplayName="开门音效"))
 TObjectPtr<USoundBase> OpenSound;
 /** 关闭时的独立音效。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|音效", meta=(DisplayName="关门音效"))
 TObjectPtr<USoundBase> CloseSound;
 /** 暖木色竖向木纹。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|材质", meta=(DisplayName="门板材质"))
 TObjectPtr<UMaterialInterface> WoodMaterial;
 /** 深棕色框和边条。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|材质", meta=(DisplayName="边框材质"))
 TObjectPtr<UMaterialInterface> FrameMaterial;
 /** 克制的金属装饰。 */
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="木门|材质", meta=(DisplayName="金属材质"))
 TObjectPtr<UMaterialInterface> MetalMaterial;
 /** 生成失败的中文诊断。 */
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="木门|生成", meta=(DisplayName="生成提示"))
 FString GenerationDiagnostic;
private:
 /** 可随关卡保存的几何。 */
 UPROPERTY(Instanced, VisibleAnywhere, Category="木门|缓存", meta=(DisplayName="几何缓存"))
 TArray<TObjectPtr<UStaticMesh>> MeshCache;
 /** 仅形状变化触发重建。 */
 UPROPERTY(VisibleAnywhere, Category="木门|缓存", meta=(DisplayName="参数缓存"))
 TArray<double> CachedSettings;
 /** 同步当前交互树的机关状态。 */
 void SynchronizeMechanism();
 /** 转发原有机关通知。 */
 void HandleMechanismState(ELxMechanismState State);
 /** 蓝图未实现事件时使用原生表现回退。 */
 void DispatchDoorState(bool bOpen, bool bImmediate);
};
