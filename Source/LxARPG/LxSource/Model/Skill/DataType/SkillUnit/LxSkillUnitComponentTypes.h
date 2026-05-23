#pragma once

#include "CoreMinimal.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillUnitEnum.h"
#include "LxSkillUnitComponentTypes.generated.h"

class AActor;
class UActorComponent;
class UPrimitiveComponent;

/** 技能能力组件运行状态。 */
UENUM(BlueprintType, DisplayName="技能能力组件状态")
enum class ELxSkillAbilityComponentState : uint8
{
	Ready		UMETA(DisplayName="就绪"),
	Running		UMETA(DisplayName="运行中"),
	Paused		UMETA(DisplayName="暂停"),
	Stopped		UMETA(DisplayName="终止"),
	Finished	UMETA(DisplayName="结束")
};

/** 技能检测事件类型，表示检测组件发现了什么。 */
UENUM(BlueprintType, DisplayName="技能检测事件类型")
enum class ELxSkillDetectionEventType : uint8
{
	None		UMETA(DisplayName="无"),
	OverlapBegin	UMETA(DisplayName="开始重叠"),
	OverlapEnd		UMETA(DisplayName="结束重叠"),
	HitTarget		UMETA(DisplayName="命中目标"),
	HitWorld		UMETA(DisplayName="命中场景"),
	ManualScan		UMETA(DisplayName="手动扫描")
};

/** 技能传播评估结果类型。 */
UENUM(BlueprintType, DisplayName="技能传播评估类型")
enum class ELxSkillPropagationDecisionType : uint8
{
	None	UMETA(DisplayName="无传播"),
	Pierce	UMETA(DisplayName="穿透"),
	Bounce	UMETA(DisplayName="弹跳"),
	Chain	UMETA(DisplayName="连锁"),
	Split	UMETA(DisplayName="分裂"),
	Infect	UMETA(DisplayName="扩散感染"),
	Ended	UMETA(DisplayName="传播结束")
};

/** 检测组件输出的数据，只表达检测到了什么，不决定是否真正触发效果。 */
USTRUCT(BlueprintType, DisplayName="技能检测结果")
struct FLxSkillDetectionResult
{
	GENERATED_BODY()

	/** 检测事件类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="检测事件类型")
	ELxSkillDetectionEventType EventType = ELxSkillDetectionEventType::None;

	/** 产生检测结果的组件。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="来源组件")
	TObjectPtr<UActorComponent> SourceComponent = nullptr;

	/** 产生检测结果的技能单元。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="来源技能单元")
	TObjectPtr<AActor> SourceUnit = nullptr;

	/** 本次检测得到的候选目标列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="候选目标列表")
	TArray<TObjectPtr<AActor>> CandidateTargets;

	/** 本次检测命中的单个目标，适用于碰撞、射线、近战轨迹等单点结果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="命中目标")
	TObjectPtr<AActor> HitActor = nullptr;

	/** 命中位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="命中位置")
	FVector HitLocation = FVector::ZeroVector;

	/** 命中法线。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="命中法线")
	FVector HitNormal = FVector::ZeroVector;

	/** 是否命中了场景障碍物。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="命中场景")
	bool bHitWorld = false;

	/** 触发检测的碰撞体。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|检测", DisplayName="触发碰撞体")
	TObjectPtr<UPrimitiveComponent> TriggerCollision = nullptr;
};

/** 触发组件输出的数据，表示候选目标经过触发条件和命中限制后，最终是否触发。 */
USTRUCT(BlueprintType, DisplayName="技能触发结果")
struct FLxSkillTriggerResult
{
	GENERATED_BODY()

	/** 本次触发是否成功。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="是否触发")
	bool bTriggered = false;

	/** 触发来源检测结果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="检测结果")
	FLxSkillDetectionResult DetectionResult;

	/** 最终确认触发的目标列表。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="触发目标列表")
	TArray<TObjectPtr<AActor>> TriggeredTargets;

	/** 本组件累计触发次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|触发", DisplayName="累计触发次数")
	int32 TriggeredCount = 0;
};

/** 传播组件输出的数据，表示命中后下一步是否还能穿透、弹跳、连锁或分裂。 */
USTRUCT(BlueprintType, DisplayName="技能传播结果")
struct FLxSkillPropagationResult
{
	GENERATED_BODY()

	/** 本次传播决策类型。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="传播决策类型")
	ELxSkillPropagationDecisionType DecisionType = ELxSkillPropagationDecisionType::None;

	/** 是否允许继续传播。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="可以继续传播")
	bool bCanContinue = false;

	/** 剩余穿透次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="剩余穿透次数")
	int32 RemainingPierceCount = 0;

	/** 剩余弹跳次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="剩余弹跳次数")
	int32 RemainingBounceCount = 0;

	/** 剩余连锁次数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="剩余连锁次数")
	int32 RemainingChainCount = 0;

	/** 分裂数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="分裂数量")
	int32 SplitCount = 0;

	/** 传播来源触发结果。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="技能单元|传播", DisplayName="触发结果")
	FLxSkillTriggerResult TriggerResult;
};
