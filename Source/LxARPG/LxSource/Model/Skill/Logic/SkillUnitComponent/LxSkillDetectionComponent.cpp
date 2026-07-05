#include "LxSkillDetectionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

void ULxSkillDetectionComponent::SetTargetFilterSpec(const FLxSkillTargetFilterSpec& InTargetFilterSpec)
{
	TargetFilterSpec = InTargetFilterSpec;
	OnDataChange.Broadcast();
}

void ULxSkillDetectionComponent::SetTriggerCollisionComponent(UPrimitiveComponent* InTriggerCollisionComponent)
{
	TArray<UPrimitiveComponent*> CollisionComponents;
	if (IsValid(InTriggerCollisionComponent))
	{
		CollisionComponents.Add(InTriggerCollisionComponent);
	}
	SetTriggerCollisionComponents(CollisionComponents);
}

void ULxSkillDetectionComponent::SetTriggerCollisionComponents(
	const TArray<UPrimitiveComponent*>& InTriggerCollisionComponents)
{
	const bool bRestartDetection = bDetecting;
	if (bRestartDetection)
	{
		StopDetection();
	}

	TriggerCollisionComponents.Reset();
	for (UPrimitiveComponent* CollisionComponent : InTriggerCollisionComponents)
	{
		if (IsValid(CollisionComponent))
		{
			TriggerCollisionComponents.AddUnique(CollisionComponent);
		}
	}
	if (bRestartDetection)
	{
		StartDetection();
	}
}

void ULxSkillDetectionComponent::SetPublishWorldHit(bool bInPublishWorldHit)
{
	bPublishWorldHit = bInPublishWorldHit;
	OnDataChange.Broadcast();
}

void ULxSkillDetectionComponent::StartDetection()
{
	if (TriggerCollisionComponents.IsEmpty() || bDetecting)
	{
		return;
	}

	for (UPrimitiveComponent* CollisionComponent : TriggerCollisionComponents)
	{
		if (!IsValid(CollisionComponent))
		{
			continue;
		}
		CollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleEndOverlap);
		CollisionComponent->OnComponentHit.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleComponentHit);
	}
	bDetecting = true;
}

void ULxSkillDetectionComponent::StopDetection()
{
	if (!bDetecting)
	{
		return;
	}

	for (UPrimitiveComponent* CollisionComponent : TriggerCollisionComponents)
	{
		if (!IsValid(CollisionComponent))
		{
			continue;
		}
		CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleBeginOverlap);
		CollisionComponent->OnComponentEndOverlap.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleEndOverlap);
		CollisionComponent->OnComponentHit.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleComponentHit);
	}
	CurrentCandidateTargets.Reset();
	bDetecting = false;
}

void ULxSkillDetectionComponent::PublishManualDetectionResult(const TArray<AActor*>& InCandidateTargets)
{
	FLxSkillDetectionResult Result;
	Result.EventType = ELxSkillDetectionEventType::ManualScan;
	Result.SourceComponent = this;
	Result.SourceUnit = GetOwner();

	for (AActor* CandidateTarget : InCandidateTargets)
	{
		if (IsTargetCandidateValid(CandidateTarget))
		{
			Result.CandidateTargets.Add(CandidateTarget);
		}
	}

	OnDetectionResult.Broadcast(Result);
}

TArray<AActor*> ULxSkillDetectionComponent::GetCurrentCandidateTargets() const
{
	TArray<AActor*> Result;
	for (AActor* CandidateTarget : CurrentCandidateTargets)
	{
		if (CandidateTarget)
		{
			Result.Add(CandidateTarget);
		}
	}
	return Result;
}

void ULxSkillDetectionComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	if (!IsBasicActorValid(OtherActor))
	{
		return;
	}

	const bool bIsTarget = IsTargetCandidateValid(OtherActor);
	// 非 Sweep 方式产生的 Overlap 通常没有有效 ImpactPoint，此时使用触发碰撞体的实时位置。
	const FVector HitLocation = bFromSweep
		? FVector(SweepResult.ImpactPoint)
		: (OverlappedComponent ? OverlappedComponent->GetComponentLocation() : OtherActor->GetActorLocation());
	FVector HitNormal = SweepResult.ImpactNormal;
	if (!bFromSweep && HitNormal.IsNearlyZero() && OverlappedComponent)
	{
		HitNormal = (OverlappedComponent->GetComponentLocation() - OtherActor->GetActorLocation()).GetSafeNormal();
	}
	if (bIsTarget)
	{
		CurrentCandidateTargets.AddUnique(OtherActor);
		PublishSingleActorResult(ELxSkillDetectionEventType::OverlapBegin, OtherActor, OverlappedComponent,
			HitLocation, HitNormal, false);
		return;
	}

	if (bPublishWorldHit)
	{
		PublishSingleActorResult(ELxSkillDetectionEventType::HitWorld, OtherActor, OverlappedComponent,
			HitLocation, HitNormal, true);
	}
}

void ULxSkillDetectionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!IsTargetCandidateValid(OtherActor))
	{
		return;
	}

	for (UPrimitiveComponent* CollisionComponent : TriggerCollisionComponents)
	{
		if (IsValid(CollisionComponent) && CollisionComponent != OverlappedComponent
			&& CollisionComponent->IsOverlappingActor(OtherActor))
		{
			return;
		}
	}
	CurrentCandidateTargets.Remove(OtherActor);
	PublishSingleActorResult(ELxSkillDetectionEventType::OverlapEnd, OtherActor, OverlappedComponent,
		OtherActor->GetActorLocation(), FVector::ZeroVector, false);
}

void ULxSkillDetectionComponent::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (ShouldIgnoreActor(OtherActor))
	{
		return;
	}

	if (IsTargetCandidateValid(OtherActor))
	{
		PublishSingleActorResult(ELxSkillDetectionEventType::HitTarget, OtherActor, HitComponent,
			Hit.ImpactPoint, Hit.ImpactNormal, false);
		return;
	}

	if (bPublishWorldHit)
	{
		PublishSingleActorResult(ELxSkillDetectionEventType::HitWorld, OtherActor, HitComponent,
			Hit.ImpactPoint, Hit.ImpactNormal, true);
	}
}

bool ULxSkillDetectionComponent::IsBasicActorValid(AActor* InActor) const
{
	return InActor && InActor != GetOwner() && !InActor->IsPendingKillPending();
}

bool ULxSkillDetectionComponent::IsTargetCandidateValid(AActor* InActor) const
{
	return IsBasicActorValid(InActor) && InActor->IsA<ALxBaseCharacter>();
}

bool ULxSkillDetectionComponent::ShouldIgnoreActor(AActor* InActor) const
{
	return InActor && InActor->IsA<ALxSkillUnitActor>();
}

void ULxSkillDetectionComponent::PublishSingleActorResult(ELxSkillDetectionEventType EventType, AActor* InActor,
	UPrimitiveComponent* InTriggerCollision, const FVector& HitLocation, const FVector& HitNormal, bool bHitWorld)
{
	FLxSkillDetectionResult Result;
	Result.EventType = EventType;
	Result.SourceComponent = this;
	Result.SourceUnit = GetOwner();
	Result.HitActor = InActor;
	Result.HitLocation = HitLocation;
	Result.HitNormal = HitNormal;
	Result.bHitWorld = bHitWorld;
	Result.TriggerCollision = InTriggerCollision;

	if (IsTargetCandidateValid(InActor))
	{
		Result.CandidateTargets.Add(InActor);
	}

	OnDetectionResult.Broadcast(Result);
}
