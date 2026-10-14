#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LxMechanismOperationComponent.generated.h"

class USceneComponent;
class USoundBase;
class UAudioComponent;

/** 每个门扇独立保存的旋转任务；弱引用不延长组件生命周期。 */
USTRUCT()
struct FLxMechanismRotationTask
{
 GENERATED_BODY()
 /** 被控制的可移动组件。 */
 TWeakObjectPtr<USceneComponent> Target;
 /** 此次动画的起始相对旋转。 */
 FQuat Start = FQuat::Identity;
 /** 此次动画的结束相对旋转。 */
 FQuat End = FQuat::Identity;
 /** 动画总时长，单位秒。 */
 float Duration = 0;
 /** 已经播放的时间。 */
 float Elapsed = 0;
 /** 随动画停止的空间音效实例。 */
 TWeakObjectPtr<UAudioComponent> Audio;
};

/** 可复用的机关运行组件，统一控制多个门扇的缓入缓出旋转和音效。 */
UCLASS(ClassGroup=(Custom), BlueprintType, meta=(BlueprintSpawnableComponent, DisplayName="机关运行组件"))
class LXARPG_API ULxMechanismOperationComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 /** 空闲时关闭逐帧更新。 */
 ULxMechanismOperationComponent();
 /** 按相对旋转的最短路径播放；同组件重新调用从当前姿态接续，零时长立即完成且静音。 */
 UFUNCTION(BlueprintCallable, Category="机关运行|旋转", meta=(DisplayName="旋转控制器"))
 bool RotateComponent(UPARAM(DisplayName="门组件") USceneComponent* Target,
  UPARAM(DisplayName="旋转前角度") FRotator FromRotation, UPARAM(DisplayName="旋转后角度") FRotator ToRotation,
  UPARAM(DisplayName="旋转时间") float Duration, UPARAM(DisplayName="旋转音效") USoundBase* Sound = nullptr);
 /** 停止指定组件的动画和声音，可选择吸附到目标角度。 */
 UFUNCTION(BlueprintCallable, Category="机关运行|旋转", meta=(DisplayName="停止旋转"))
 void StopRotation(UPARAM(DisplayName="门组件") USceneComponent* Target, UPARAM(DisplayName="到达目标") bool bFinish = false);
 /** 查询指定组件是否还在旋转。 */
 UFUNCTION(BlueprintPure, Category="机关运行|旋转", meta=(DisplayName="是否正在旋转"))
 bool IsRotating(UPARAM(DisplayName="门组件") USceneComponent* Target) const;
 /** 单次更新所有活动任务，空闲后停止更新。 */
 virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
 /** 所属对象结束运行时停止声音并释放任务。 */
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 /** 活动任务表，每个组件最多一个任务。 */
 TArray<FLxMechanismRotationTask> Tasks;
};
