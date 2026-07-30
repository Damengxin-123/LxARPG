#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LxARPG/LxSource/Model/AI/DataType/LxAITypes.h"
#include "LxAIGroupSubsystem.generated.h"

class AActor;
class ALxAICharacter;

/** 群体成员感知到目标时保存的共享情报。 */
struct FLxAISharedPerceptionRecord
{
	/** 被群体成员感知到的目标。 */
	TWeakObjectPtr<AActor> TargetActor;

	/** 最近一次确认感知到目标的世界时间。 */
	double LastSensedTime = 0.0;

	/** 最近一次确认该目标的群体成员。 */
	TWeakObjectPtr<ALxAICharacter> SourceCharacter;

	/** 是否有群体成员因目标造成伤害而确认了动态敌对行为。 */
	bool bHostileBehaviorConfirmed = false;
};

/** 单个AI群体的运行时共享数据。 */
struct FLxAIGroupRuntimeData
{
	/** 当前注册到群体中的AI成员。 */
	TSet<TWeakObjectPtr<ALxAICharacter>> Members;

	/** 按目标保存的最新共享感知记录。 */
	TMap<TWeakObjectPtr<AActor>, FLxAISharedPerceptionRecord> SharedTargets;

	/** 每个成员当前占用的自动行为。 */
	TMap<TWeakObjectPtr<ALxAICharacter>, ELxAIActionType> MemberActions;
};

/** 在同一世界内集中共享AI群体成员、感知目标和行为占用。 */
UCLASS(DisplayName="AI群体共享子系统")
class LXARPG_API ULxAIGroupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 将AI角色注册到指定群体；无效群体ID不会注册。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体", DisplayName="注册AI群体成员")
	void RegisterMember(FName InGroupId, ALxAICharacter* InCharacter);

	/** 从指定群体移除AI角色及其行为占用。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体", DisplayName="注销AI群体成员")
	void UnregisterMember(FName InGroupId, ALxAICharacter* InCharacter);

	/** 将成员直接感知到的目标写入群体共享情报。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体|感知", DisplayName="报告群体感知目标")
	void ReportSensedTarget(FName InGroupId, ALxAICharacter* InSourceCharacter, AActor* InTargetActor,
		bool bInHostileBehavior = false);

	/** 获取指定有效时间内的全部共享目标。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体|感知", DisplayName="获取群体共享目标")
	void GetSharedTargets(FName InGroupId, float InMaxAge, TArray<AActor*>& OutTargets) const;

	/** 判断群体是否已经根据目标造成伤害等行为确认其动态敌对关系。 */
	UFUNCTION(BlueprintPure, Category="AI|群体|感知", DisplayName="群体目标是否动态敌对")
	bool IsTargetMarkedHostile(FName InGroupId, const AActor* InTargetActor) const;

	/** 获取仍然有效的全部群体成员。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体", DisplayName="获取AI群体成员")
	void GetGroupMembers(FName InGroupId, TArray<ALxAICharacter*>& OutMembers) const;

	/** 更新某个群体成员当前占用的行为类型。 */
	UFUNCTION(BlueprintCallable, Category="AI|群体|行为", DisplayName="设置群体成员行为")
	void SetMemberAction(FName InGroupId, ALxAICharacter* InCharacter, ELxAIActionType InActionType);

	/** 统计群体内当前执行指定行为的有效成员数量。 */
	UFUNCTION(BlueprintPure, Category="AI|群体|行为", DisplayName="获取群体行为执行人数")
	int32 GetActionExecutorCount(FName InGroupId, ELxAIActionType InActionType) const;

private:
	/** 按群体ID保存的运行时共享数据。 */
	TMap<FName, FLxAIGroupRuntimeData> GroupRuntimeData;
};
