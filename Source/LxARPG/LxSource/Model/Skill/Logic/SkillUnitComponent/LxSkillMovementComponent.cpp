#include "LxSkillMovementComponent.h"

#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

ULxSkillMovementComponent::ULxSkillMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void ULxSkillMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (MovementState != ELxSkillAbilityComponentState::Running)
	{
		return;
	}

	USceneComponent* TargetComponent = ResolveMovementTargetComponent();
	if (!TargetComponent)
	{
		StopMovement();
		return;
	}

	CurrentSpeedCmPerSecond += MovementSpec.GetAccelerationInUnrealUnits() * DeltaTime;
	const float MoveDistanceCm = FMath::Max(CurrentSpeedCmPerSecond, 0.0f) * DeltaTime;
	if (MoveDistanceCm <= 0.0f)
	{
		return;
	}

	const FVector MoveDelta = TargetComponent->GetForwardVector() * MoveDistanceCm;
	TargetComponent->AddWorldOffset(MoveDelta, true);
	TraveledDistanceCm += MoveDistanceCm;

	const float MaxDistanceCm = MovementSpec.GetMaxDistanceInUnrealUnits();
	if (MaxDistanceCm > 0.0f && TraveledDistanceCm >= MaxDistanceCm)
	{
		OnReachMaxDistance.Broadcast(GetMovementProgress());
		SetMovementState(ELxSkillAbilityComponentState::Finished);
		SetComponentTickEnabled(false);
	}
}

void ULxSkillMovementComponent::SetMovementSpec(const FLxSkillMovementSpec& InMovementSpec)
{
	MovementSpec = InMovementSpec;
	CurrentSpeedCmPerSecond = MovementSpec.GetSpeedInUnrealUnits();
	OnDataChange.Broadcast();
}

void ULxSkillMovementComponent::SetMovementTargetComponent(USceneComponent* InMovementTargetComponent)
{
	MovementTargetComponent = InMovementTargetComponent;
}

void ULxSkillMovementComponent::StartMovement()
{
	TraveledDistanceCm = 0.0f;
	CurrentSpeedCmPerSecond = MovementSpec.GetSpeedInUnrealUnits();
	SetMovementState(ELxSkillAbilityComponentState::Running);
	SetComponentTickEnabled(true);
}

void ULxSkillMovementComponent::PauseMovement()
{
	if (MovementState != ELxSkillAbilityComponentState::Running)
	{
		return;
	}

	SetMovementState(ELxSkillAbilityComponentState::Paused);
	SetComponentTickEnabled(false);
}

void ULxSkillMovementComponent::StopMovement()
{
	SetMovementState(ELxSkillAbilityComponentState::Stopped);
	SetComponentTickEnabled(false);
}

float ULxSkillMovementComponent::GetMovementProgress() const
{
	const float MaxDistanceCm = MovementSpec.GetMaxDistanceInUnrealUnits();
	if (MaxDistanceCm <= 0.0f)
	{
		return 0.0f;
	}

	return FMath::Clamp(TraveledDistanceCm / MaxDistanceCm, 0.0f, 1.0f);
}

void ULxSkillMovementComponent::SetMovementState(ELxSkillAbilityComponentState NewState)
{
	if (MovementState == NewState)
	{
		return;
	}

	const ELxSkillAbilityComponentState OldState = MovementState;
	MovementState = NewState;
	OnMovementStateChanged.Broadcast(OldState, MovementState);
	OnDataChange.Broadcast();
}

USceneComponent* ULxSkillMovementComponent::ResolveMovementTargetComponent() const
{
	if (MovementTargetComponent)
	{
		return MovementTargetComponent;
	}

	const AActor* Owner = GetOwner();
	return Owner ? Owner->GetRootComponent() : nullptr;
}
