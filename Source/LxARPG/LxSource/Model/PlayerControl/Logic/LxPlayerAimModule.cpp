#include "LxPlayerAimModule.h"

#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxPlayerCharacter.h"

void ULxPlayerAimModule::InitializeModule(ULxPlayerControlComponent* InOwnerComponent)
{
	Super::InitializeModule(InOwnerComponent);
	CacheOwnerReferences();
	RegisterInputActionReceive(AimInputActionID);
}

void ULxPlayerAimModule::ShutdownModule()
{
	SetAiming(false);
	AimResultUpdateRequestCount = 0;
	OwnerPlayerCharacter = nullptr;
	CachedCameraBoom = nullptr;
	CachedFollowCamera = nullptr;
	Super::ShutdownModule();
}

void ULxPlayerAimModule::TickModule(float DeltaTime)
{
	if (!OwnerPlayerCharacter)
	{
		CacheOwnerReferences();
	}

	if (!OwnerPlayerCharacter || !OwnerPlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	UpdateAimCamera(DeltaTime);

	if (bIsAiming || AimResultUpdateRequestCount > 0)
	{
		FLxPlayerAimResult NewAimResult;
		if (CalculateAimResult(NewAimResult))
		{
			const bool bTransformChanged = !CurrentAimResult.ReleaseLocation.Equals(NewAimResult.ReleaseLocation, 0.1f)
				|| !CurrentAimResult.SkillDirection.Equals(NewAimResult.SkillDirection, 0.0001f);
			CurrentAimResult = NewAimResult;
			if (bTransformChanged)
			{
				OnAimResultChanged.Broadcast(CurrentAimResult);
			}
			if (bIsAiming)
			{
				UpdateAimRotation(DeltaTime);
			}
		}
	}
}

void ULxPlayerAimModule::HandleInputValue(ELxInputActionID InInputActionID, FLxInputValue InValue)
{
	if (InInputActionID == AimInputActionID)
	{
		SetAiming(InValue.m_blValue);
	}
}

void ULxPlayerAimModule::SetAiming(bool bNewAiming)
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
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerPlayerCharacter->GetCharacterBehaviorControlComponent())
		{
			if (bIsAiming)
			{
				BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Aiming);
				BehaviorControlComponent->AddFacingControlRequest();
			}
			else
			{
				BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Aiming);
				BehaviorControlComponent->RemoveFacingControlRequest();
			}
		}
	}

	if (bIsAiming)
	{
		CalculateAimResult(CurrentAimResult);
	}

	OnAimingStateChanged.Broadcast(bIsAiming);
	BroadcastModuleDataChanged();
}

bool ULxPlayerAimModule::CalculateAimResult(FLxPlayerAimResult& OutAimResult) const
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

FLxSkillCastContext ULxPlayerAimModule::MakeAimSkillCastContext(UObject* SourceObject)
{
	const ALxPlayerCharacter* PlayerCharacter = OwnerPlayerCharacter.Get();
	if (!PlayerCharacter)
	{
		PlayerCharacter = Cast<ALxPlayerCharacter>(GetOwner());
	}

	FLxSkillCastContext CastContext;
	CastContext.WorldContextObject = const_cast<ULxPlayerAimModule*>(this);
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

void ULxPlayerAimModule::AddAimResultUpdateRequest()
{
	AimResultUpdateRequestCount = FMath::Max(0, AimResultUpdateRequestCount) + 1;
}

void ULxPlayerAimModule::RemoveAimResultUpdateRequest()
{
	AimResultUpdateRequestCount = FMath::Max(0, AimResultUpdateRequestCount - 1);
}

FVector ULxPlayerAimModule::GetSkillReleasePoint() const
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

	return PlayerCharacter->GetSkillReleaseAnchorTransform().GetLocation();

}

void ULxPlayerAimModule::CacheOwnerReferences()
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

bool ULxPlayerAimModule::ResolveCameraRay(FVector& OutRayStart, FVector& OutRayDirection) const
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

void ULxPlayerAimModule::UpdateAimCamera(float DeltaTime)
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

void ULxPlayerAimModule::UpdateAimRotation(float DeltaTime)
{
	if (!bRotateCharacterWhileAiming || !OwnerPlayerCharacter)
	{
		return;
	}

	RotateCharacterToAimResult(CurrentAimResult, false, DeltaTime);
}

void ULxPlayerAimModule::RotateCharacterToAimResult(const FLxPlayerAimResult& InAimResult, bool bInstantRotation, float DeltaTime)
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
	if (!InAimResult.CameraRayDirection.IsNearlyZero())
	{
		ToAimLocation = InAimResult.CameraRayDirection;
	}
	else if (!InAimResult.SkillDirection.IsNearlyZero())
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

	if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
		OwnerPlayerCharacter->GetCharacterBehaviorControlComponent())
	{
		BehaviorControlComponent->SetDesiredFacingDirection(ToAimLocation);
		if (bInstantRotation && !bIsAiming)
		{
			BehaviorControlComponent->RefreshFacingControl();
		}
	}
}

