#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Skill/DataType/LxSkillFlowAsset.h"
#include "LxARPG/LxSource/Model/Skill/DataType/SkillUnit/LxSkillUnitResult.h"
#include "LxSkillFlowExecution.generated.h"

class ULxSkill;
class ULxSkillUnitGroup;

/** 一次事件传递的独立结果，排队期间保持目标引用。 */
USTRUCT(meta=(DisplayName="技能流程待执行节点"))
struct FLxSkillFlowTask
{
	GENERATED_BODY()
	/** 需要执行的节点。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="节点标识", Category="流程"))
	FGuid NodeId;
	/** 当前这次事件产生的结果。 */
	UPROPERTY(VisibleAnywhere, meta=(DisplayName="前置结果", Category="流程"))
	FLxSkillUnitResult Result;
};

/** 世界持有的单次流程实例，角色结束释放后仍保留自主单元的后续流程。 */
UCLASS(NotBlueprintable, Transient, meta=(DisplayName="技能流程执行实例"))
class LXARPG_API ALxSkillFlowExecution : public AActor
{
	GENERATED_BODY()
public:
	/** 启用事件队列处理，不复制控制实例到客户端。 */
	ALxSkillFlowExecution();
	/** 从技能创建独立上下文和配置快照。 */
	bool Initialize(ULxSkill* Source);
	/** 接收角色事件，只对维持单元转发停止或取消通知。 */
	void SendEvent(ELxSkillFlowEvent Event);
	/** 更新维持单元的瞄准，自主单元保持自身运动。 */
	void UpdateAim(const FTransform& Transform);
	/** 处理排队节点，并在所有单元结束后清理实例。 */
	virtual void Tick(float DeltaSeconds) override;
	/** 世界结束时解绑事件，不主动销毁自主单元。 */
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	/** 返回本次仍在运行的单元组，供调试与自动化验证。 */
	TArray<ULxSkillUnitGroup*> GetActiveGroups() const;
private:
	/** 查找本次释放复制的节点配置。 */
	const ULxSkillFlowNode* FindNode(const FGuid& Id) const;
	/** 将后续节点与当前结果加入队列，避免同步命中造成递归执行。 */
	void Enqueue(const TArray<FGuid>& Links, const FLxSkillUnitResult& Result);
	/** 创建并激活一个节点对应的技能单元组。 */
	void Execute(const FLxSkillFlowTask& Task);
	/** 接收一次命中结果，应用词条并启动对应后续流程。 */
	UFUNCTION(Category="技能|流程", meta=(DisplayName="接收流程单元命中"))
	void OnHit(ULxSkillUnitGroup* Group, const FLxSkillUnitResult& Result);
	/** 单元自行完成后继续结束分支并解除引用。 */
	void OnFinished(ULxSkillUnitGroup* Group, const FLxSkillUnitResult& Result);
	/** 仅用于复用现有单元创建与词条投递逻辑，永不接收下一次释放上下文。 */
	UPROPERTY(Transient, VisibleAnywhere, meta=(DisplayName="本次技能上下文", Category="运行"))
	TObjectPtr<ULxSkill> Worker;
	/** 静态配置的本次快照。 */
	UPROPERTY(Transient, VisibleAnywhere, meta=(DisplayName="节点快照", Category="运行"))
	TArray<TObjectPtr<ULxSkillFlowNode>> Nodes;
	/** 活跃组到其配置节点的对应关系。 */
	UPROPERTY(Transient, VisibleAnywhere, meta=(DisplayName="活跃单元组", Category="运行"))
	TMap<TObjectPtr<ULxSkillUnitGroup>, FGuid> Groups;
	/** 待执行的事件任务。 */
	UPROPERTY(Transient, VisibleAnywhere, meta=(DisplayName="待执行节点", Category="运行"))
	TArray<FLxSkillFlowTask> Pending;
	/** 是否仍接受本次释放的维持通知。 */
	bool bControlOpen = true;
	/** 世界退出后不再执行后续分支。 */
	bool bShuttingDown = false;
};
