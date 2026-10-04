#include "LxSkillFlowExecution.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxElementAbnormalAttachSkillUnitActor.h"
#include "LxSkill.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxStraightProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDirectHitAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxGroundBounceProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxLobProjectileSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxDurationAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxScalingAreaSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMeleeSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSingleRaySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAttachEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxContinuousAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxPeriodicAuraEffectSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSpawnEntitySkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxBarrierSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxMarkerSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSummonCreatureSkillUnitActor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxTriggerSkillUnitActor.h"

ALxSkillFlowExecution::ALxSkillFlowExecution()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
}

bool ALxSkillFlowExecution::Initialize(ULxSkill* Source)
{
	if (!Source || !Source->FlowAsset) return false;
	FText Error;
	if (!Source->FlowAsset->Validate(Error))
	{
		UE_LOG(LogTemp, Error, TEXT("技能流程无法运行：%s"), *Error.ToString());
		return false;
	}
	Worker = NewObject<ULxSkill>(this);
	Worker->CurrentCastContext = Source->CurrentCastContext;
	Worker->CurrentCastContext.SpawnTransform = Source->GetSkillSpawnTransform();
	Worker->CurrentCastContext.bOverrideSpawnTransform = true;
	Worker->CurrentCastContext.WorldContextObject = this;
	Worker->SkillEntryPackages = Source->FlowAsset->EntryPackages;
	Worker->FlowSourceSkillId = Source->GetSkillIDTag();
	// 保留现有权威端词条投递链路，委托中的接收方仍由弱对象绑定保护。
	Worker->OnSkillHitEntriesReady = Source->OnSkillHitEntriesReady;
	Worker->OnPersistentSkillHitEntriesReady = Source->OnPersistentSkillHitEntriesReady;
	Worker->OnSkillEffectsRemoved = Source->OnSkillEffectsRemoved;
	for (const ULxSkillFlowNode* Node : Source->FlowAsset->Nodes)
		Nodes.Add(DuplicateObject<ULxSkillFlowNode>(Node, this));
	return true;
}

const ULxSkillFlowNode* ALxSkillFlowExecution::FindNode(const FGuid& Id) const
{
	for (const ULxSkillFlowNode* Node : Nodes) if (Node->Id == Id) return Node;
	return nullptr;
}

void ALxSkillFlowExecution::Enqueue(const TArray<FGuid>& Links, const FLxSkillUnitResult& Result)
{
	if (bShuttingDown) return;
	for (const FGuid& Id : Links)
	{
		FLxSkillFlowTask& Task = Pending.AddDefaulted_GetRef();
		Task.NodeId = Id;
		Task.Result = Result;
	}
}

void ALxSkillFlowExecution::SendEvent(ELxSkillFlowEvent Event)
{
	if (!Worker || bShuttingDown) return;
	const bool bEndsControl = Event == ELxSkillFlowEvent::ReleaseEnd || Event == ELxSkillFlowEvent::Cancel
		|| Event == ELxSkillFlowEvent::ChargeEnd || Event == ELxSkillFlowEvent::Direct;
	if (!bControlOpen && bEndsControl) return;
	if (bEndsControl)
	{
		bControlOpen = false;
		// 先关闭维持入口，再发通知，防止结束回调重新创建维持单元。
		for (ULxSkillUnitGroup* Group : GetActiveGroups())
		{
			const FGuid* Id = Groups.Find(Group);
			const ULxSkillFlowNode* Node = Id ? FindNode(*Id) : nullptr;
			if (Node && Node->Lifetime == ELxSkillFlowLifetime::Maintained)
			{
				if (Event == ELxSkillFlowEvent::Cancel) Group->CancelSkillUnits();
				else Group->StopSkillUnits();
			}
		}
	}
	for (const ULxSkillFlowNode* Node : Nodes)
		if (Node->Kind == ELxSkillFlowNodeKind::Event && Node->Event == Event)
			Enqueue(Node->Next, FLxSkillUnitResult());
}

void ALxSkillFlowExecution::UpdateAim(const FTransform& Transform)
{
	if (!bControlOpen || !Worker) return;
	Worker->CurrentCastContext.SpawnTransform = Transform;
	for (ULxSkillUnitGroup* Group : GetActiveGroups())
	{
		const FGuid* Id = Groups.Find(Group);
		const ULxSkillFlowNode* Node = Id ? FindNode(*Id) : nullptr;
		// 依附和光环由单元自己跟随目标或角色锚点，瞄准更新不能把它们移离挂接对象。
		if (Node && Node->Lifetime == ELxSkillFlowLifetime::Maintained
			&& Node->Kind != ELxSkillFlowNodeKind::ContinuousAttach && Node->Kind != ELxSkillFlowNodeKind::PeriodicAttach
			&& Node->Kind != ELxSkillFlowNodeKind::ElementAbnormalAttach
			&& Node->Kind != ELxSkillFlowNodeKind::ContinuousAura && Node->Kind != ELxSkillFlowNodeKind::PeriodicAura)
			Group->UpdateSkillUnitsTransform(Transform);
	}
}

void ALxSkillFlowExecution::Execute(const FLxSkillFlowTask& Task)
{
	const ULxSkillFlowNode* Node = FindNode(Task.NodeId);
	if (!Node || (Node->Lifetime == ELxSkillFlowLifetime::Maintained && !bControlOpen)) return;
	ULxSkillUnitGroup* Group = nullptr;
	switch (Node->Kind)
	{
	case ELxSkillFlowNodeKind::Projectile:
		Group = Worker->CreateStraightProjectileUnits(Task.Result, Node->ProjectileClass, Node->Projectile,
			Node->TargetFilter, Node->HitLimit, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Area:
		Group = Worker->CreateDirectHitAreaEffects(Task.Result, Node->AreaClass, Node->Area, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Ray:
		Group = Worker->CreateContinuousRayEffectUnit(Task.Result, Node->RayClass, Node->Ray, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::GroundBounce:
		Group = Worker->CreateGroundBounceProjectileUnits(Task.Result, Node->GroundBounceClass, Node->GroundBounce, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Lob:
		Group = Worker->CreateLobProjectileUnits(Task.Result, Node->LobClass, Node->Lob, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::DurationArea:
		Group = Worker->CreateDurationAreaEffects(Task.Result, Node->DurationAreaClass, Node->DurationArea, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::ScalingArea:
		Group = Worker->CreateScalingAreaEffects(Task.Result, Node->ScalingAreaClass, Node->ScalingArea, Node->TargetFilter, Node->HitLimit, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Melee:
		Group = Worker->CreateMeleeEffect(Node->MeleeClass, Node->Melee, false);
		break;
	case ELxSkillFlowNodeKind::SingleRay:
		Group = Worker->CreateSingleRayEffectUnits(Task.Result, Node->SingleRayClass, Node->SingleRay, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::ContinuousAttach:
		Group = Worker->CreateContinuousAttachEffects(Task.Result, Node->ContinuousAttachClass, Node->ContinuousAttach, false);
		break;
	case ELxSkillFlowNodeKind::ElementAbnormalAttach:
		Group = Worker->CreateElementAbnormalAttachEffects(Task.Result, Node->ElementAbnormalAttachClass, Node->ElementAbnormalAttach, false);
		break;
	case ELxSkillFlowNodeKind::PeriodicAttach:
		Group = Worker->CreatePeriodicAttachEffects(Task.Result, Node->PeriodicAttachClass, Node->PeriodicAttach, false);
		break;
	case ELxSkillFlowNodeKind::ContinuousAura:
		Group = Worker->CreateContinuousAuraEffectUnit(Node->ContinuousAuraClass, Node->ContinuousAura, Node->AuraRange, false);
		break;
	case ELxSkillFlowNodeKind::PeriodicAura:
		Group = Worker->CreatePeriodicAuraEffectUnit(Node->PeriodicAuraClass, Node->PeriodicAura, Node->AuraRange, false);
		break;
	case ELxSkillFlowNodeKind::SpawnEntity:
		Group = Worker->CreateSpawnEntityUnits(Task.Result, Node->SpawnEntityClass, Node->SpawnEntity, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Barrier:
		Group = Worker->CreateSpawnEntityUnits(Task.Result, Node->BarrierClass ? Node->BarrierClass.Get() : ALxBarrierSkillUnitActor::StaticClass(), Node->Barrier, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Marker:
		Group = Worker->CreateSpawnEntityUnits(Task.Result, Node->MarkerClass ? Node->MarkerClass.Get() : ALxMarkerSkillUnitActor::StaticClass(), Node->Marker, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::SummonCreature:
		Group = Worker->CreateSpawnEntityUnits(Task.Result, Node->SummonCreatureClass ? Node->SummonCreatureClass.Get() : ALxSummonCreatureSkillUnitActor::StaticClass(), Node->SummonCreature, Node->SpawnLocation, false);
		break;
	case ELxSkillFlowNodeKind::Trigger:
		Group = Worker->CreateTriggerUnits(Task.Result, Node->TriggerClass, Node->Trigger, Node->SpawnLocation, false);
		break;
	default: return;
	}
	if (!Group) return;
	if (Node->bOverrideTargetRules)
	{
		for (ALxSkillUnitActor* Unit : Group->GetSkillUnits())
			if (IsValid(Unit)) Unit->SetTargetRules(Node->TargetFilter, Node->HitLimit);
	}
	// 本次流程负责结束后的缓存清理，避免原缓存监听先清空结果与其他事件。
	Group->OnSkillUnitGroupFinished.RemoveAll(Worker);
	Groups.Add(Group, Node->Id);
	Group->OnSkillUnitGroupHit.AddDynamic(this, &ALxSkillFlowExecution::OnHit);
	Group->OnSkillUnitGroupFinished.AddUObject(this, &ALxSkillFlowExecution::OnFinished);
	Group->ActivateSkillUnits();
}

void ALxSkillFlowExecution::OnHit(ULxSkillUnitGroup* Group, const FLxSkillUnitResult& Result)
{
	const FGuid* Id = Groups.Find(Group);
	const ULxSkillFlowNode* Node = Id ? FindNode(*Id) : nullptr;
	if (!Node || bShuttingDown) return;
	if (Node->EntryPackageIndex != INDEX_NONE) Worker->ApplySkillEntryPackageByIndex(Result, Node->EntryPackageIndex);
	Enqueue(Node->Hit, Result);
}

void ALxSkillFlowExecution::OnFinished(ULxSkillUnitGroup* Group, const FLxSkillUnitResult& Result)
{
	const FGuid* Id = Groups.Find(Group);
	const ULxSkillFlowNode* Node = Id ? FindNode(*Id) : nullptr;
	if (Node) Enqueue(Node->Finished, Result);
	Groups.Remove(Group);
	Group->OnSkillUnitGroupHit.RemoveAll(this);
	Group->OnSkillUnitGroupFinished.RemoveAll(this);
	Worker->ReleaseSkillUnitGroup(Group);
}

TArray<ULxSkillUnitGroup*> ALxSkillFlowExecution::GetActiveGroups() const
{
	TArray<ULxSkillUnitGroup*> Result;
	for (const auto& Pair : Groups) if (IsValid(Pair.Key)) Result.Add(Pair.Key);
	return Result;
}

void ALxSkillFlowExecution::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Worker) { Destroy(); return; }
	if (bControlOpen && !IsValid(Worker->GetSkillCasterActor())) SendEvent(ELxSkillFlowEvent::Cancel);
	// 限制每帧工作量；同步命中的后续节点也经过队列，不递归调用。
	for (int32 Count = 0; Count < 128 && !Pending.IsEmpty(); ++Count)
	{
		const FLxSkillFlowTask Task = Pending[0];
		Pending.RemoveAt(0);
		Execute(Task);
	}
	if (!bControlOpen && Pending.IsEmpty() && Groups.IsEmpty()) Destroy();
}

void ALxSkillFlowExecution::EndPlay(const EEndPlayReason::Type Reason)
{
	bShuttingDown = true;
	for (ULxSkillUnitGroup* Group : GetActiveGroups())
	{
		Group->OnSkillUnitGroupHit.RemoveAll(this);
		Group->OnSkillUnitGroupFinished.RemoveAll(this);
	}
	Pending.Reset();
	Groups.Reset();
	Super::EndPlay(Reason);
}
