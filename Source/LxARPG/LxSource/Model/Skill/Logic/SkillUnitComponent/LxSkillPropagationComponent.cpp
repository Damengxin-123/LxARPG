#include "LxSkillPropagationComponent.h"

void ULxSkillPropagationComponent::SetPropagationSpec(const FLxSkillPropagationSpec& InPropagationSpec)
{
	PropagationSpec = InPropagationSpec;
	ResetPropagationCounts();
	OnDataChange.Broadcast();
}

void ULxSkillPropagationComponent::ResetPropagationCounts()
{
	RemainingPierceCount = PropagationSpec.MaxPierceCount;
	RemainingBounceCount = PropagationSpec.MaxBounceCount;
	RemainingChainCount = PropagationSpec.MaxChainCount;
}

FLxSkillPropagationResult ULxSkillPropagationComponent::EvaluatePropagation(const FLxSkillTriggerResult& TriggerResult)
{
	FLxSkillPropagationResult Result;
	Result.TriggerResult = TriggerResult;
	Result.RemainingPierceCount = RemainingPierceCount;
	Result.RemainingBounceCount = RemainingBounceCount;
	Result.RemainingChainCount = RemainingChainCount;
	Result.SplitCount = PropagationSpec.SplitCount;

	if (RemainingPierceCount > 0 && TriggerResult.TriggeredTargets.Num() > 0)
	{
		--RemainingPierceCount;
		Result.DecisionType = ELxSkillPropagationDecisionType::Pierce;
		Result.bCanContinue = true;
	}
	else if (RemainingBounceCount > 0)
	{
		--RemainingBounceCount;
		Result.DecisionType = ELxSkillPropagationDecisionType::Bounce;
		Result.bCanContinue = true;
	}
	else if (RemainingChainCount > 0)
	{
		--RemainingChainCount;
		Result.DecisionType = ELxSkillPropagationDecisionType::Chain;
		Result.bCanContinue = true;
	}
	else if (PropagationSpec.SplitCount > 0)
	{
		Result.DecisionType = ELxSkillPropagationDecisionType::Split;
		Result.bCanContinue = true;
	}
	else if (PropagationSpec.GetInfectRadiusInUnrealUnits() > 0.0f)
	{
		Result.DecisionType = ELxSkillPropagationDecisionType::Infect;
		Result.bCanContinue = true;
	}
	else
	{
		Result.DecisionType = ELxSkillPropagationDecisionType::Ended;
		Result.bCanContinue = false;
	}

	Result.RemainingPierceCount = RemainingPierceCount;
	Result.RemainingBounceCount = RemainingBounceCount;
	Result.RemainingChainCount = RemainingChainCount;

	OnPropagationEvaluated.Broadcast(Result);
	OnDataChange.Broadcast();
	return Result;
}
