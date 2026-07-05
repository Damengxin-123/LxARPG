#include "LxDirectHitAreaSkillUnitActor.h"

#include "Engine/World.h"
#include "TimerManager.h"

void ALxDirectHitAreaSkillUnitActor::ActivateSkillUnit_Implementation()
{
	Super::ActivateSkillUnit_Implementation();
	if (AreaEffectSpec.Duration <= 0.0f)
	{
		ScanCurrentAreaTargets();
		FinishSkillUnit(MakeSkillUnitResult(ELxSkillUnitResultType::Completed, true));
		Destroy();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this,
			&ALxDirectHitAreaSkillUnitActor::HandleDeferredInitialScan);
	}
}

void ALxDirectHitAreaSkillUnitActor::HandleDeferredInitialScan()
{
	if (IsValid(this) && IsSkillUnitActive())
	{
		ScanCurrentAreaTargets();
	}
}

bool ALxDirectHitAreaSkillUnitActor::ShouldProcessAreaDetectionResult(const FLxSkillDetectionResult& DetectionResult) const
{
	return DetectionResult.EventType == ELxSkillDetectionEventType::ManualScan;
}
