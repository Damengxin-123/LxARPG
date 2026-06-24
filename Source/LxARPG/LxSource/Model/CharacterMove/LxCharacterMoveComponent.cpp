#include "LxCharacterMoveComponent.h"

#include "GameFramework/Controller.h"
#include "LxARPG/LxSource/Model/Lifecycle/Logic/LxCharacterLifecycleComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterMoveComponent::ULxCharacterMoveComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterMoveComponent::BaseComponentInitialize()
{
	if (!m_pOwnerCharacter)
	{
		m_pOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}
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

	if (const ULxCharacterLifecycleComponent* LifecycleComponent = m_pOwnerCharacter->GetCharacterLifecycleComponent())
	{
		if (!LifecycleComponent->IsCharacterAlive())
		{
			return;
		}
	}

	const FRotator ControlRotation = m_pOwnerCharacter->Controller->GetControlRotation();
	const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	m_pOwnerCharacter->SetCharacterState(ELxCharacterState::Moving);
	
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

	if (const ULxCharacterLifecycleComponent* LifecycleComponent = m_pOwnerCharacter->GetCharacterLifecycleComponent())
	{
		if (!LifecycleComponent->IsCharacterAlive())
		{
			return;
		}
	}

	if (bPressed)
	{
		m_pOwnerCharacter->Jump();
		m_pOwnerCharacter->SetCharacterState(ELxCharacterState::JumpStart);
	}
	else
	{
		m_pOwnerCharacter->StopJumping();
		m_pOwnerCharacter->SetCharacterState(ELxCharacterState::JumpEnd);
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

void ULxCharacterMoveComponent::AddMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount) + 1;
}

void ULxCharacterMoveComponent::RemoveMoveRotationLock()
{
	MoveRotationLockCount = FMath::Max(0, MoveRotationLockCount - 1);
}
