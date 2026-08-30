#include "LxCharacterLocomotionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxCharacterLocomotionComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	RefreshRuntimeBodyNavigation();
}

void ULxCharacterLocomotionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ALxBaseCharacter* LocalOwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
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
