#include "LxCharacterBehaviorControlComponent.h"

#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterBehaviorControlComponent::ULxCharacterBehaviorControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void ULxCharacterBehaviorControlComponent::BaseComponentInitialize()
{
	CacheOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	if (const UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		bWasFalling = MovementComponent->IsFalling();
	}
	RefreshBaseBehaviorState();
}

void ULxCharacterBehaviorControlComponent::BeginPlay()
{
	Super::BeginPlay();
	BaseComponentInitialize();
}

void ULxCharacterBehaviorControlComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	PendingJumpStartRemaining = FMath::Max(0.0f, PendingJumpStartRemaining - DeltaTime);
	UpdateFacingControl(DeltaTime);

	BehaviorSampleAccumulator += DeltaTime;
	if (BehaviorSampleInterval <= 0.0f || BehaviorSampleAccumulator >= BehaviorSampleInterval)
	{
		BehaviorSampleAccumulator = 0.0f;
		RefreshBaseBehaviorState();
	}
}

void ULxCharacterBehaviorControlComponent::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterBehaviorControlComponent, ActiveBehaviorStateTags);
}

void ULxCharacterBehaviorControlComponent::HandleMoveInput(const FVector2D& InMoveValue)
{
	CacheOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->Controller || InMoveValue.IsNearlyZero())
	{
		return;
	}

	const ULxCharacterAttributeComponent* SpecialAttributeComponent =
		OwnerCharacter->GetCharacterSpecialAttributeComponent();
	if (SpecialAttributeComponent && !SpecialAttributeComponent->IsCharacterAlive())
	{
		return;
	}

	const FRotator ControlRotation = OwnerCharacter->Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector MoveDirection = (ForwardDirection * InMoveValue.Y + RightDirection * InMoveValue.X).GetSafeNormal2D();

	OwnerCharacter->AddMovementInput(ForwardDirection, InMoveValue.Y);
	OwnerCharacter->AddMovementInput(RightDirection, InMoveValue.X);
	if (CanRotateByMoveInput())
	{
		UpdateMoveFacing(MoveDirection, GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f);
	}
}

void ULxCharacterBehaviorControlComponent::HandleJumpInput(const bool bPressed)
{
	CacheOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	const ULxCharacterAttributeComponent* SpecialAttributeComponent =
		OwnerCharacter->GetCharacterSpecialAttributeComponent();
	if (SpecialAttributeComponent && !SpecialAttributeComponent->IsCharacterAlive())
	{
		return;
	}

	if (bPressed)
	{
		PendingJumpStartRemaining = OwnerCharacter->CanJump() ? JumpStartRecognitionDuration : 0.0f;
		OwnerCharacter->Jump();
	}
	else
	{
		OwnerCharacter->StopJumping();
	}
}

void ULxCharacterBehaviorControlComponent::HandleLookInput(const FVector2D& InLookValue)
{
	CacheOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->Controller)
	{
		return;
	}
	OwnerCharacter->AddControllerYawInput(InLookValue.X);
	OwnerCharacter->AddControllerPitchInput(InLookValue.Y);
}

bool ULxCharacterBehaviorControlComponent::RequestMoveToActor(AActor* InTargetActor, const float InAcceptanceRadius)
{
	CacheOwnerCharacter();
	AAIController* AIController = OwnerCharacter ? Cast<AAIController>(OwnerCharacter->GetController()) : nullptr;
	if (!AIController || !IsValid(InTargetActor))
	{
		return false;
	}
	return AIController->MoveToActor(InTargetActor, InAcceptanceRadius, true, true, true, nullptr, true) !=
		EPathFollowingRequestResult::Failed;
}

bool ULxCharacterBehaviorControlComponent::RequestMoveToLocation(const FVector InTargetLocation,
	const float InAcceptanceRadius)
{
	CacheOwnerCharacter();
	AAIController* AIController = OwnerCharacter ? Cast<AAIController>(OwnerCharacter->GetController()) : nullptr;
	if (!AIController)
	{
		return false;
	}
	return AIController->MoveToLocation(InTargetLocation, InAcceptanceRadius, true, true, false, true, nullptr, true) !=
		EPathFollowingRequestResult::Failed;
}

void ULxCharacterBehaviorControlComponent::StopActiveMovement()
{
	CacheOwnerCharacter();
	if (AAIController* AIController = OwnerCharacter ? Cast<AAIController>(OwnerCharacter->GetController()) : nullptr)
	{
		AIController->StopMovement();
	}
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
}

bool ULxCharacterBehaviorControlComponent::IsNavigationMoving() const
{
	const AAIController* AIController = OwnerCharacter ? Cast<AAIController>(OwnerCharacter->GetController()) : nullptr;
	return AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Moving;
}

void ULxCharacterBehaviorControlComponent::AddBehaviorState(const FGameplayTag InBehaviorStateTag)
{
	if (!InBehaviorStateTag.IsValid())
	{
		return;
	}
	int32& RequestCount = BehaviorStateRequestCounts.FindOrAdd(InBehaviorStateTag);
	RequestCount = FMath::Max(0, RequestCount) + 1;
	SetBehaviorStateActive(InBehaviorStateTag, true);
	CacheOwnerCharacter();
	if (OwnerCharacter && !OwnerCharacter->HasAuthority() && OwnerCharacter->IsLocallyControlled())
	{
		ServerAddBehaviorState(InBehaviorStateTag);
	}
}

void ULxCharacterBehaviorControlComponent::RemoveBehaviorState(const FGameplayTag InBehaviorStateTag)
{
	int32* RequestCount = BehaviorStateRequestCounts.Find(InBehaviorStateTag);
	if (!RequestCount)
	{
		return;
	}
	*RequestCount = FMath::Max(0, *RequestCount - 1);
	if (*RequestCount <= 0)
	{
		BehaviorStateRequestCounts.Remove(InBehaviorStateTag);
		SetBehaviorStateActive(InBehaviorStateTag, false);
	}
	CacheOwnerCharacter();
	if (OwnerCharacter && !OwnerCharacter->HasAuthority() && OwnerCharacter->IsLocallyControlled())
	{
		ServerRemoveBehaviorState(InBehaviorStateTag);
	}
}

bool ULxCharacterBehaviorControlComponent::HasBehaviorState(const FGameplayTag InBehaviorStateTag) const
{
	return InBehaviorStateTag.IsValid() && ActiveBehaviorStateTags.HasTag(InBehaviorStateTag);
}

void ULxCharacterBehaviorControlComponent::AddFacingControlRequest()
{
	FacingControlRequestCount = FMath::Max(0, FacingControlRequestCount) + 1;
	FacingControlHoldRemaining = 0.0f;
	SetBehaviorStateActive(LxTag_CharacterState_Combat_CombatFacing, true);
	CacheOwnerCharacter();
	if (OwnerCharacter && !OwnerCharacter->HasAuthority() && OwnerCharacter->IsLocallyControlled())
	{
		ServerAddFacingControlRequest();
	}
}

void ULxCharacterBehaviorControlComponent::RemoveFacingControlRequest(const float InHoldDuration)
{
	FacingControlRequestCount = FMath::Max(0, FacingControlRequestCount - 1);
	CacheOwnerCharacter();
	if (OwnerCharacter && !OwnerCharacter->HasAuthority() && OwnerCharacter->IsLocallyControlled())
	{
		ServerRemoveFacingControlRequest(InHoldDuration);
	}
	if (FacingControlRequestCount > 0)
	{
		return;
	}
	FacingControlHoldRemaining = FMath::Max(0.0f,
		InHoldDuration >= 0.0f ? InHoldDuration : DefaultFacingControlHoldDuration);
	if (FacingControlHoldRemaining <= 0.0f)
	{
		SetBehaviorStateActive(LxTag_CharacterState_Combat_CombatFacing, false);
	}
}

void ULxCharacterBehaviorControlComponent::RefreshFacingControl(const float InHoldDuration)
{
	const float HoldDuration = FMath::Max(0.0f,
		InHoldDuration >= 0.0f ? InHoldDuration : DefaultFacingControlHoldDuration);
	FacingControlHoldRemaining = FMath::Max(FacingControlHoldRemaining, HoldDuration);
	SetBehaviorStateActive(LxTag_CharacterState_Combat_CombatFacing, true);
	CacheOwnerCharacter();
	if (OwnerCharacter && !OwnerCharacter->HasAuthority() && OwnerCharacter->IsLocallyControlled())
	{
		ServerRefreshFacingControl(InHoldDuration);
	}
}

void ULxCharacterBehaviorControlComponent::ServerAddBehaviorState_Implementation(
	const FGameplayTag InBehaviorStateTag)
{
	AddBehaviorState(InBehaviorStateTag);
}

void ULxCharacterBehaviorControlComponent::ServerRemoveBehaviorState_Implementation(
	const FGameplayTag InBehaviorStateTag)
{
	RemoveBehaviorState(InBehaviorStateTag);
}

void ULxCharacterBehaviorControlComponent::ServerAddFacingControlRequest_Implementation()
{
	AddFacingControlRequest();
}

void ULxCharacterBehaviorControlComponent::ServerRemoveFacingControlRequest_Implementation(
	const float InHoldDuration)
{
	RemoveFacingControlRequest(InHoldDuration);
}

void ULxCharacterBehaviorControlComponent::ServerRefreshFacingControl_Implementation(const float InHoldDuration)
{
	RefreshFacingControl(InHoldDuration);
}

void ULxCharacterBehaviorControlComponent::SetDesiredFacingDirection(FVector InWorldDirection)
{
	InWorldDirection.Z = 0.0f;
	if (!InWorldDirection.IsNearlyZero())
	{
		DesiredFacingDirection = InWorldDirection.GetSafeNormal();
	}
}

bool ULxCharacterBehaviorControlComponent::IsFacingControlActive() const
{
	return FacingControlRequestCount > 0 || FacingControlHoldRemaining > 0.0f ||
		HasBehaviorState(LxTag_CharacterState_Combat_CombatFacing);
}

void ULxCharacterBehaviorControlComponent::AddMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount) + 1;
}

void ULxCharacterBehaviorControlComponent::RemoveMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount - 1);
}

bool ULxCharacterBehaviorControlComponent::CanRotateByMoveInput() const
{
	return MoveRotationLockCount <= 0 && !IsFacingControlActive();
}

void ULxCharacterBehaviorControlComponent::RefreshBaseBehaviorState()
{
	CacheOwnerCharacter();
	const ULxCharacterAttributeComponent* SpecialAttributeComponent = OwnerCharacter
		? OwnerCharacter->GetCharacterSpecialAttributeComponent()
		: nullptr;
	if (SpecialAttributeComponent && !SpecialAttributeComponent->IsCharacterAlive())
	{
		SetBehaviorStateActive(LxTag_CharacterState_Lifecycle_Dead, true);
		return;
	}
	SetBehaviorStateActive(LxTag_CharacterState_Lifecycle_Dead, false);

	UCharacterMovementComponent* MovementComponent = OwnerCharacter ? OwnerCharacter->GetCharacterMovement() : nullptr;
	if (!MovementComponent)
	{
		return;
	}

	const bool bIsFalling = MovementComponent->IsFalling();
	const FVector Velocity = MovementComponent->Velocity;
	const float HorizontalSpeed = Velocity.Size2D();
	FLxCharacterMotionSignal MotionSignal;
	MotionSignal.MotionSpeed = Velocity.Size();
	MotionSignal.bLoop = true;

	if (bIsFalling)
	{
		const bool bStartedFromJumpInput = !bWasFalling && PendingJumpStartRemaining > 0.0f;
		if (bStartedFromJumpInput)
		{
			SetLocomotionState(LxTag_CharacterState_Movement_Jumping);
			MotionSignal.MotionType = ELxCharacterMotionType::JumpStart;
			MotionSignal.MotionDirection = FVector::UpVector;
			MotionSignal.bLoop = false;
			PendingJumpStartRemaining = 0.0f;
		}
		else
		{
			SetLocomotionState(LxTag_CharacterState_Movement_Airborne);
			MotionSignal.MotionType = ELxCharacterMotionType::Airborne;
			MotionSignal.MotionDirection = Velocity.GetSafeNormal();
			MotionSignal.bLoop = true;
		}
	}
	else if (bWasFalling)
	{
		SetLocomotionState(LxTag_CharacterState_Movement_Landing);
		MotionSignal.MotionType = ELxCharacterMotionType::JumpEnd;
		MotionSignal.MotionDirection = FVector::DownVector;
		MotionSignal.bLoop = false;
	}
	else if (HorizontalSpeed > FMath::Max(RunSpeedThreshold, IdleSpeedThreshold))
	{
		SetLocomotionState(LxTag_CharacterState_Movement_Running);
		MotionSignal.MotionType = ELxCharacterMotionType::Run;
		MotionSignal.MotionDirection = Velocity.GetSafeNormal2D();
	}
	else if (HorizontalSpeed > IdleSpeedThreshold)
	{
		SetLocomotionState(LxTag_CharacterState_Movement_Moving);
		MotionSignal.MotionType = ELxCharacterMotionType::Move;
		MotionSignal.MotionDirection = Velocity.GetSafeNormal2D();
	}
	else
	{
		SetLocomotionState(LxTag_CharacterState_Movement_Idle);
		MotionSignal.MotionType = ELxCharacterMotionType::Idle;
		MotionSignal.MotionDirection = FVector::ZeroVector;
		MotionSignal.MotionSpeed = 0.0f;
	}

	bWasFalling = bIsFalling;
	if (!ShouldSendBaseAnimationSignal(MotionSignal))
	{
		return;
	}

	// 旧角色状态仅作为现有蓝图兼容快照，新的玩法判断统一读取行为状态标签。
	if (OwnerCharacter->HasAuthority() || OwnerCharacter->IsLocallyControlled())
	{
		switch (MotionSignal.MotionType)
		{
		case ELxCharacterMotionType::Move:
		case ELxCharacterMotionType::Run:
			OwnerCharacter->SetCharacterState(ELxCharacterState::Moving);
			break;
		case ELxCharacterMotionType::JumpStart:
		case ELxCharacterMotionType::Airborne:
			OwnerCharacter->SetCharacterState(ELxCharacterState::JumpStart);
			break;
		case ELxCharacterMotionType::JumpEnd:
			OwnerCharacter->SetCharacterState(ELxCharacterState::JumpEnd);
			break;
		default:
			OwnerCharacter->SetCharacterState(ELxCharacterState::Idle);
			break;
		}
	}

	SendBaseAnimationMotionSignal(MotionSignal);
	LastBaseAnimationSignal = MotionSignal;
	bHasSentBaseAnimationSignal = true;
}

void ULxCharacterBehaviorControlComponent::SendBaseAnimationMotionSignal(
	const FLxCharacterMotionSignal& InMotionSignal)
{
	OnBaseMotionSignalChanged.Broadcast(InMotionSignal);
}

void ULxCharacterBehaviorControlComponent::SendActionAnimationMotionSignal(
	const FLxCharacterMotionSignal& InMotionSignal)
{
	OnActionMotionSignalChanged.Broadcast(InMotionSignal);
}

void ULxCharacterBehaviorControlComponent::ResendCurrentBaseAnimationMotionSignal()
{
	if (bHasSentBaseAnimationSignal)
	{
		SendBaseAnimationMotionSignal(LastBaseAnimationSignal);
	}
}

void ULxCharacterBehaviorControlComponent::CacheOwnerCharacter()
{
	if (!OwnerCharacter)
	{
		OwnerCharacter = GetCharacterOwner();
	}
}

void ULxCharacterBehaviorControlComponent::UpdateFacingControl(const float DeltaTime)
{
	if (FacingControlRequestCount <= 0 && FacingControlHoldRemaining > 0.0f)
	{
		FacingControlHoldRemaining = FMath::Max(0.0f, FacingControlHoldRemaining - DeltaTime);
		if (FacingControlHoldRemaining <= 0.0f)
		{
			SetBehaviorStateActive(LxTag_CharacterState_Combat_CombatFacing, false);
		}
	}

	if (!IsFacingControlActive() || DesiredFacingDirection.IsNearlyZero())
	{
		return;
	}

	CacheOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}
	const FRotator TargetRotation(0.0f, DesiredFacingDirection.Rotation().Yaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpTo(
		OwnerCharacter->GetActorRotation(), TargetRotation, DeltaTime, BehaviorFacingTurnSpeed);
	OwnerCharacter->SetActorRotation(NewRotation);
	if (!OwnerCharacter->HasAuthority())
	{
		OwnerCharacter->ServerSetCharacterRotation(NewRotation);
	}
}

void ULxCharacterBehaviorControlComponent::UpdateMoveFacing(const FVector& InMoveDirection, const float DeltaTime)
{
	if (!OwnerCharacter || InMoveDirection.IsNearlyZero())
	{
		return;
	}
	const FRotator TargetRotation(0.0f, InMoveDirection.Rotation().Yaw, 0.0f);
	const FRotator NewRotation = FMath::RInterpTo(
		OwnerCharacter->GetActorRotation(), TargetRotation, DeltaTime, MoveTurnSpeed);
	OwnerCharacter->SetActorRotation(NewRotation);
	if (!OwnerCharacter->HasAuthority())
	{
		OwnerCharacter->ServerSetCharacterRotation(NewRotation);
	}
}

void ULxCharacterBehaviorControlComponent::SetLocomotionState(const FGameplayTag InLocomotionStateTag)
{
	if (CurrentLocomotionStateTag == InLocomotionStateTag)
	{
		return;
	}
	if (CurrentLocomotionStateTag.IsValid())
	{
		SetBehaviorStateActive(CurrentLocomotionStateTag, false);
	}
	CurrentLocomotionStateTag = InLocomotionStateTag;
	SetBehaviorStateActive(CurrentLocomotionStateTag, true);
}

void ULxCharacterBehaviorControlComponent::SetBehaviorStateActive(const FGameplayTag InBehaviorStateTag,
	const bool bInActive)
{
	if (!InBehaviorStateTag.IsValid())
	{
		return;
	}
	const bool bWasActive = ActiveBehaviorStateTags.HasTagExact(InBehaviorStateTag);
	if (bWasActive == bInActive)
	{
		return;
	}
	if (bInActive)
	{
		ActiveBehaviorStateTags.AddTag(InBehaviorStateTag);
	}
	else
	{
		ActiveBehaviorStateTags.RemoveTag(InBehaviorStateTag);
	}
	OnBehaviorStateChanged.Broadcast(InBehaviorStateTag, bInActive);
	OnDataChange.Broadcast();
}

void ULxCharacterBehaviorControlComponent::BroadcastBehaviorStateDifferences(
	const FGameplayTagContainer& InOldStateTags)
{
	for (const FGameplayTag& StateTag : ActiveBehaviorStateTags)
	{
		if (!InOldStateTags.HasTagExact(StateTag))
		{
			OnBehaviorStateChanged.Broadcast(StateTag, true);
		}
	}
	for (const FGameplayTag& StateTag : InOldStateTags)
	{
		if (!ActiveBehaviorStateTags.HasTagExact(StateTag))
		{
			OnBehaviorStateChanged.Broadcast(StateTag, false);
		}
	}
}

bool ULxCharacterBehaviorControlComponent::ShouldSendBaseAnimationSignal(
	const FLxCharacterMotionSignal& InSignal) const
{
	if (!bHasSentBaseAnimationSignal || InSignal.MotionType != LastBaseAnimationSignal.MotionType)
	{
		return true;
	}
	if (!InSignal.bLoop)
	{
		return false;
	}
	if (FMath::Abs(InSignal.MotionSpeed - LastBaseAnimationSignal.MotionSpeed) >= AnimationSpeedRefreshThreshold)
	{
		return true;
	}
	if (!InSignal.MotionDirection.IsNearlyZero() && !LastBaseAnimationSignal.MotionDirection.IsNearlyZero())
	{
		const float DirectionDot = FVector::DotProduct(InSignal.MotionDirection.GetSafeNormal(),
			LastBaseAnimationSignal.MotionDirection.GetSafeNormal());
		const float DirectionAngle = FMath::RadiansToDegrees(
			FMath::Acos(FMath::Clamp(DirectionDot, -1.0f, 1.0f)));
		return DirectionAngle >= AnimationDirectionRefreshAngle;
	}
	return false;
}

void ULxCharacterBehaviorControlComponent::OnRep_ActiveBehaviorStateTags(
	const FGameplayTagContainer& InOldStateTags)
{
	BroadcastBehaviorStateDifferences(InOldStateTags);
	OnDataChange.Broadcast();
}
