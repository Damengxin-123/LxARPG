#include "LxAIBehaviorTreeAsset.h"

bool FLxAIHealthRange::IsValid() const
{
	return FMath::IsFinite(Min) && FMath::IsFinite(Max) && Min >= 0.0f && Max <= 1.0f && Min <= Max;
}

bool FLxAIHealthRange::Contains(float HealthRatio) const
{
	return IsValid() && FMath::IsFinite(HealthRatio) && HealthRatio >= Min && HealthRatio <= Max;
}

bool FLxAIHealthRange::ContainsRange(const FLxAIHealthRange& Other) const
{
	return IsValid() && Other.IsValid() && Other.Min >= Min && Other.Max <= Max;
}

ELxAIBehaviorState ULxAIBehaviorTreeNodeData::GetState() const
{
	return Kind == ELxAIBehaviorNodeKind::Action ? ULxAIBehaviorTreeAsset::GetActionState(Action) : State;
}

ELxAIBehaviorState ULxAIBehaviorTreeAsset::GetActionState(ELxAIBehaviorAction Action)
{
	switch (Action)
	{
	case ELxAIBehaviorAction::Wait: return ELxAIBehaviorState::Idle;
	case ELxAIBehaviorAction::PointPatrol:
	case ELxAIBehaviorAction::RoutePatrol: return ELxAIBehaviorState::Patrol;
	case ELxAIBehaviorAction::Alert: return ELxAIBehaviorState::Alert;
	case ELxAIBehaviorAction::RandomFlee:
	case ELxAIBehaviorAction::PointFlee:
	case ELxAIBehaviorAction::RouteFlee: return ELxAIBehaviorState::Flee;
	default: return ELxAIBehaviorState::Combat;
	}
}

FText ULxAIBehaviorTreeNodeData::GetDisplayLabel() const
{
	if (Kind == ELxAIBehaviorNodeKind::Entry) return ULxAIBehaviorTreeAsset::GetEntryLabel(Entry);
	FText Type = Kind == ELxAIBehaviorNodeKind::Action ? ULxAIBehaviorTreeAsset::GetActionLabel(Action)
		: FText::Format(NSLOCTEXT("AI行为树", "容器标题", "{0}{1}"), ULxAIBehaviorTreeAsset::GetStateLabel(State),
			Kind == ELxAIBehaviorNodeKind::State ? NSLOCTEXT("AI行为树", "状态", "状态") : NSLOCTEXT("AI行为树", "阶段", "阶段"));
	return Label.IsEmpty() ? Type : FText::Format(NSLOCTEXT("AI行为树", "命名节点", "{0} · {1}"), Type, Label);
}

bool ULxAIBehaviorTreeNodeData::ValidateConfiguration(FText& OutError) const
{
	OutError = FText();
	FString Error;
	if (Kind > ELxAIBehaviorNodeKind::Entry || (Kind != ELxAIBehaviorNodeKind::Entry && GetState() > ELxAIBehaviorState::Flee)
		|| (Kind == ELxAIBehaviorNodeKind::Action && Action > ELxAIBehaviorAction::RouteFlee))
		Error = TEXT("节点类型无效");
	else if (Order < 0) Error = TEXT("排列序号不能小于零");
	else if (Kind == ELxAIBehaviorNodeKind::Entry)
	{
		if (Entry > ELxAIBehaviorEntry::Attacked || EntryPriority < 0) Error = TEXT("入口类型或优先级无效");
	}
	else if (Kind != ELxAIBehaviorNodeKind::Action)
	{
		if (!HealthRange.IsValid()) Error = TEXT("生命值区间必须满足 0 ≤ 下限 ≤ 上限 ≤ 1");
	}
	else
	{
		switch (Action)
		{
		case ELxAIBehaviorAction::PointPatrol:
		case ELxAIBehaviorAction::PointFlee:
			if (!PointId.IsValid()) Error = TEXT("请选择场景点位ID；范围由点位提供");
			break;
		case ELxAIBehaviorAction::RoutePatrol:
		case ELxAIBehaviorAction::RouteFlee:
			if (!RouteId.IsValid()) Error = TEXT("请选择场景路线ID");
			else if (Action == ELxAIBehaviorAction::RoutePatrol && RouteMode != ELxAIRoutePatrolMode::PingPong && RouteMode != ELxAIRoutePatrolMode::Loop) Error = TEXT("巡逻方式无效");
			break;
		case ELxAIBehaviorAction::MeleeSkill:
		case ELxAIBehaviorAction::RangedSkill:
		case ELxAIBehaviorAction::BuffSkill:
			if (!SkillItemId.IsValid()) Error = TEXT("请选择技能物品ID");
			break;
		default: break;
		}
		if (Error.IsEmpty() && (Action == ELxAIBehaviorAction::Wait || Action == ELxAIBehaviorAction::PointPatrol || Action == ELxAIBehaviorAction::RoutePatrol)
			&& (!FMath::IsFinite(WaitSeconds) || WaitSeconds < 0.0f)) Error = TEXT("等待时间必须是非负有限值");
		if (Error.IsEmpty() && (Action == ELxAIBehaviorAction::Alert || Action == ELxAIBehaviorAction::Defend || Action == ELxAIBehaviorAction::RangedSkill)
			&& (!FMath::IsFinite(MinDistanceMeters) || !FMath::IsFinite(MaxDistanceMeters) || MinDistanceMeters < 0.0f || MaxDistanceMeters < MinDistanceMeters))
			Error = TEXT("敌人距离必须满足 0 ≤ 最小距离 ≤ 最大距离");
		if (Error.IsEmpty() && Action == ELxAIBehaviorAction::MeleeSkill && (!FMath::IsFinite(MeleeDistanceMeters) || MeleeDistanceMeters <= 0.0f))
			Error = TEXT("近战释放距离必须是正有限值");
		if (Error.IsEmpty() && Action == ELxAIBehaviorAction::RandomFlee && (!FMath::IsFinite(FleeStepMeters) || FleeStepMeters <= 0.0f))
			Error = TEXT("单次逃跑距离必须是正有限值");
	}
	if (Error.IsEmpty()) return true;
	OutError = FText::Format(NSLOCTEXT("AI行为树", "节点错误", "{0}：{1}"), GetDisplayLabel(), FText::FromString(Error));
	return false;
}

ULxAIBehaviorTreeNodeData* ULxAIBehaviorTreeAsset::FindNode(const FGuid& Id) const
{
	for (ULxAIBehaviorTreeNodeData* Node : Nodes)
		if (Node && Node->NodeId == Id) return Node;
	return nullptr;
}

bool ULxAIBehaviorTreeAsset::CanAttach(const ULxAIBehaviorTreeNodeData& Parent, const ULxAIBehaviorTreeNodeData& Child)
{
	if (Parent.Kind == ELxAIBehaviorNodeKind::Entry) return Child.Kind == ELxAIBehaviorNodeKind::State;
	return Parent.GetState() == Child.GetState()
		&& ((Parent.Kind == ELxAIBehaviorNodeKind::State && Child.Kind == ELxAIBehaviorNodeKind::Phase)
			|| (Parent.Kind == ELxAIBehaviorNodeKind::Phase && Child.Kind == ELxAIBehaviorNodeKind::Action));
}

bool ULxAIBehaviorTreeAsset::ValidateConfiguration(FText& OutError) const
{
	return Perception.ValidateConfiguration(OutError) && Analysis.ValidateConfiguration(OutError) && ValidateTree(OutError);
}

bool ULxAIBehaviorTreeAsset::ValidateTree(FText& OutError) const
{
	OutError = FText();
	const auto Fail = [&OutError](const FString& Message)
	{
		OutError = FText::FromString(Message);
		return false;
	};
	if (Roots.Num() != 4) return Fail(TEXT("必须保留平静、发现敌人、敌人靠近、受到攻击四个固定入口"));
	TSet<FGuid> Ids;
	for (const ULxAIBehaviorTreeNodeData* Node : Nodes)
	{
		if (!Node || !Node->NodeId.IsValid() || Ids.Contains(Node->NodeId)) return Fail(TEXT("存在空节点、无效或重复的节点标识"));
		Ids.Add(Node->NodeId);
	}
	TSet<FGuid> Attached;
	TSet<ELxAIBehaviorEntry> EntryTypes;
	for (const FGuid& Id : Roots)
	{
		const ULxAIBehaviorTreeNodeData* Root = FindNode(Id);
		if (!Root || Root->Kind != ELxAIBehaviorNodeKind::Entry || Root->Entry > ELxAIBehaviorEntry::Attacked
			|| Attached.Contains(Id) || EntryTypes.Contains(Root->Entry)) return Fail(TEXT("入口标识或入口类型无效、重复"));
		Attached.Add(Id);
		EntryTypes.Add(Root->Entry);
	}
	for (const ULxAIBehaviorTreeNodeData* Parent : Nodes)
	{
		if ((Parent->Kind == ELxAIBehaviorNodeKind::State || Parent->Kind == ELxAIBehaviorNodeKind::Phase) && Parent->Children.IsEmpty())
			return Fail(Parent->GetDisplayLabel().ToString() + TEXT("：请连接至少一个子节点"));
		if (Parent->Kind == ELxAIBehaviorNodeKind::Action && !Parent->Children.IsEmpty()) return Fail(TEXT("行为节点不能包含子节点"));
		TSet<FGuid> UniqueChildren;
		for (const FGuid& Id : Parent->Children)
		{
			const ULxAIBehaviorTreeNodeData* Child = FindNode(Id);
			if (!Child || !CanAttach(*Parent, *Child)) return Fail(TEXT("只能连接 入口 → 状态 → 同类阶段 → 同类行为"));
			if (UniqueChildren.Contains(Id)) return Fail(TEXT("同一父节点不能重复连接同一子节点"));
			UniqueChildren.Add(Id);
			if (Child->Kind == ELxAIBehaviorNodeKind::Phase && Attached.Contains(Id)) return Fail(TEXT("阶段只能属于一个状态；状态和行为允许共享"));
			Attached.Add(Id);
			if (Parent->Kind == ELxAIBehaviorNodeKind::State && !Parent->HealthRange.ContainsRange(Child->HealthRange))
				return Fail(Child->GetDisplayLabel().ToString() + TEXT("：阶段生命值区间必须包含于父状态区间内"));
		}
	}
	// 严格分层排除环；入口唯一、其他节点至少有父项，故每个节点都可从入口到达。
	if (Attached.Num() != Nodes.Num()) return Fail(TEXT("存在未连接到固定入口的节点"));
	for (const ULxAIBehaviorTreeNodeData* Node : Nodes)
		if (!Node->ValidateConfiguration(OutError)) return false;
	return true;
}

bool ULxAIBehaviorTreeAsset::IsPhaseHealthEligible(FGuid PhaseId, float HealthRatio) const
{
	const ULxAIBehaviorTreeNodeData* Phase = FindNode(PhaseId);
	if (!Phase || Phase->Kind != ELxAIBehaviorNodeKind::Phase || !Phase->HealthRange.Contains(HealthRatio)) return false;
	for (const FGuid& EntryId : Roots)
	{
		const ULxAIBehaviorTreeNodeData* Entry = FindNode(EntryId);
		if (!Entry || Entry->Kind != ELxAIBehaviorNodeKind::Entry) continue;
		for (const FGuid& StateId : Entry->Children)
		{
			const ULxAIBehaviorTreeNodeData* State = FindNode(StateId);
			if (State && State->Kind == ELxAIBehaviorNodeKind::State && CanAttach(*State, *Phase) && State->Children.Contains(PhaseId))
				return State->HealthRange.ContainsRange(Phase->HealthRange) && State->HealthRange.Contains(HealthRatio);
		}
	}
	return false;
}

void ULxAIBehaviorTreeAsset::PostLoad()
{
	Super::PostLoad();
	UpgradeLegacyEntries();
}

bool ULxAIBehaviorTreeAsset::UpgradeLegacyEntries()
{
	if (Roots.IsEmpty()) return false;
	for (const ULxAIBehaviorTreeNodeData* Node : Nodes)
		if (Node && Node->Kind == ELxAIBehaviorNodeKind::Entry) return false;
	for (const FGuid& Id : Roots)
	{
		const ULxAIBehaviorTreeNodeData* Root = FindNode(Id);
		if (!Root || Root->Kind != ELxAIBehaviorNodeKind::State) return false;
	}
	const TArray<FGuid> OldRoots = Roots;
	Roots.Reset();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		ULxAIBehaviorTreeNodeData* EntryNode = NewObject<ULxAIBehaviorTreeNodeData>(this, NAME_None, RF_Transactional);
		EntryNode->Kind = ELxAIBehaviorNodeKind::Entry;
		EntryNode->Entry = static_cast<ELxAIBehaviorEntry>(Index);
		EntryNode->EntryPriority = GetDefaultEntryPriority(EntryNode->Entry);
		EntryNode->NodeId = FGuid::NewGuid();
		for (const FGuid& Id : OldRoots)
		{
			const ULxAIBehaviorTreeNodeData* State = FindNode(Id);
			if (GetDefaultEntriesForState(State->State).Contains(EntryNode->Entry)) EntryNode->Children.AddUnique(Id);
		}
		Nodes.Add(EntryNode);
		Roots.Add(EntryNode->NodeId);
	}
	return true;
}

FText ULxAIBehaviorTreeAsset::GetEntryLabel(ELxAIBehaviorEntry Entry)
{
	return StaticEnum<ELxAIBehaviorEntry>()->GetDisplayNameTextByValue(static_cast<int64>(Entry));
}

FText ULxAIBehaviorTreeAsset::GetEntryDescription(ELxAIBehaviorEntry Entry)
{
	switch (Entry)
	{
	case ELxAIBehaviorEntry::Calm: return FText::FromString(TEXT("没有已知敌人，且不在受击警觉期；持续条件。"));
	case ELxAIBehaviorEntry::EnemyFound: return FText::FromString(TEXT("存在已确认的敌人，包括仍在感知记忆中的敌人；持续条件。"));
	case ELxAIBehaviorEntry::EnemyNear: return FText::FromString(TEXT("已知敌人进入靠近距离，超过退出距离才解除；使用最后已知位置，不透视丢失目标。"));
	case ELxAIBehaviorEntry::Attacked: return FText::FromString(TEXT("有效受击触发一次响应；响应完成前的连续受击只刷新警觉时间，不重启或排队。"));
	default: return FText();
	}
}

int32 ULxAIBehaviorTreeAsset::GetDefaultEntryPriority(ELxAIBehaviorEntry Entry)
{
	return static_cast<int32>(Entry) * 100;
}

TArray<ELxAIBehaviorEntry> ULxAIBehaviorTreeAsset::GetDefaultEntriesForState(ELxAIBehaviorState State)
{
	switch (State)
	{
	case ELxAIBehaviorState::Idle:
	case ELxAIBehaviorState::Patrol: return {ELxAIBehaviorEntry::Calm};
	case ELxAIBehaviorState::Alert: return {ELxAIBehaviorEntry::EnemyFound};
	case ELxAIBehaviorState::Combat: return {ELxAIBehaviorEntry::EnemyNear, ELxAIBehaviorEntry::Attacked};
	case ELxAIBehaviorState::Flee: return {ELxAIBehaviorEntry::Attacked};
	default: return {};
	}
}

TArray<const ULxAIBehaviorTreeNodeData*> ULxAIBehaviorTreeAsset::GetOrderedEntries() const
{
	TArray<const ULxAIBehaviorTreeNodeData*> Entries;
	for (const FGuid& Id : Roots)
		if (const ULxAIBehaviorTreeNodeData* Node = FindNode(Id); Node && Node->Kind == ELxAIBehaviorNodeKind::Entry) Entries.Add(Node);
	Entries.Sort([](const ULxAIBehaviorTreeNodeData& A, const ULxAIBehaviorTreeNodeData& B)
	{
		return A.EntryPriority != B.EntryPriority ? A.EntryPriority > B.EntryPriority : A.Entry < B.Entry;
	});
	return Entries;
}

bool ULxAIBehaviorTreeAsset::FindEligibleBranch(const ULxAIBehaviorTreeNodeData& EntryNode, float HealthRatio, FGuid& OutState, FGuid& OutPhase) const
{
	OutState.Invalidate();
	OutPhase.Invalidate();
	if (EntryNode.Kind != ELxAIBehaviorNodeKind::Entry || !Roots.Contains(EntryNode.NodeId)
		|| !FMath::IsFinite(HealthRatio) || HealthRatio < 0.0f || HealthRatio > 1.0f) return false;
	const auto OrderedChildren = [this](const ULxAIBehaviorTreeNodeData& Parent)
	{
		TArray<const ULxAIBehaviorTreeNodeData*> Result;
		for (const FGuid& Id : Parent.Children)
			if (const ULxAIBehaviorTreeNodeData* Child = FindNode(Id); Child && CanAttach(Parent, *Child)) Result.Add(Child);
		Result.Sort([](const ULxAIBehaviorTreeNodeData& A, const ULxAIBehaviorTreeNodeData& B)
		{
			return A.Order != B.Order ? A.Order < B.Order : A.NodeId < B.NodeId;
		});
		return Result;
	};
	for (const ULxAIBehaviorTreeNodeData* State : OrderedChildren(EntryNode))
	{
		if (!State->HealthRange.Contains(HealthRatio)) continue;
		for (const ULxAIBehaviorTreeNodeData* Phase : OrderedChildren(*State))
		{
			if (!State->HealthRange.ContainsRange(Phase->HealthRange) || !Phase->HealthRange.Contains(HealthRatio)
				|| OrderedChildren(*Phase).IsEmpty()) continue;
			OutState = State->NodeId;
			OutPhase = Phase->NodeId;
			return true;
		}
	}
	return false;
}

FText ULxAIBehaviorTreeAsset::GetStateLabel(ELxAIBehaviorState State)
{
	return StaticEnum<ELxAIBehaviorState>()->GetDisplayNameTextByValue(static_cast<int64>(State));
}

FText ULxAIBehaviorTreeAsset::GetActionLabel(ELxAIBehaviorAction Action)
{
	return StaticEnum<ELxAIBehaviorAction>()->GetDisplayNameTextByValue(static_cast<int64>(Action));
}

FText ULxAIBehaviorTreeAsset::GetActionDescription(ELxAIBehaviorAction Action)
{
	switch (Action)
	{
	case ELxAIBehaviorAction::Wait: return NSLOCTEXT("AI行为树", "待机说明", "在原地待机；等待时间为零时持续待机。");
	case ELxAIBehaviorAction::PointPatrol: return NSLOCTEXT("AI行为树", "定点巡逻说明", "在点位ID对应的场景点位范围内随机移动，范围由点位统一配置。");
	case ELxAIBehaviorAction::RoutePatrol: return NSLOCTEXT("AI行为树", "路线巡逻说明", "按路线ID查询场景路线，依照点位顺序往返或循环移动。");
	case ELxAIBehaviorAction::Alert: return NSLOCTEXT("AI行为树", "警戒说明", "始终面朝敌人，并与敌人保持配置的距离。");
	case ELxAIBehaviorAction::MeleeSkill: return NSLOCTEXT("AI行为树", "近战说明", "移动到敌人附近，在近战释放距离内释放指定技能。");
	case ELxAIBehaviorAction::RangedSkill: return NSLOCTEXT("AI行为树", "远程说明", "调整与敌人的距离，在指定距离区间内释放技能。");
	case ELxAIBehaviorAction::BuffSkill: return NSLOCTEXT("AI行为树", "增益说明", "在原地释放指定增益技能。");
	case ELxAIBehaviorAction::Defend: return NSLOCTEXT("AI行为树", "防卫说明", "不释放技能，始终面朝敌人并保持指定距离。");
	case ELxAIBehaviorAction::RandomFlee: return NSLOCTEXT("AI行为树", "随机逃跑说明", "反复向远离敌人的方向选取导航点并移动。");
	case ELxAIBehaviorAction::PointFlee: return NSLOCTEXT("AI行为树", "定点逃跑说明", "前往点位ID对应的目的地，进入点位共享范围即可视为到达。");
	case ELxAIBehaviorAction::RouteFlee: return NSLOCTEXT("AI行为树", "固定路线逃跑说明", "按路线ID依次经过样条路径点，抵达末点后结束本次逃跑。");
	default: return FText();
	}
}
