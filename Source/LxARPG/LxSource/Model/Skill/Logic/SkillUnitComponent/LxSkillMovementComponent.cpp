#include "LxSkillMovementComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
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

	FVector MoveDelta = FVector::ZeroVector;
	if (bGravityEnabled)
	{
		const float PreviousSpeedCmPerSecond = CurrentSpeedCmPerSecond;
		CurrentSpeedCmPerSecond = FMath::Max(
			CurrentSpeedCmPerSecond + MovementSpec.GetAccelerationInUnrealUnits() * DeltaTime,
			0.0f);
		FVector AccelerationDirection = CurrentVelocityCmPerSecond;
		AccelerationDirection.Z = 0.0f;
		if (!AccelerationDirection.Normalize())
		{
			AccelerationDirection = TargetComponent->GetForwardVector();
			AccelerationDirection.Z = 0.0f;
			AccelerationDirection.Normalize();
		}
		CurrentVelocityCmPerSecond += AccelerationDirection
			* (CurrentSpeedCmPerSecond - PreviousSpeedCmPerSecond);

		if (const UWorld* World = GetWorld())
		{
			CurrentVelocityCmPerSecond.Z += World->GetGravityZ()
				* FMath::Max(MovementSpec.GravityScale, 0.0f)
				* DeltaTime;
		}
		MoveDelta = CurrentVelocityCmPerSecond * DeltaTime;
	}
	else
	{
		CurrentSpeedCmPerSecond += MovementSpec.GetAccelerationInUnrealUnits() * DeltaTime;
		const float MoveDistanceCm = FMath::Max(CurrentSpeedCmPerSecond, 0.0f) * DeltaTime;
		MoveDelta = TargetComponent->GetForwardVector() * MoveDistanceCm;
		CurrentVelocityCmPerSecond = DeltaTime > UE_SMALL_NUMBER ? MoveDelta / DeltaTime : FVector::ZeroVector;
	}

	if (MoveDelta.IsNearlyZero())
	{
		return;
	}

	TargetComponent->AddWorldOffset(MoveDelta, true);
	TraveledDistanceCm += MoveDelta.Size();

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
	if (USceneComponent* TargetComponent = ResolveMovementTargetComponent())
	{
		CurrentVelocityCmPerSecond = TargetComponent->GetForwardVector() * CurrentSpeedCmPerSecond;
	}
	else
	{
		CurrentVelocityCmPerSecond = FVector::ZeroVector;
	}
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

void ULxSkillMovementComponent::SetGravityEnabled(bool bInGravityEnabled)
{
	bGravityEnabled = bInGravityEnabled;
}

void ULxSkillMovementComponent::SetCurrentVelocity(const FVector& InVelocity)
{
	CurrentVelocityCmPerSecond = InVelocity;
}
void ULxSkillMovementComponent::ReflectCurrentVelocity(const FVector& HitNormal, float VelocityRetention)
{
	const FVector SafeHitNormal = HitNormal.GetSafeNormal();
	if (SafeHitNormal.IsNearlyZero() || FVector::DotProduct(CurrentVelocityCmPerSecond, SafeHitNormal) >= 0.0f)
	{
		return;
	}

	CurrentVelocityCmPerSecond = CurrentVelocityCmPerSecond.MirrorByVector(SafeHitNormal)
		* FMath::Clamp(VelocityRetention, 0.0f, 1.0f);
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
