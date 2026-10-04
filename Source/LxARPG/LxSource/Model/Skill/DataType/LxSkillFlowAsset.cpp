#include "LxSkillFlowAsset.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxElementAbnormalAttachSkillUnitActor.h"
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

ULxSkillFlowNode::ULxSkillFlowNode()
{
	for (FLxSkillProjectileSpec* Spec : {&Projectile.ProjectileSpec, &GroundBounce.ProjectileSpec, &Lob.ProjectileSpec})
	{
		Spec->FlightSpeed = 10.f;
		Spec->MaxFlightDistance = 30.f;
	}
	ContinuousAttach.AttachEffectSpec.Duration = 5.f;
	ElementAbnormalAttach.AttachEffectSpec.Duration = 5.f;
	DurationArea.AreaEffectSpec.Duration = 5.f;
	ScalingArea.AreaEffectSpec.Duration = 5.f;
	PeriodicAttach.AttachEffectSpec.Duration = 5.f;
	ContinuousAura.AuraEffectSpec.Duration = 5.f;
	PeriodicAura.AuraEffectSpec.Duration = 5.f;
	SpawnEntity.LifeSpec.Duration = 5.f;
	Barrier.LifeSpec.Duration = 5.f;
	Marker.LifeSpec.Duration = 5.f;
	SummonCreature.LifeSpec.Duration = 5.f;
	Trigger.LifeSpec.Duration = 5.f;
}

FText ULxSkillFlowNode::GetMenuCategory(ELxSkillFlowNodeKind InKind)
{
	switch (InKind)
	{
	case ELxSkillFlowNodeKind::Projectile:
	case ELxSkillFlowNodeKind::GroundBounce:
	case ELxSkillFlowNodeKind::Lob:
		return NSLOCTEXT("技能流程分类", "投射物", "技能单元|投射物");
	case ELxSkillFlowNodeKind::Area:
	case ELxSkillFlowNodeKind::DurationArea:
	case ELxSkillFlowNodeKind::ScalingArea:
		return NSLOCTEXT("技能流程分类", "范围效果", "技能单元|范围效果");
	case ELxSkillFlowNodeKind::Ray:
	case ELxSkillFlowNodeKind::SingleRay:
		return NSLOCTEXT("技能流程分类", "射线", "技能单元|射线");
	case ELxSkillFlowNodeKind::Melee:
		return NSLOCTEXT("技能流程分类", "近战", "技能单元|近战");
	case ELxSkillFlowNodeKind::ContinuousAttach:
	case ELxSkillFlowNodeKind::PeriodicAttach:
	case ELxSkillFlowNodeKind::ElementAbnormalAttach:
		return NSLOCTEXT("技能流程分类", "依附效果", "技能单元|依附效果");
	case ELxSkillFlowNodeKind::ContinuousAura:
	case ELxSkillFlowNodeKind::PeriodicAura:
		return NSLOCTEXT("技能流程分类", "光环效果", "技能单元|光环效果");
	case ELxSkillFlowNodeKind::SpawnEntity:
	case ELxSkillFlowNodeKind::Barrier:
	case ELxSkillFlowNodeKind::Marker:
	case ELxSkillFlowNodeKind::SummonCreature:
		return NSLOCTEXT("技能流程分类", "生成实体", "技能单元|生成实体");
	case ELxSkillFlowNodeKind::Trigger:
		return NSLOCTEXT("技能流程分类", "触发器", "技能单元|触发器");
	default: return NSLOCTEXT("技能流程分类", "事件", "角色事件");
	}
}

FText ULxSkillFlowNode::GetSkillUnitDisplayName() const
{
	return ALxSkillUnitActor::GetSkillUnitClassDisplayName(GetSkillUnitClass());
}

TSubclassOf<ALxSkillUnitActor> ULxSkillFlowNode::GetSkillUnitClass() const
{
	switch (Kind)
	{
	case ELxSkillFlowNodeKind::Projectile:
		return ProjectileClass;
	case ELxSkillFlowNodeKind::Area:
		return AreaClass;
	case ELxSkillFlowNodeKind::Ray:
		return RayClass;
	case ELxSkillFlowNodeKind::GroundBounce:
		return GroundBounceClass;
	case ELxSkillFlowNodeKind::Lob:
		return LobClass;
	case ELxSkillFlowNodeKind::DurationArea:
		return DurationAreaClass;
	case ELxSkillFlowNodeKind::ScalingArea:
		return ScalingAreaClass;
	case ELxSkillFlowNodeKind::Melee:
		return MeleeClass;
	case ELxSkillFlowNodeKind::SingleRay:
		return SingleRayClass;
	case ELxSkillFlowNodeKind::ContinuousAttach:
		return ContinuousAttachClass;
	case ELxSkillFlowNodeKind::PeriodicAttach:
		return PeriodicAttachClass;
	case ELxSkillFlowNodeKind::ElementAbnormalAttach:
		return ElementAbnormalAttachClass;
	case ELxSkillFlowNodeKind::ContinuousAura:
		return ContinuousAuraClass;
	case ELxSkillFlowNodeKind::PeriodicAura:
		return PeriodicAuraClass;
	case ELxSkillFlowNodeKind::SpawnEntity:
		return SpawnEntityClass;
	case ELxSkillFlowNodeKind::Barrier:
		return BarrierClass;
	case ELxSkillFlowNodeKind::Marker:
		return MarkerClass;
	case ELxSkillFlowNodeKind::SummonCreature:
		return SummonCreatureClass;
	case ELxSkillFlowNodeKind::Trigger:
		return TriggerClass;
	default:
		return nullptr;
	}
}

bool ULxSkillFlowAsset::Validate(FText& Error) const
{
	// 校验同时供编辑器和运行入口使用，避免非图形方式修改资产绕过规则。
	auto Fail = [&Error](const TCHAR* Message) { Error = FText::FromString(Message); return false; };
	if (ReleaseType == ELxSkillReleaseType::None) return Fail(TEXT("请选择释放方式。"));
	TMap<FGuid, const ULxSkillFlowNode*> Lookup;
	bool bHasEntry = false;
	for (const ULxSkillFlowNode* Node : Nodes)
	{
		if (!Node || !Node->Id.IsValid() || Lookup.Contains(Node->Id)) return Fail(TEXT("节点标识缺失或重复。"));
		if (!StaticEnum<ELxSkillFlowNodeKind>()->IsValidEnumValue(static_cast<int64>(Node->Kind))
			|| Node->Kind > ELxSkillFlowNodeKind::ElementAbnormalAttach) return Fail(TEXT("节点类型无效。"));
		Lookup.Add(Node->Id, Node);
		if (Node->Kind == ELxSkillFlowNodeKind::Event)
		{
			bHasEntry |= !Node->Next.IsEmpty() &&
				((ReleaseType == ELxSkillReleaseType::DirectRelease && Node->Event == ELxSkillFlowEvent::Direct)
				|| (ReleaseType == ELxSkillReleaseType::SustainedRelease && Node->Event == ELxSkillFlowEvent::SustainStart)
				|| (ReleaseType == ELxSkillReleaseType::ChargeRelease && Node->Event == ELxSkillFlowEvent::ChargeEnd));
		}
		else
		{
			if (Node->EntryPackageIndex != INDEX_NONE && !EntryPackages.IsValidIndex(Node->EntryPackageIndex)) return Fail(TEXT("命中词条包下标不存在。"));
			if ((Node->Kind == ELxSkillFlowNodeKind::Ray || Node->Kind == ELxSkillFlowNodeKind::Melee)
				&& Node->Lifetime != ELxSkillFlowLifetime::Maintained) return Fail(TEXT("持续射线和近战效果必须随本次释放维持。"));
			if (ReleaseType == ELxSkillReleaseType::DirectRelease && Node->Lifetime == ELxSkillFlowLifetime::Maintained) return Fail(TEXT("直接释放流程中的单元应设为自主运行。"));
			if (Node->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach
				&& (!Node->ElementAbnormalAttach.AbnormalSpec.IsValid() || !FMath::IsFinite(Node->ElementAbnormalAttach.AttachEffectSpec.Duration)))
				return Fail(TEXT("元素异常需要有效的状态或 Buff，触发几率为 0 至 100，持续时间必须为有限数值。"));
			const FLxSkillProjectileSpec* ProjectileSpec = Node->Kind == ELxSkillFlowNodeKind::Projectile ? &Node->Projectile.ProjectileSpec
				: Node->Kind == ELxSkillFlowNodeKind::GroundBounce ? &Node->GroundBounce.ProjectileSpec
				: Node->Kind == ELxSkillFlowNodeKind::Lob ? &Node->Lob.ProjectileSpec : nullptr;
			if (ProjectileSpec && (!FMath::IsFinite(ProjectileSpec->FlightSpeed) || !FMath::IsFinite(ProjectileSpec->MaxFlightDistance)
				|| ProjectileSpec->FlightSpeed <= 0 || ProjectileSpec->MaxFlightDistance <= 0))
				return Fail(TEXT("投射物速度和最大飞行距离必须是大于零的有限数值。"));
			const FLxSkillAuraEffectSpec* AuraSpec = Node->Kind == ELxSkillFlowNodeKind::ContinuousAura ? &Node->ContinuousAura.AuraEffectSpec
				: Node->Kind == ELxSkillFlowNodeKind::PeriodicAura ? &Node->PeriodicAura.AuraEffectSpec : nullptr;
			if (AuraSpec && (!FMath::IsFinite(Node->AuraRange) || Node->AuraRange <= 0
				|| !FMath::IsFinite(AuraSpec->Duration) || (AuraSpec->Duration <= 0 && AuraSpec->Duration != -1.f)))
				return Fail(TEXT("光环范围必须大于零，持续时间必须大于零或为 -1。"));
			const float Period = Node->Kind == ELxSkillFlowNodeKind::PeriodicAttach ? Node->PeriodicAttach.PeriodicSpec.TriggerInterval
				: Node->Kind == ELxSkillFlowNodeKind::PeriodicAura ? Node->PeriodicAura.PeriodicSpec.TriggerInterval
				: Node->Kind == ELxSkillFlowNodeKind::DurationArea ? Node->DurationArea.DurationAreaEffectSpec.DetectionPeriod
				: Node->Kind == ELxSkillFlowNodeKind::Ray ? Node->Ray.ContinuousRaySpec.TriggerInterval : 1.f;
			if (!FMath::IsFinite(Period) || Period <= 0) return Fail(TEXT("周期触发或检测间隔必须大于零。"));
		}
	}
	if (!bHasEntry) return Fail(TEXT("请连接与释放方式对应的事件入口。"));
	TMap<FGuid, int32> Indegrees;
	for (const auto& Pair : Lookup) Indegrees.Add(Pair.Key, 0);
	for (const auto& Pair : Lookup)
	{
		for (const TArray<FGuid>* Links : {&Pair.Value->Next, &Pair.Value->Hit, &Pair.Value->Finished})
			for (const FGuid& Target : *Links)
			{
				if (!Lookup.Contains(Target) || Lookup[Target]->Kind == ELxSkillFlowNodeKind::Event) return Fail(TEXT("存在无效连线或连线指向事件入口。"));
				if (Lookup[Target]->Kind == ELxSkillFlowNodeKind::ElementAbnormalAttach && Links != &Pair.Value->Hit)
					return Fail(TEXT("元素异常依附必须连接前置技能单元的命中出口。"));
				++Indegrees[Target];
			}
	}
	TArray<FGuid> Pending;
	for (const auto& Pair : Indegrees) if (Pair.Value == 0) Pending.Add(Pair.Key);
	int32 Count = 0;
	while (!Pending.IsEmpty())
	{
		const ULxSkillFlowNode* Node = Lookup[Pending.Pop()];
		++Count;
		for (const TArray<FGuid>* Links : {&Node->Next, &Node->Hit, &Node->Finished})
			for (const FGuid& Target : *Links) if (--Indegrees[Target] == 0) Pending.Add(Target);
	}
	if (Count != Lookup.Num()) return Fail(TEXT("最小版本不支持循环连线。"));
	// 已经结束控制的入口不能再创建维持单元，避免配置有效却静默跳过节点。
	for (const ULxSkillFlowNode* Entry : Nodes)
	{
		if (Entry->Kind != ELxSkillFlowNodeKind::Event || Entry->Event == ELxSkillFlowEvent::ChargeStart
			|| Entry->Event == ELxSkillFlowEvent::SustainStart) continue;
		TArray<FGuid> Remaining = Entry->Next;
		TSet<FGuid> Visited;
		while (!Remaining.IsEmpty())
		{
			const FGuid Id = Remaining.Pop();
			if (Visited.Contains(Id)) continue;
			Visited.Add(Id);
			const ULxSkillFlowNode* Node = Lookup[Id];
			if (Node->Lifetime == ELxSkillFlowLifetime::Maintained) return Fail(TEXT("直接释放、结束蓄力、结束释放或取消分支只能创建自主单元。"));
			Remaining.Append(Node->Next); Remaining.Append(Node->Hit); Remaining.Append(Node->Finished);
		}
	}
	Error = FText::GetEmpty();
	return true;
}
