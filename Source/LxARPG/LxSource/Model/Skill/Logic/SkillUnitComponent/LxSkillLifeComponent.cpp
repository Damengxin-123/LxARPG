#include "LxSkillLifeComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

void ULxSkillLifeComponent::SetLifeSpec(const FLxSkillLifeSpec& InLifeSpec)
{
	LifeSpec = InLifeSpec;
	OnDataChange.Broadcast();
}

void ULxSkillLifeComponent::SetLifeTickInterval(float InLifeTickInterval)
{
	LifeTickInterval = FMath::Max(InLifeTickInterval, 0.0f);
}

void ULxSkillLifeComponent::StartLife()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(LifeTimerHandle);
	World->GetTimerManager().ClearTimer(LifeTickTimerHandle);
	LifeStartWorldTime = World->GetTimeSeconds();

	SetLifeState(ELxSkillAbilityComponentState::Running);

	if (LifeSpec.Duration > 0.0f)
	{
		World->GetTimerManager().SetTimer(LifeTimerHandle, this, &ULxSkillLifeComponent::HandleLifeExpired, LifeSpec.Duration, false);
	}

	if (LifeTickInterval > 0.0f)
	{
		World->GetTimerManager().SetTimer(LifeTickTimerHandle, this, &ULxSkillLifeComponent::HandleLifeTick, LifeTickInterval, true);
	}
}

void ULxSkillLifeComponent::StopLife()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(LifeTimerHandle);
		World->GetTimerManager().ClearTimer(LifeTickTimerHandle);
	}

	SetLifeState(ELxSkillAbilityComponentState::Stopped);
}

float ULxSkillLifeComponent::GetRemainingDuration() const
{
	if (LifeSpec.Duration <= 0.0f || LifeStartWorldTime < 0.0f)
	{
		return -1.0f;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.0f;
	}

	return FMath::Max(LifeSpec.Duration - (World->GetTimeSeconds() - LifeStartWorldTime), 0.0f);
}

void ULxSkillLifeComponent::SetLifeState(ELxSkillAbilityComponentState NewState)
{
	if (LifeState == NewState)
	{
		return;
	}

	const ELxSkillAbilityComponentState OldState = LifeState;
	LifeState = NewState;
	OnLifeStateChanged.Broadcast(OldState, LifeState);
	OnDataChange.Broadcast();
}

void ULxSkillLifeComponent::HandleLifeExpired()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeTickTimerHandle);
	}

	SetLifeState(ELxSkillAbilityComponentState::Finished);
}

void ULxSkillLifeComponent::HandleLifeTick()
{
	OnLifeTick.Broadcast(GetRemainingDuration());
}
