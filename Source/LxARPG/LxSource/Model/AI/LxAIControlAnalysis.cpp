#include "LxAIControlAnalysis.h"

#include "DataType/LxAIBehaviorTreeAsset.h"
#include "GameFramework/Actor.h"

void ULxAIControlAnalysis::Initialize(ULxAIBehaviorTreeAsset* InConfiguration)
{
	Configuration = InConfiguration;
	PreviousEnemy.Reset();
	CurrentDecision = FLxAIAnalysisDecision();
	AlertSecondsRemaining = 0.0f;
	bPendingAttack = false;
	bAttackedResponseActive = false;
	bEnemyNear = false;
	bResponseCompleted = false;
	bCharacterDeathCompleted = false;
	bHasSelfLocation = false;
	bHasChaseOrigin = false;
	bReturningFromChase = false;
	SelfLocation = FVector::ZeroVector;
	ChaseOrigin = FVector::ZeroVector;
}

void ULxAIControlAnalysis::NotifyAttacked()
{
	if (!Configuration) return;
	FText Error;
	if (!Configuration->Analysis.ValidateConfiguration(Error)) return;
	AlertSecondsRemaining = Configuration->Analysis.AttackedAlertSeconds;
	if (!bAttackedResponseActive) bPendingAttack = true;
}

void ULxAIControlAnalysis::CompleteAttackedResponse()
{
	if (!bAttackedResponseActive) return;
	bAttackedResponseActive = false;
	bPendingAttack = false;
	bResponseCompleted = true;
}

FLxAIAnalysisDecision ULxAIControlAnalysis::Evaluate(AActor* KnownEnemy, float KnownEnemyDistanceMeters, float HealthRatio, float DeltaSeconds, bool bCanInterrupt)
{
	FLxAIAnalysisDecision Next;
	FText Error;
	const bool bKnownEnemy = IsValid(KnownEnemy) && !KnownEnemy->IsActorBeingDestroyed();
	if (bCharacterDeathCompleted) return CurrentDecision;
	if (Configuration && FMath::IsFinite(HealthRatio) && HealthRatio <= 0.0f)
	{
		if (Configuration->ValidateConfiguration(Error)) for (const ULxAIBehaviorTreeNodeData* EntryNode : Configuration->GetOrderedEntries())
		{
			if (EntryNode->Entry != ELxAIBehaviorEntry::CharacterDeath) continue;
			for (const FGuid& StateId : EntryNode->Children)
			{
				const ULxAIBehaviorTreeNodeData* State = Configuration->FindNode(StateId);
				if (!State || State->Kind != ELxAIBehaviorNodeKind::State) continue;
				for (const FGuid& PhaseId : State->Children)
				{
					const ULxAIBehaviorTreeNodeData* Phase = Configuration->FindNode(PhaseId);
					if (!Phase || Phase->Kind != ELxAIBehaviorNodeKind::Phase) continue;
					Next.bHasBranch = true;
					Next.Entry = EntryNode->Entry;
					Next.EntryId = EntryNode->NodeId;
					Next.StateId = StateId;
					Next.PhaseId = PhaseId;
					break;
				}
				if (Next.bHasBranch) break;
			}
			break;
		}
		if (!Next.bHasBranch)
		{
			for (const ULxAIBehaviorTreeNodeData* Node : Configuration->Nodes)
				if (Node && Node->Kind == ELxAIBehaviorNodeKind::Action && Node->Action == ELxAIBehaviorAction::EnterDeath)
				{
					Next.bHasBranch = true;
					Next.Entry = ELxAIBehaviorEntry::CharacterDeath;
					Next.EntryId = Node->NodeId;
					break;
				}
		}
		if (!Next.bHasBranch)
		{
			Next.bHasBranch = true;
			Next.Entry = ELxAIBehaviorEntry::CharacterDeath;
			Next.EntryId = FGuid();
		}
		Next.bStartNewBehavior = Next.bHasBranch && (!CurrentDecision.bHasBranch || CurrentDecision.Entry != ELxAIBehaviorEntry::CharacterDeath
			|| Next.EntryId != CurrentDecision.EntryId || Next.StateId != CurrentDecision.StateId || Next.PhaseId != CurrentDecision.PhaseId);
		CurrentDecision = Next;
		return CurrentDecision;
	}
	if (!Configuration || !Configuration->ValidateConfiguration(Error)
		|| !FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.0f
		|| !FMath::IsFinite(HealthRatio) || HealthRatio < 0.0f || HealthRatio > 1.0f
		|| (bKnownEnemy && (!FMath::IsFinite(KnownEnemyDistanceMeters) || KnownEnemyDistanceMeters < 0.0f)))
	{
		CurrentDecision = Next;
		bPendingAttack = false;
		bAttackedResponseActive = false;
		bEnemyNear = false;
		PreviousEnemy.Reset();
		return CurrentDecision;
	}
	AlertSecondsRemaining = FMath::Max(0.0f, AlertSecondsRemaining - DeltaSeconds);
	const ULxAIBehaviorTreeNodeData* CurrentState = Configuration->FindNode(CurrentDecision.StateId);
	const bool bChaseLimitReached = bHasSelfLocation && bHasChaseOrigin && CurrentState &&
		CurrentState->State == ELxAIBehaviorState::Combat && CurrentState->ChaseDistanceMeters > 0.0f &&
		FVector::Dist2D(SelfLocation, ChaseOrigin) >= CurrentState->ChaseDistanceMeters * 100.0f;
	if (bChaseLimitReached)
	{
		bReturningFromChase = true;
		bHasChaseOrigin = false;
	}
	if (bReturningFromChase)
	{
		bPendingAttack = false;
		bAttackedResponseActive = false;
		AlertSecondsRemaining = 0.0f;
	}
	if (!bKnownEnemy || PreviousEnemy.Get() != KnownEnemy) bEnemyNear = false;
	PreviousEnemy = bKnownEnemy ? KnownEnemy : nullptr;
	if (bKnownEnemy)
		bEnemyNear = bEnemyNear ? KnownEnemyDistanceMeters <= Configuration->Analysis.NearExitDistanceMeters
			: KnownEnemyDistanceMeters <= Configuration->Analysis.NearEnterDistanceMeters;
	const bool bAttackEligible = bPendingAttack || bAttackedResponseActive;
	for (const ULxAIBehaviorTreeNodeData* EntryNode : Configuration->GetOrderedEntries())
	{
		bool bActive = false;
		switch (EntryNode->Entry)
		{
		case ELxAIBehaviorEntry::Calm: bActive = bReturningFromChase || (!bKnownEnemy && !IsAlertAfterAttack()); break;
		case ELxAIBehaviorEntry::EnemyFound: bActive = !bReturningFromChase && bKnownEnemy; break;
		case ELxAIBehaviorEntry::EnemyNear: bActive = !bReturningFromChase && bKnownEnemy && bEnemyNear; break;
		case ELxAIBehaviorEntry::Attacked: bActive = bAttackEligible; break;
		case ELxAIBehaviorEntry::CharacterDeath: bActive = false; break;
		default: break;
		}
		if (!bActive || !Configuration->FindEligibleBranch(*EntryNode, HealthRatio, Next.StateId, Next.PhaseId,
			bKnownEnemy ? KnownEnemyDistanceMeters : TNumericLimits<float>::Max(), !bReturningFromChase)) continue;
		Next.bHasBranch = true;
		Next.Entry = EntryNode->Entry;
		Next.EntryId = EntryNode->NodeId;
		break;
	}
	// 每个受击脉冲只参与一次选路；没有可用分支或不能打断时不会延迟重放。
	bPendingAttack = false;
	if (!bCanInterrupt && !bChaseLimitReached && CurrentDecision.bHasBranch && !bResponseCompleted)
	{
		CurrentDecision.bStartNewBehavior = false;
		return CurrentDecision;
	}
	Next.bStartNewBehavior = Next.bHasBranch && (bResponseCompleted || !CurrentDecision.bHasBranch
		|| Next.EntryId != CurrentDecision.EntryId || Next.StateId != CurrentDecision.StateId
		|| Next.PhaseId != CurrentDecision.PhaseId);
	bAttackedResponseActive = Next.bHasBranch && Next.Entry == ELxAIBehaviorEntry::Attacked;
	bResponseCompleted = false;
	const ULxAIBehaviorTreeNodeData* NextState = Configuration->FindNode(Next.StateId);
	if (NextState && NextState->State == ELxAIBehaviorState::Combat && bHasSelfLocation)
	{
		if (!bHasChaseOrigin) { ChaseOrigin = SelfLocation; bHasChaseOrigin = true; }
	}
	else bHasChaseOrigin = false;
	CurrentDecision = Next;
	return CurrentDecision;
}
