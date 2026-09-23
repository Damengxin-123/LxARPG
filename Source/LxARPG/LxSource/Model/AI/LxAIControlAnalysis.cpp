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
		case ELxAIBehaviorEntry::Calm: bActive = !bKnownEnemy && !IsAlertAfterAttack(); break;
		case ELxAIBehaviorEntry::EnemyFound: bActive = bKnownEnemy; break;
		case ELxAIBehaviorEntry::EnemyNear: bActive = bKnownEnemy && bEnemyNear; break;
		case ELxAIBehaviorEntry::Attacked: bActive = bAttackEligible; break;
		default: break;
		}
		if (!bActive || !Configuration->FindEligibleBranch(*EntryNode, HealthRatio, Next.StateId, Next.PhaseId)) continue;
		Next.bHasBranch = true;
		Next.Entry = EntryNode->Entry;
		Next.EntryId = EntryNode->NodeId;
		break;
	}
	// 每个受击脉冲只参与一次选路；没有可用分支或不能打断时不会延迟重放。
	bPendingAttack = false;
	if (!bCanInterrupt && CurrentDecision.bHasBranch && !bResponseCompleted)
	{
		CurrentDecision.bStartNewBehavior = false;
		return CurrentDecision;
	}
	Next.bStartNewBehavior = Next.bHasBranch && (bResponseCompleted || !CurrentDecision.bHasBranch
		|| Next.StateId != CurrentDecision.StateId || Next.PhaseId != CurrentDecision.PhaseId);
	bAttackedResponseActive = Next.bHasBranch && Next.Entry == ELxAIBehaviorEntry::Attacked;
	bResponseCompleted = false;
	CurrentDecision = Next;
	return CurrentDecision;
}
