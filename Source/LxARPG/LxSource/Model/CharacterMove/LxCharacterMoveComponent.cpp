#include "LxCharacterMoveComponent.h"

#include "AIController.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "LxARPG/LxSource/Model/Animation/DataType/LxCharacterAnimationTypes.h"
#include "LxARPG/LxSource/Model/Animation/Logic/LxCharacterAnimationMotionAnalysisComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterSpecialAttributeComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterMoveComponent::ULxCharacterMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void ULxCharacterMoveComponent::BaseComponentInitialize()
{
	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}
	if (m_pOwnerCharacter && m_pOwnerCharacter->GetCharacterMovement())
	{
		bWasFalling = m_pOwnerCharacter->GetCharacterMovement()->IsFalling();
	}
}

void ULxCharacterMoveComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshBaseAnimationMotionSignal();
}

void ULxCharacterMoveComponent::HandleMoveInput(const FVector2D& InMoveValue)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (!m_pOwnerCharacter || !m_pOwnerCharacter->Controller)
	{
		return;
	}

	if (const ULxCharacterSpecialAttributeComponent* SpecialAttributeComponent = m_pOwnerCharacter->GetCharacterSpecialAttributeComponent())
	{
		if (!SpecialAttributeComponent->IsCharacterAlive())
		{
			return;
		}
	}

	const FRotator ControlRotation = m_pOwnerCharacter->Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (InMoveValue.IsNearlyZero())
	{
		return;
	}

	m_pOwnerCharacter->AddMovementInput(ForwardDirection, InMoveValue.Y);
	m_pOwnerCharacter->AddMovementInput(RightDirection, InMoveValue.X);

	// 角色转向
	// 2️⃣ 没输入就不转向
	if (InMoveValue.IsNearlyZero()) return;
	if (!CanRotateByMoveInput()) return;

	// 3️⃣ 判断输入方向（四方向）
	float TargetYawOffset = 0.f;

	if (FMath::Abs(InMoveValue.Y) >= FMath::Abs(InMoveValue.X))
	{
		// 前后
		TargetYawOffset = InMoveValue.Y > 0 ? 0.f : 180.f;
	}
	else
	{
		// 左右
		TargetYawOffset = InMoveValue.X > 0 ? 90.f : -90.f;
	}

	// 4️⃣ 计算最终目标Yaw
	float TargetYaw = ControlRotation.Yaw + TargetYawOffset;

	FRotator TargetRot(0.f, TargetYaw, 0.f);

	// 5️⃣ 平滑旋转
	FRotator NewRot = FMath::RInterpTo(
		m_pOwnerCharacter->GetActorRotation(),
		TargetRot,
		GetWorld()->GetDeltaSeconds(),
		5.f // 转向速度
	);

	m_pOwnerCharacter->SetActorRotation(NewRot);
	if (!m_pOwnerCharacter->HasAuthority())
	{
		m_pOwnerCharacter->ServerSetCharacterRotation(NewRot);
	}
}

void ULxCharacterMoveComponent::HandleJumpInput(bool bPressed)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (!m_pOwnerCharacter)
	{
		return;
	}

	if (const ULxCharacterSpecialAttributeComponent* SpecialAttributeComponent = m_pOwnerCharacter->GetCharacterSpecialAttributeComponent())
	{
		if (!SpecialAttributeComponent->IsCharacterAlive())
		{
			return;
		}
	}

	if (bPressed)
	{
		m_pOwnerCharacter->Jump();
	}
	else
	{
		m_pOwnerCharacter->StopJumping();
	}
}

void ULxCharacterMoveComponent::HandleLookInput(const FVector2D& InMoveValue)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (!m_pOwnerCharacter || !m_pOwnerCharacter->Controller)
	{
		return;
	}
	m_pOwnerCharacter->AddControllerYawInput(InMoveValue.X);
	m_pOwnerCharacter->AddControllerPitchInput(InMoveValue.Y);
}

bool ULxCharacterMoveComponent::RequestMoveToActor(AActor* InTargetActor, const float InAcceptanceRadius)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	AAIController* AIController = m_pOwnerCharacter ? Cast<AAIController>(m_pOwnerCharacter->GetController()) : nullptr;
	if (!AIController || !IsValid(InTargetActor))
	{
		return false;
	}
	return AIController->MoveToActor(InTargetActor, InAcceptanceRadius, true, true, true, nullptr, true) !=
		EPathFollowingRequestResult::Failed;
}

bool ULxCharacterMoveComponent::RequestMoveToLocation(const FVector InTargetLocation, const float InAcceptanceRadius)
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	AAIController* AIController = m_pOwnerCharacter ? Cast<AAIController>(m_pOwnerCharacter->GetController()) : nullptr;
	if (!AIController)
	{
		return false;
	}
	return AIController->MoveToLocation(InTargetLocation, InAcceptanceRadius, true, true, false, true, nullptr, true) !=
		EPathFollowingRequestResult::Failed;
}

void ULxCharacterMoveComponent::StopActiveMovement()
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	if (AAIController* AIController = m_pOwnerCharacter ? Cast<AAIController>(m_pOwnerCharacter->GetController()) : nullptr)
	{
		AIController->StopMovement();
	}
	if (m_pOwnerCharacter)
	{
		m_pOwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
}

bool ULxCharacterMoveComponent::IsNavigationMoving() const
{
	const AAIController* AIController = m_pOwnerCharacter ? Cast<AAIController>(m_pOwnerCharacter->GetController()) : nullptr;
	return AIController && AIController->GetMoveStatus() == EPathFollowingStatus::Moving;
}

void ULxCharacterMoveComponent::RefreshBaseAnimationMotionSignal()
{
	if (!m_pOwnerCharacter)
	{
		BaseComponentInitialize();
	}
	UCharacterMovementComponent* MovementComponent = m_pOwnerCharacter ? m_pOwnerCharacter->GetCharacterMovement() : nullptr;
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
		MotionSignal.MotionType = ELxCharacterMotionType::JumpStart;
		MotionSignal.MotionDirection = FVector::UpVector;
		MotionSignal.bLoop = false;
	}
	else if (bWasFalling)
	{
		MotionSignal.MotionType = ELxCharacterMotionType::JumpEnd;
		MotionSignal.MotionDirection = FVector::DownVector;
		MotionSignal.bLoop = false;
	}
	else if (HorizontalSpeed > IdleSpeedThreshold)
	{
		MotionSignal.MotionType = ELxCharacterMotionType::Move;
		MotionSignal.MotionDirection = Velocity.GetSafeNormal2D();
	}
	else
	{
		MotionSignal.MotionType = ELxCharacterMotionType::Idle;
		MotionSignal.MotionDirection = FVector::ZeroVector;
		MotionSignal.MotionSpeed = 0.0f;
	}

	bWasFalling = bIsFalling;
	if (!ShouldSendBaseAnimationSignal(MotionSignal))
	{
		return;
	}

	if (m_pOwnerCharacter->HasAuthority() || m_pOwnerCharacter->IsLocallyControlled())
	{
		switch (MotionSignal.MotionType)
		{
		case ELxCharacterMotionType::Move:
			m_pOwnerCharacter->SetCharacterState(ELxCharacterState::Moving);
			break;
		case ELxCharacterMotionType::JumpStart:
			m_pOwnerCharacter->SetCharacterState(ELxCharacterState::JumpStart);
			break;
		case ELxCharacterMotionType::JumpEnd:
			m_pOwnerCharacter->SetCharacterState(ELxCharacterState::JumpEnd);
			break;
		default:
			m_pOwnerCharacter->SetCharacterState(ELxCharacterState::Idle);
			break;
		}
	}
	SendBaseAnimationMotionSignal(MotionSignal);
	LastBaseAnimationSignal = MotionSignal;
	bHasSentBaseAnimationSignal = true;
}

bool ULxCharacterMoveComponent::ShouldSendBaseAnimationSignal(const FLxCharacterMotionSignal& InSignal) const
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
		const float DirectionAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DirectionDot, -1.0f, 1.0f)));
		return DirectionAngle >= AnimationDirectionRefreshAngle;
	}
	return false;
}

void ULxCharacterMoveComponent::AddMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount) + 1;
}

void ULxCharacterMoveComponent::RemoveMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount - 1);
}

void ULxCharacterMoveComponent::SendBaseAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal) const
{
	if (!m_pOwnerCharacter)
	{
		return;
	}

	ULxCharacterAnimationMotionAnalysisComponent* MotionAnalysisComponent = m_pOwnerCharacter->GetCharacterAnimationMotionAnalysisComponent();
	if (!MotionAnalysisComponent)
	{
		return;
	}

	MotionAnalysisComponent->ReceiveBaseMotionEvent(InMotionSignal);
}

void ULxCharacterMoveComponent::SendActionAnimationMotionSignal(const FLxCharacterMotionSignal& InMotionSignal) const
{
	if (!m_pOwnerCharacter)
	{
		return;
	}

	ULxCharacterAnimationMotionAnalysisComponent* MotionAnalysisComponent = m_pOwnerCharacter->GetCharacterAnimationMotionAnalysisComponent();
	if (!MotionAnalysisComponent)
	{
		return;
	}

	MotionAnalysisComponent->ReceiveActionMotionEvent(InMotionSignal);
}
