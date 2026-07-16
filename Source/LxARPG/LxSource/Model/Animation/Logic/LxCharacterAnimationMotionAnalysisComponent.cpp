#include "LxCharacterAnimationMotionAnalysisComponent.h"

ULxCharacterAnimationMotionAnalysisComponent::ULxCharacterAnimationMotionAnalysisComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterAnimationMotionAnalysisComponent::ReceiveBaseMotionEvent(const FLxCharacterMotionSignal& InMotionSignal)
{
	const FLxCharacterMotionSignal AnalyzedSignal = AnalyzeMotionEvent(InMotionSignal);
	OnBaseMotionSignalAnalyzed.Broadcast(AnalyzedSignal);
}

void ULxCharacterAnimationMotionAnalysisComponent::ReceiveActionMotionEvent(const FLxCharacterMotionSignal& InMotionSignal)
{
	const FLxCharacterMotionSignal AnalyzedSignal = AnalyzeMotionEvent(InMotionSignal);
	OnActionMotionSignalAnalyzed.Broadcast(AnalyzedSignal);
}

FLxCharacterMotionSignal ULxCharacterAnimationMotionAnalysisComponent::AnalyzeMotionEvent_Implementation(const FLxCharacterMotionSignal& InMotionSignal) const
{
	return InMotionSignal;
}
