#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Core/Database/LxComponentBase.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillTargetFilterSpec.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitComponentTypes.h"
#include "LxSkillDetectionComponent.generated.h"

class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLxSkillDetectionResult, const FLxSkillDetectionResult&, DetectionResult);

/** 技能目标检测组件，只负责从碰撞、重叠或手动输入中产生候选目标。 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable, DisplayName="技能目标检测组件")
class LXARPG_API ULxSkillDetectionComponent : public ULxComponentBase
{
	GENERATED_BODY()

public:
	/** 传入目标筛选参数。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|检测", DisplayName="传入目标筛选参数")
	void SetTargetFilterSpec(const FLxSkillTargetFilterSpec& InTargetFilterSpec);

	/** 传入用于检测重叠和碰撞的碰撞体。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|检测", DisplayName="传入技能单元触发碰撞体")
	void SetTriggerCollisionComponent(UPrimitiveComponent* InTriggerCollisionComponent);

	/** 开始检测，并绑定碰撞体事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|检测", DisplayName="开始检测")
	void StartDetection();

	/** 停止检测，并解绑碰撞体事件。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|检测", DisplayName="停止检测")
	void StopDetection();

	/** 手动发布一组候选目标，适合射线、范围扫描、近战轨迹等外部检测结果接入。 */
	UFUNCTION(BlueprintCallable, Category="技能单元|检测", DisplayName="发布手动检测结果")
	void PublishManualDetectionResult(const TArray<AActor*>& InCandidateTargets);

	/** 获取当前仍在重叠范围内的候选目标。 */
	UFUNCTION(BlueprintPure, Category="技能单元|检测", DisplayName="获取当前候选目标")
	TArray<AActor*> GetCurrentCandidateTargets() const;

	/** 检测结果发布事件。 */
	UPROPERTY(BlueprintAssignable, Category="技能单元|检测", DisplayName="检测结果发布事件")
	FOnLxSkillDetectionResult OnDetectionResult;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	bool IsBasicCandidateValid(AActor* InActor) const;
	void PublishSingleActorResult(ELxSkillDetectionEventType EventType, AActor* InActor, const FVector& HitLocation, const FVector& HitNormal, bool bHitWorld);

	/** 检测参数，只做候选目标的基础筛选，深度命中限制交给触发组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="目标筛选参数", meta=(AllowPrivateAccess="true"))
	FLxSkillTargetFilterSpec TargetFilterSpec;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> TriggerCollisionComponent = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> CurrentCandidateTargets;

	bool bDetecting = false;
};
