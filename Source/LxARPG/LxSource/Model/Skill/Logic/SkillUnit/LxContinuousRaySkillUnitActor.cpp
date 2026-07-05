#include "LxContinuousRaySkillUnitActor.h"

#include "Engine/World.h"

ALxContinuousRaySkillUnitActor::ALxContinuousRaySkillUnitActor()
{
	SetActorHiddenInGame(true);
}

void ALxContinuousRaySkillUnitActor::InitializeContinuousRayParameters(const FLxContinuousRayEffectSpec& InContinuousRaySpec)
{
	ContinuousRaySpec = InContinuousRaySpec;
}

void ALxContinuousRaySkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		return;
	}

	Super::ActivateSkillUnit_Implementation();
	SetActorHiddenInGame(false);
	if (!HasAuthority() || !IsSkillUnitActive())
	{
		return;
	}

	HandleContinuousRayTick();

	if (UWorld* World = GetWorld())
	{
		const float EffectiveInterval = FMath::Max(ContinuousRaySpec.TriggerInterval, 0.01f);
		World->GetTimerManager().SetTimer(
			ContinuousRayTimerHandle,
			this,
			&ALxContinuousRaySkillUnitActor::HandleContinuousRayTick,
			EffectiveInterval,
			true);
	}
}

void ALxContinuousRaySkillUnitActor::StopSkillUnit_Implementation()
{
	ClearContinuousRayTimer();
	SetActorHiddenInGame(true);
	Super::StopSkillUnit_Implementation();
}

void ALxContinuousRaySkillUnitActor::CancelSkillUnit_Implementation()
{
	ClearContinuousRayTimer();
	SetActorHiddenInGame(true);
	Super::CancelSkillUnit_Implementation();
}

void ALxContinuousRaySkillUnitActor::HandleContinuousRayTick()
{
	if (IsSkillUnitActive())
	{
		PerformRayDetection();
	}
}

void ALxContinuousRaySkillUnitActor::ClearContinuousRayTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ContinuousRayTimerHandle);
	}
}
