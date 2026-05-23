#include "LxSkillDetectionComponent.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

void ULxSkillDetectionComponent::SetTargetFilterSpec(const FLxSkillTargetFilterSpec& InTargetFilterSpec)
{
	TargetFilterSpec = InTargetFilterSpec;
	OnDataChange.Broadcast();
}

void ULxSkillDetectionComponent::SetTriggerCollisionComponent(UPrimitiveComponent* InTriggerCollisionComponent)
{
	if (TriggerCollisionComponent == InTriggerCollisionComponent)
	{
		return;
	}

	if (bDetecting)
	{
		StopDetection();
		TriggerCollisionComponent = InTriggerCollisionComponent;
		StartDetection();
		return;
	}

	TriggerCollisionComponent = InTriggerCollisionComponent;
}

void ULxSkillDetectionComponent::StartDetection()
{
	if (!TriggerCollisionComponent || bDetecting)
	{
		return;
	}

	TriggerCollisionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleBeginOverlap);
	TriggerCollisionComponent->OnComponentEndOverlap.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleEndOverlap);
	TriggerCollisionComponent->OnComponentHit.AddUniqueDynamic(this, &ULxSkillDetectionComponent::HandleComponentHit);
	bDetecting = true;
}

void ULxSkillDetectionComponent::StopDetection()
{
	if (!TriggerCollisionComponent || !bDetecting)
	{
		return;
	}

	TriggerCollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleBeginOverlap);
	TriggerCollisionComponent->OnComponentEndOverlap.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleEndOverlap);
	TriggerCollisionComponent->OnComponentHit.RemoveDynamic(this, &ULxSkillDetectionComponent::HandleComponentHit);
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
		if (IsBasicCandidateValid(CandidateTarget))
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
	if (!IsBasicCandidateValid(OtherActor))
	{
		return;
	}

	CurrentCandidateTargets.AddUnique(OtherActor);
	PublishSingleActorResult(ELxSkillDetectionEventType::OverlapBegin, OtherActor, SweepResult.ImpactPoint, SweepResult.ImpactNormal, false);
}

void ULxSkillDetectionComponent::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	CurrentCandidateTargets.Remove(OtherActor);
	PublishSingleActorResult(ELxSkillDetectionEventType::OverlapEnd, OtherActor, OtherActor->GetActorLocation(), FVector::ZeroVector, false);
}

void ULxSkillDetectionComponent::HandleComponentHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const bool bHitWorld = !IsBasicCandidateValid(OtherActor);
	PublishSingleActorResult(bHitWorld ? ELxSkillDetectionEventType::HitWorld : ELxSkillDetectionEventType::HitTarget, OtherActor, Hit.ImpactPoint, Hit.ImpactNormal, bHitWorld);
}

bool ULxSkillDetectionComponent::IsBasicCandidateValid(AActor* InActor) const
{
	return InActor && InActor != GetOwner() && !InActor->IsPendingKillPending();
}

void ULxSkillDetectionComponent::PublishSingleActorResult(ELxSkillDetectionEventType EventType, AActor* InActor, const FVector& HitLocation, const FVector& HitNormal, bool bHitWorld)
{
	FLxSkillDetectionResult Result;
	Result.EventType = EventType;
	Result.SourceComponent = this;
	Result.SourceUnit = GetOwner();
	Result.HitActor = InActor;
	Result.HitLocation = HitLocation;
	Result.HitNormal = HitNormal;
	Result.bHitWorld = bHitWorld;
	Result.TriggerCollision = TriggerCollisionComponent;

	if (IsBasicCandidateValid(InActor))
	{
		Result.CandidateTargets.Add(InActor);
	}

	OnDetectionResult.Broadcast(Result);
}
