#include "LxSkillDetectionComponent.h"

#include "Engine/World.h"
#include "LxARPG/LxSource/Model/Attribute/Logic/LxCharacterAttributeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitActor.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

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
		// 被目标规则过滤掉的角色默认不视为场景障碍，避免友方意外让投射物失效。
		if (OtherActor && OtherActor->IsA<ALxBaseCharacter>())
		{
			return;
		}
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
		// 被目标规则过滤掉的角色默认不视为场景障碍，避免友方意外让投射物失效。
		if (OtherActor && OtherActor->IsA<ALxBaseCharacter>())
		{
			return;
		}
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
	const ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(InActor);
	return IsBasicActorValid(InActor) && TargetCharacter
		&& IsTargetRelationAllowed(TargetCharacter)
		&& MatchesTargetStateFilter(TargetCharacter);
}

ALxBaseCharacter* ULxSkillDetectionComponent::ResolveSourceCharacter() const
{
	const ALxSkillUnitActor* SourceSkillUnit = Cast<ALxSkillUnitActor>(GetOwner());
	if (!SourceSkillUnit)
	{
		return nullptr;
	}

	if (ALxBaseCharacter* SourceCharacter = Cast<ALxBaseCharacter>(SourceSkillUnit->GetOwner()))
	{
		return SourceCharacter;
	}
	return Cast<ALxBaseCharacter>(SourceSkillUnit->GetInstigator());
}

bool ULxSkillDetectionComponent::IsTargetRelationAllowed(const ALxBaseCharacter* InTargetCharacter) const
{
	const ALxBaseCharacter* SourceCharacter = ResolveSourceCharacter();
	if (!SourceCharacter || !InTargetCharacter)
	{
		return false;
	}

	ELxSkillTargetRelation TargetRelation = ELxSkillTargetRelation::Neutral;
	if (SourceCharacter == InTargetCharacter)
	{
		TargetRelation = ELxSkillTargetRelation::Self;
	}
	else if (const ULxCharacterAttributeComponent* SourceAttributeComponent =
		SourceCharacter->GetCharacterAttributeComponent())
	{
		switch (SourceAttributeComponent->GetCharacterFactionRelation(InTargetCharacter))
		{
		case ELxCharacterFactionRelation::Friendly:
			TargetRelation = ELxSkillTargetRelation::Friendly;
			break;
		case ELxCharacterFactionRelation::Hostile:
			TargetRelation = ELxSkillTargetRelation::Hostile;
			break;
		default:
			TargetRelation = ELxSkillTargetRelation::Neutral;
			break;
		}
	}

	return (TargetFilterSpec.AllowedRelations & static_cast<int32>(TargetRelation)) != 0;
}

bool ULxSkillDetectionComponent::MatchesTargetStateFilter(const ALxBaseCharacter* InTargetCharacter) const
{
	if (!InTargetCharacter)
	{
		return false;
	}

	const ULxCharacterAttributeComponent* TargetAttributeComponent =
		InTargetCharacter->GetCharacterAttributeComponent();
	if (!TargetAttributeComponent)
	{
		return TargetFilterSpec.RequiredTags.IsEmpty();
	}

	if (!TargetFilterSpec.bIncludeDead && !TargetAttributeComponent->IsCharacterAlive())
	{
		return false;
	}

	FGameplayTagContainer TargetStateTags;
	TargetAttributeComponent->GetAllStateTags(TargetStateTags);
	if (!TargetFilterSpec.RequiredTags.IsEmpty() && !TargetStateTags.HasAll(TargetFilterSpec.RequiredTags))
	{
		return false;
	}
	if (!TargetFilterSpec.BlockedTags.IsEmpty() && TargetStateTags.HasAny(TargetFilterSpec.BlockedTags))
	{
		return false;
	}

	if (TargetFilterSpec.bRequireLineOfSight)
	{
		const ALxBaseCharacter* SourceCharacter = ResolveSourceCharacter();
		const UWorld* World = GetWorld();
		if (!SourceCharacter || !World)
		{
			return false;
		}

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LxSkillTargetLineOfSight), false);
		QueryParams.AddIgnoredActor(GetOwner());
		QueryParams.AddIgnoredActor(SourceCharacter);
		FHitResult SightHit;
		if (World->LineTraceSingleByChannel(SightHit, SourceCharacter->GetActorLocation(),
			InTargetCharacter->GetActorLocation(), ECC_Visibility, QueryParams)
			&& SightHit.GetActor() != InTargetCharacter)
		{
			return false;
		}
	}

	return true;
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
