#include "LxRaySkillUnitActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillPropagationComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ALxRaySkillUnitActor::ALxRaySkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
	PropagationComponent = CreateDefaultSubobject<ULxSkillPropagationComponent>(TEXT("PropagationComponent"));
}

void ALxRaySkillUnitActor::InitializeRayDetectionCollisionComponents()
{
	RayDetectionCollisionComponents.Reset();
	for (UPrimitiveComponent* CollisionComponent : ResolveRayDetectionCollisionComponents())
	{
		RayDetectionCollisionComponents.Add(CollisionComponent);
	}
}

void ALxRaySkillUnitActor::InitializeRayParameters(const FLxSkillRaySpec& InRaySpec)
{
	RaySpec = InRaySpec;
}

FLxSkillDetectionResult ALxRaySkillUnitActor::PerformRayDetection()
{
	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::ManualScan;
	DetectionResult.SourceComponent = TriggerComponent;
	DetectionResult.SourceUnit = this;

	if (RayDetectionCollisionComponents.IsEmpty())
	{
		OnRayDetectionCompleted.Broadcast(this, DetectionResult);
		return DetectionResult;
	}

	TArray<AActor*> OverlappingActors;
	for (UPrimitiveComponent* CollisionComponent : RayDetectionCollisionComponents)
	{
		if (!IsValid(CollisionComponent))
		{
			continue;
		}
		// 刚创建或刚更新变换时主动刷新重叠，保证首帧扫描能发现已在判定体内的目标。
		CollisionComponent->UpdateOverlaps();
		TArray<AActor*> ComponentOverlappingActors;
		CollisionComponent->GetOverlappingActors(ComponentOverlappingActors, ALxBaseCharacter::StaticClass());
		for (AActor* OverlappingActor : ComponentOverlappingActors)
		{
			OverlappingActors.AddUnique(OverlappingActor);
		}
	}

	TArray<AActor*> ValidTargets;
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (!IsValidRayTarget(OverlappingActor) || OverlappingActor == GetOwner() || OverlappingActor == GetInstigator())
		{
			continue;
		}
		ValidTargets.AddUnique(OverlappingActor);
	}

	// 未启用穿过障碍物时，仅保留与射线起点之间不存在场景阻挡的目标。
	TArray<AActor*> UnblockedTargets;
	FHitResult NearestObstacleHit;
	for (AActor* ValidTarget : ValidTargets)
	{
		FHitResult ObstacleHit;
		if (!RaySpec.bPassThroughObstacles
			&& FindObstacleBeforeTarget(ValidTarget, ValidTargets, ObstacleHit))
		{
			if (!NearestObstacleHit.bBlockingHit || ObstacleHit.Distance < NearestObstacleHit.Distance)
			{
				NearestObstacleHit = ObstacleHit;
			}
			continue;
		}
		UnblockedTargets.Add(ValidTarget);
	}

	UnblockedTargets.Sort([this](const AActor& Left, const AActor& Right)
	{
		return FVector::DistSquared(GetActorLocation(), Left.GetActorLocation())
			< FVector::DistSquared(GetActorLocation(), Right.GetActorLocation());
	});
	if (!RaySpec.bPassThroughTargets && UnblockedTargets.Num() > 1)
	{
		UnblockedTargets.SetNum(1);
	}

	for (AActor* TargetActor : UnblockedTargets)
	{
		DetectionResult.CandidateTargets.AddUnique(TargetActor);
		if (!DetectionResult.HitActor)
		{
			DetectionResult.EventType = ELxSkillDetectionEventType::HitTarget;
			DetectionResult.HitActor = TargetActor;
			DetectionResult.HitLocation = TargetActor->GetActorLocation();
			DetectionResult.HitNormal = (GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
		}
	}
	if (DetectionResult.CandidateTargets.IsEmpty())
	{
		if (NearestObstacleHit.bBlockingHit)
		{
			DetectionResult.EventType = ELxSkillDetectionEventType::HitWorld;
			DetectionResult.bHitWorld = true;
			DetectionResult.HitLocation = NearestObstacleHit.ImpactPoint;
			DetectionResult.HitNormal = NearestObstacleHit.ImpactNormal;
		}
		else
		{
			DetectionResult.HitLocation = GetActorLocation();
		}
	}

	OnRayDetectionCompleted.Broadcast(this, DetectionResult);
	if (TriggerComponent)
	{
		TriggerComponent->HandleDetectionResult(DetectionResult);
	}
	return DetectionResult;
}

TArray<UPrimitiveComponent*> ALxRaySkillUnitActor::ResolveRayDetectionCollisionComponents() const
{
	TArray<UPrimitiveComponent*> Result;
	TInlineComponentArray<UCapsuleComponent*> CapsuleComponents(this);
	for (UCapsuleComponent* CapsuleComponent : CapsuleComponents)
	{
		if (IsValid(CapsuleComponent) && CapsuleComponent->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		{
			Result.Add(CapsuleComponent);
		}
	}
	return Result;
}

bool ALxRaySkillUnitActor::IsValidRayTarget(AActor* InActor) const
{
	return InActor && InActor->IsA<ALxBaseCharacter>();
}

bool ALxRaySkillUnitActor::FindObstacleBeforeTarget(AActor* TargetActor, const TArray<AActor*>& RayTargets,
	FHitResult& OutObstacleHit) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(TargetActor))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LxRayObstacleCheck), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(GetInstigator());
	// 目标穿透由独立参数处理，因此障碍检测需要忽略当前射线胶囊内的全部有效目标。
	QueryParams.AddIgnoredActors(RayTargets);
	return World->LineTraceSingleByChannel(OutObstacleHit, GetActorLocation(), TargetActor->GetActorLocation(),
		ECC_Visibility, QueryParams) && OutObstacleHit.bBlockingHit;
}

void ALxRaySkillUnitActor::HandleSkillTriggered(const FLxSkillTriggerResult& TriggerResult)
{
	Super::HandleSkillTriggered(TriggerResult);
}
