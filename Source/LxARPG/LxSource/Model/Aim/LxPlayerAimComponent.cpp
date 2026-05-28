#include "LxPlayerAimComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "LxARPG/LxSource/Model/CharacterMove/LxCharacterMoveComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

ULxPlayerAimComponent::ULxPlayerAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void ULxPlayerAimComponent::BaseComponentInitialize()
{
	CacheOwnerReferences();
	RegisterInputActionReceive(AimInputActionID);
}

void ULxPlayerAimComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!OwnerPlayerCharacter)
	{
		CacheOwnerReferences();
	}

	if (!OwnerPlayerCharacter || !OwnerPlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	UpdateAimCamera(DeltaTime);

	if (bIsAiming && CalculateAimResult(CurrentAimResult))
	{
		UpdateAimRotation(DeltaTime);
	}
}

void ULxPlayerAimComponent::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (InInputActionID == AimInputActionID)
	{
		SetAiming(InValue.m_blValue);
	}
}

void ULxPlayerAimComponent::SetAiming(bool bNewAiming)
{
	if (bIsAiming == bNewAiming)
	{
		return;
	}

	bIsAiming = bNewAiming;
	if (!OwnerPlayerCharacter)
	{
		CacheOwnerReferences();
	}

	if (OwnerPlayerCharacter)
	{
		if (ULxCharacterMoveComponent* MoveComponent = OwnerPlayerCharacter->GetCharacterMoveComponent())
		{
			if (bIsAiming)
			{
				MoveComponent->AddMoveRotationLock();
			}
			else
			{
				MoveComponent->RemoveMoveRotationLock();
			}
		}
	}

	if (bIsAiming)
	{
		CalculateAimResult(CurrentAimResult);
	}

	OnAimingStateChanged.Broadcast(bIsAiming);
	OnDataChange.Broadcast();
}

bool ULxPlayerAimComponent::CalculateAimResult(FLxPlayerAimResult& OutAimResult) const
{
	OutAimResult = FLxPlayerAimResult();

	FVector RayStart;
	FVector RayDirection;
	if (!ResolveCameraRay(RayStart, RayDirection))
	{
		return false;
	}

	OutAimResult.CameraRayStart = RayStart;
	OutAimResult.CameraRayDirection = RayDirection;

	const FVector RayEnd = RayStart + RayDirection * MaxAimDistance;
	const UWorld* World = GetWorld();
	if (!World)
	{
		OutAimResult.AimLocation = RayEnd;
	}
	else
	{
		FHitResult HitResult;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LxPlayerAimTrace), true);
		if (const ALxPlayerCharacter* PlayerCharacter = OwnerPlayerCharacter.Get())
		{
			QueryParams.AddIgnoredActor(PlayerCharacter);
		}

		const bool bHit = World->LineTraceSingleByChannel(
			HitResult,
			RayStart,
			RayEnd,
			AimTraceChannel,
			QueryParams);

		OutAimResult.bBlockingHit = bHit;
		OutAimResult.AimLocation = bHit ? HitResult.ImpactPoint : RayEnd;
		OutAimResult.TargetActor = bHit ? HitResult.GetActor() : nullptr;
	}

	OutAimResult.ReleaseLocation = GetSkillReleasePoint();
	OutAimResult.SkillDirection = (OutAimResult.AimLocation - OutAimResult.ReleaseLocation).GetSafeNormal();
	if (OutAimResult.SkillDirection.IsNearlyZero())
	{
		OutAimResult.SkillDirection = RayDirection;
	}

	return true;
}

FLxSkillCastContext ULxPlayerAimComponent::MakeAimSkillCastContext(UObject* SourceObject)
{
	const ALxPlayerCharacter* PlayerCharacter = OwnerPlayerCharacter.Get();
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	}

	FLxSkillCastContext CastContext;
	CastContext.WorldContextObject = const_cast<ULxPlayerAimComponent*>(this);
	CastContext.CasterActor = const_cast<ALxPlayerCharacter*>(PlayerCharacter);
	CastContext.SourceObject = SourceObject;

	if (PlayerCharacter)
	{
		CastContext.InstigatorController = PlayerCharacter->GetController();
	}

	FLxPlayerAimResult AimResult;
	if (CalculateAimResult(AimResult))
	{
		if (bRotateCharacterOnSkillCast)
		{
			RotateCharacterToAimResult(AimResult, true);
			AimResult.ReleaseLocation = GetSkillReleasePoint();
			AimResult.SkillDirection = (AimResult.AimLocation - AimResult.ReleaseLocation).GetSafeNormal();
			if (AimResult.SkillDirection.IsNearlyZero())
			{
				AimResult.SkillDirection = AimResult.CameraRayDirection;
			}
		}

		CastContext.TargetActor = AimResult.TargetActor;
		CastContext.AimLocation = AimResult.AimLocation;
		CastContext.bHasAimLocation = true;
		CastContext.AimDirection = AimResult.SkillDirection;
		CastContext.bHasAimDirection = true;
		CastContext.SpawnTransform = FTransform(AimResult.SkillDirection.Rotation(), AimResult.ReleaseLocation);
	}
	else
	{
		const FVector ReleaseLocation = GetSkillReleasePoint();
		const FVector FallbackDirection = PlayerCharacter
			? PlayerCharacter->GetActorForwardVector()
			: FVector::ForwardVector;

		CastContext.AimDirection = FallbackDirection;
		CastContext.bHasAimDirection = true;
		CastContext.SpawnTransform = FTransform(FallbackDirection.Rotation(), ReleaseLocation);

		if (bRotateCharacterOnSkillCast && PlayerCharacter)
		{
			FLxPlayerAimResult FallbackAimResult;
			FallbackAimResult.ReleaseLocation = ReleaseLocation;
			FallbackAimResult.AimLocation = ReleaseLocation + FallbackDirection * MaxAimDistance;
			FallbackAimResult.SkillDirection = FallbackDirection;
			RotateCharacterToAimResult(FallbackAimResult, true);
		}
	}

	return CastContext;
}

FVector ULxPlayerAimComponent::GetSkillReleasePoint() const
{
	const ALxPlayerCharacter* PlayerCharacter = OwnerPlayerCharacter.Get();
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	}

	if (!PlayerCharacter)
	{
		return FVector::ZeroVector;
	}

	if (!SkillReleaseSocketName.IsNone())
	{
		if (const USkeletalMeshComponent* MeshComponent = PlayerCharacter->GetMesh())
		{
			if (MeshComponent->DoesSocketExist(SkillReleaseSocketName))
			{
				return MeshComponent->GetSocketLocation(SkillReleaseSocketName);
			}
		}
	}

	return PlayerCharacter->GetActorLocation()
		+ PlayerCharacter->GetActorForwardVector() * SkillReleaseForwardOffset
		+ FVector::UpVector * SkillReleaseHeightOffset;
}

void ULxPlayerAimComponent::CacheOwnerReferences()
{
	OwnerPlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	if (!OwnerPlayerCharacter)
	{
		CachedCameraBoom = nullptr;
		CachedFollowCamera = nullptr;
		return;
	}

	CachedCameraBoom = OwnerPlayerCharacter->GetCameraBoom();
	CachedFollowCamera = OwnerPlayerCharacter->GetFollowCamera();

	if (CachedCameraBoom && NormalCameraArmLength <= 0.f)
	{
		NormalCameraArmLength = CachedCameraBoom->TargetArmLength;
		NormalCameraSocketOffset = CachedCameraBoom->SocketOffset;
	}
}

bool ULxPlayerAimComponent::ResolveCameraRay(FVector& OutRayStart, FVector& OutRayDirection) const
{
	const ALxPlayerCharacter* PlayerCharacter = OwnerPlayerCharacter.Get();
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	}

	if (!PlayerCharacter)
	{
		return false;
	}

	if (const APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
	{
		int32 ViewportSizeX = 0;
		int32 ViewportSizeY = 0;
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
		if (ViewportSizeX > 0 && ViewportSizeY > 0)
		{
			if (PlayerController->DeprojectScreenPositionToWorld(
				ViewportSizeX * 0.5f,
				ViewportSizeY * 0.5f,
				OutRayStart,
				OutRayDirection))
			{
				OutRayDirection = OutRayDirection.GetSafeNormal();
				return !OutRayDirection.IsNearlyZero();
			}
		}
	}

	const UCameraComponent* CameraComponent = CachedFollowCamera.Get();
	if (!CameraComponent)
	{
		CameraComponent = PlayerCharacter->GetFollowCamera();
	}

	if (!CameraComponent)
	{
		return false;
	}

	OutRayStart = CameraComponent->GetComponentLocation();
	OutRayDirection = CameraComponent->GetForwardVector().GetSafeNormal();
	return !OutRayDirection.IsNearlyZero();
}

void ULxPlayerAimComponent::UpdateAimCamera(float DeltaTime)
{
	if (!bAdjustCameraWhileAiming)
	{
		return;
	}

	if (!CachedCameraBoom)
	{
		CacheOwnerReferences();
	}

	if (!CachedCameraBoom)
	{
		return;
	}

	if (NormalCameraArmLength <= 0.f)
	{
		NormalCameraArmLength = CachedCameraBoom->TargetArmLength;
		NormalCameraSocketOffset = CachedCameraBoom->SocketOffset;
	}

	const float TargetArmLength = bIsAiming ? AimCameraArmLength : NormalCameraArmLength;
	const FVector TargetSocketOffset = bIsAiming ? AimCameraSocketOffset : NormalCameraSocketOffset;

	CachedCameraBoom->TargetArmLength = FMath::FInterpTo(
		CachedCameraBoom->TargetArmLength,
		TargetArmLength,
		DeltaTime,
		AimCameraInterpSpeed);
	CachedCameraBoom->SocketOffset = FMath::VInterpTo(
		CachedCameraBoom->SocketOffset,
		TargetSocketOffset,
		DeltaTime,
		AimCameraInterpSpeed);
}

void ULxPlayerAimComponent::UpdateAimRotation(float DeltaTime)
{
	if (!bRotateCharacterWhileAiming || !OwnerPlayerCharacter)
	{
		return;
	}

	RotateCharacterToAimResult(CurrentAimResult, false, DeltaTime);
}

void ULxPlayerAimComponent::RotateCharacterToAimResult(const FLxPlayerAimResult& InAimResult, bool bInstantRotation, float DeltaTime)
{
	if (!OwnerPlayerCharacter)
	{
		CacheOwnerReferences();
	}

	if (!OwnerPlayerCharacter)
	{
		return;
	}

	FVector ToAimLocation = FVector::ZeroVector;
	if (!InAimResult.SkillDirection.IsNearlyZero())
	{
		ToAimLocation = InAimResult.SkillDirection;
	}
	else
	{
		ToAimLocation = InAimResult.AimLocation - OwnerPlayerCharacter->GetActorLocation();
	}

	ToAimLocation.Z = 0.f;
	if (ToAimLocation.IsNearlyZero())
	{
		return;
	}

	const FRotator TargetRotation(0.f, ToAimLocation.Rotation().Yaw, 0.f);
	if (DeltaTime <= 0.f)
	{
		DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;
	}

	const FRotator NewRotation = bInstantRotation
		? TargetRotation
		: FMath::RInterpTo(
			OwnerPlayerCharacter->GetActorRotation(),
			TargetRotation,
			DeltaTime,
			AimTurnSpeed);

	OwnerPlayerCharacter->SetActorRotation(NewRotation);
	if (!OwnerPlayerCharacter->HasAuthority())
	{
		OwnerPlayerCharacter->ServerSetCharacterRotation(NewRotation);
	}
}
