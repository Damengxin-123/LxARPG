#include "LxCharacterLocomotionComponent.h"
#include "Net/UnrealNetwork.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterBaseAttributeSet.h"
#include "LxARPG/LxSource/Model/Tags/LxAttributeEntryTags.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxCharacterLocomotionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	RefreshRuntimeBodyNavigation();
	RefreshMovementSettings();
}

void ULxCharacterLocomotionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ALxBaseCharacter* LocalOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (bSprintRequested && LocalOwnerCharacter && LocalOwnerCharacter->HasAuthority()
		&& (!LocalOwnerCharacter->IsPlayerControlled() || (LocalOwnerCharacter->GetCharacterAttributeComponent()
			&& !LocalOwnerCharacter->GetCharacterAttributeComponent()->IsCharacterAlive())))
	{
		SetSprintRequested(false);
	}
	const UCapsuleComponent* CapsuleComponent = LocalOwnerCharacter ? LocalOwnerCharacter->GetCapsuleComponent() : nullptr;
	if (!LocalOwnerCharacter || !CapsuleComponent)
	{
		return;
	}

	float CurrentRadius = 0.0f;
	float CurrentHalfHeight = 0.0f;
	CapsuleComponent->GetScaledCapsuleSize(CurrentRadius, CurrentHalfHeight);
	const bool bOwnerScaleChanged = !LocalOwnerCharacter->GetActorScale3D().Equals(
		LastNavigationOwnerScale, KINDA_SMALL_NUMBER);
	const bool bCapsuleSizeChanged = !FMath::IsNearlyEqual(CurrentRadius, RuntimeNavigationAgentRadius) ||
		!FMath::IsNearlyEqual(CurrentHalfHeight * 2.0f, RuntimeNavigationAgentHeight);
	if (bOwnerScaleChanged || bCapsuleSizeChanged)
	{
		RefreshRuntimeBodyNavigation();
	}
}

void ULxCharacterLocomotionComponent::RefreshRuntimeBodyNavigation()
{
	ALxBaseCharacter* LocalOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	if (!LocalOwnerCharacter)
	{
		return;
	}

	LastNavigationOwnerScale = LocalOwnerCharacter->GetActorScale3D();
	if (const USkeletalMeshComponent* MeshComponent = LocalOwnerCharacter->GetMesh())
	{
		const FBoxSphereBounds MeshBounds = MeshComponent->CalcBounds(MeshComponent->GetComponentTransform());
		RuntimeMeshBoundsSize = MeshBounds.BoxExtent * 2.0f;
	}
	else
	{
		RuntimeMeshBoundsSize = FVector::ZeroVector;
	}

	UCapsuleComponent* CapsuleComponent = LocalOwnerCharacter->GetCapsuleComponent();
	if (!CapsuleComponent)
	{
		RuntimeNavigationAgentRadius = 0.0f;
		RuntimeNavigationAgentHeight = 0.0f;
		return;
	}

	float CapsuleHalfHeight = 0.0f;
	CapsuleComponent->GetScaledCapsuleSize(RuntimeNavigationAgentRadius, CapsuleHalfHeight);
	RuntimeNavigationAgentHeight = CapsuleHalfHeight * 2.0f;
	if (UCharacterMovementComponent* MovementComponent = LocalOwnerCharacter->GetCharacterMovement())
	{
		// 角色在场景中的整体缩放可能不同于蓝图默认值，确保导航代理使用缩放后的实际胶囊体尺寸。
		MovementComponent->UpdateNavAgent(*CapsuleComponent);
	}
}

void ULxCharacterLocomotionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ULxCharacterLocomotionComponent, bSprintRequested);
	DOREPLIFETIME(ULxCharacterLocomotionComponent, MovementConfig);
}

bool ULxCharacterLocomotionComponent::SetMovementConfig(const FLxAIMovementConfig& InConfig)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	FText Error;
	if (!InConfig.ValidateConfiguration(Error)) return false;
	MovementConfig = InConfig;
	RefreshMovementSettings();
	return true;
}

ELxCharacterMotionType ULxCharacterLocomotionComponent::GetRequestedMovementGait() const
{
	const ELxCharacterMotionType Motion = GetRequestedBehaviorMotionType();
	if (Motion == ELxCharacterMotionType::None)
		return bSprintRequested ? ELxCharacterMotionType::MediumMove : ELxCharacterMotionType::Move;
	return Motion == ELxCharacterMotionType::Move || Motion == ELxCharacterMotionType::Run
		? Motion : ELxCharacterMotionType::MediumMove;
}

float ULxCharacterLocomotionComponent::GetMovementSpeedMultiplier() const
{
	return MovementConfig.GetSpeedMultiplier(GetRequestedMovementGait());
}

float ULxCharacterLocomotionComponent::GetHighSpeedThreshold() const
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	const auto* Attributes = Character ? Character->GetCharacterAttributeComponent() : nullptr;
	const auto* Values = Attributes ? Attributes->GetRuntimeAttributeSet() : nullptr;
	FLxScalarAttributeData BaseSpeed;
	const float BaseMeters = Values && Values->GetScalarAttribute(LxTag_Attribute_Action_BaseMovementSpeed, BaseSpeed)
		? BaseSpeed.Value : 6.0f;
	// 阈值不乘速度加成，否则Buff加速时阈值也同步抬高，将无法触发高速动画。
	return FMath::Max(0.0f, BaseMeters) * 100.0f * MovementConfig.GetSpeedMultiplier(ELxCharacterMotionType::Run);
}

ELxCharacterMotionType ULxCharacterLocomotionComponent::ResolveGroundMotionType(float HorizontalSpeed) const
{
	if (HorizontalSpeed > GetHighSpeedThreshold()) return ELxCharacterMotionType::Run;
	return GetRequestedMovementGait();
}

void ULxCharacterLocomotionComponent::RefreshMovementSettings()
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	if (Character && Character->GetCharacterAttributeComponent())
		Character->GetCharacterAttributeComponent()->RefreshCharacterMovementSpeed();
	RefreshBaseBehaviorState();
}

void ULxCharacterLocomotionComponent::SetSprintRequested(bool bRequested)
{
	const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner());
	if (!Character || (!Character->HasAuthority() && !Character->IsLocallyControlled())) return;
	if (bRequested && Character->GetCharacterAttributeComponent()
		&& !Character->GetCharacterAttributeComponent()->IsCharacterAlive()) bRequested = false;
	if (bSprintRequested == bRequested) return;
	bSprintRequested = bRequested;
	OnRep_SprintRequested();
	if (!Character->HasAuthority()) ServerSetSprintRequested(bRequested);
}

void ULxCharacterLocomotionComponent::ServerSetSprintRequested_Implementation(bool bRequested)
{
	SetSprintRequested(bRequested);
}

void ULxCharacterLocomotionComponent::OnRep_SprintRequested()
{
	RefreshMovementSettings();
}
