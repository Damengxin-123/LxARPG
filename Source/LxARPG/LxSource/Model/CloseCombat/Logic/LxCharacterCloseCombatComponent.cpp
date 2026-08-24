#include "LxCharacterCloseCombatComponent.h"

#include "Components/PrimitiveComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterCloseCombatComponent::ULxCharacterCloseCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULxCharacterCloseCombatComponent::BaseComponentInitialize()
{
	Super::BaseComponentInitialize();
	OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
}

void ULxCharacterCloseCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CloseCombatState == ELxCloseCombatState::Attacking)
	{
		FinishAttack();
	}
	else if (CloseCombatState == ELxCloseCombatState::Blocking)
	{
		FinishBlock();
	}

	Super::EndPlay(EndPlayReason);
}

bool ULxCharacterCloseCombatComponent::StartAttack(const FLxMeleeAttackRequest& InAttackRequest)
{
	if (!IsCloseCombatIdle() || !InAttackRequest.IsValid())
	{
		return false;
	}

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	CurrentAttackRequest = InAttackRequest;
	CurrentAttackRequest.MaxHitCount = FMath::Max(0, CurrentAttackRequest.MaxHitCount);
	CurrentAttackHitCount = 0;
	CurrentHitTargets.Reset();
	CloseCombatState = ELxCloseCombatState::Attacking;
	if (OwnerCharacter)
	{
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerCharacter->GetCharacterBehaviorControlComponent())
		{
			BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Attacking);
			BehaviorControlComponent->AddFacingControlRequest();
		}
	}

	CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatComponent::HandleWeaponBeginOverlap);
	CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ULxCharacterCloseCombatComponent::HandleWeaponBeginOverlap);
	if (OwnerCharacter && CurrentAttackRequest.AttackMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentAttackRequest.AttackMontage);
	}

	OnDataChange.Broadcast();
	return true;
}

bool ULxCharacterCloseCombatComponent::EndAttack(ULxSkillUnitGroup* InSourceSkillUnitGroup)
{
	if (CloseCombatState != ELxCloseCombatState::Attacking || !MatchesCurrentAttackSource(InSourceSkillUnitGroup))
	{
		return false;
	}

	FinishAttack();
	return true;
}

bool ULxCharacterCloseCombatComponent::StartBlock(const FLxBlockRequest& InBlockRequest)
{
	if (!IsCloseCombatIdle() || !InBlockRequest.IsValid())
	{
		return false;
	}

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
	}

	CurrentBlockRequest = InBlockRequest;
	CloseCombatState = ELxCloseCombatState::Blocking;
	if (OwnerCharacter)
	{
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerCharacter->GetCharacterBehaviorControlComponent())
		{
			BehaviorControlComponent->AddBehaviorState(LxTag_CharacterState_Combat_Blocking);
			BehaviorControlComponent->AddFacingControlRequest();
		}
	}
	CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatComponent::HandleShieldBeginOverlap);
	CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.AddDynamic(this, &ULxCharacterCloseCombatComponent::HandleShieldBeginOverlap);
	if (OwnerCharacter && CurrentBlockRequest.BlockMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentBlockRequest.BlockMontage);
	}

	OnDataChange.Broadcast();
	return true;
}

bool ULxCharacterCloseCombatComponent::EndBlock()
{
	if (CloseCombatState != ELxCloseCombatState::Blocking)
	{
		return false;
	}

	FinishBlock();
	return true;
}

bool ULxCharacterCloseCombatComponent::EvaluateBlockHit_Implementation(const FLxBlockHitResult&) const
{
	return true;
}

void ULxCharacterCloseCombatComponent::HandleWeaponBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CloseCombatState != ELxCloseCombatState::Attacking || !IsValid(OtherActor) || OtherActor == GetOwner() || !IsValid(OtherComp))
	{
		return;
	}

	if (ULxCharacterCloseCombatComponent* TargetCloseCombat = OtherActor->FindComponentByClass<ULxCharacterCloseCombatComponent>())
	{
		if (TargetCloseCombat->CloseCombatState == ELxCloseCombatState::Blocking
			&& TargetCloseCombat->CurrentBlockRequest.ShieldCollision == OtherComp)
		{
			return;
		}
	}

	if (CurrentAttackRequest.MaxHitCount > 0 && CurrentAttackHitCount >= CurrentAttackRequest.MaxHitCount)
	{
		return;
	}
	if (!CurrentAttackRequest.bAllowRepeatedHitSameTarget && CurrentHitTargets.Contains(OtherActor))
	{
		return;
	}

	const FVector HitLocation = bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero()
		? FVector(SweepResult.ImpactPoint) : OtherComp->GetComponentLocation();
	FLxMeleeHitContext MeleeHitContext;
	MeleeHitContext.HitTarget = OtherActor;
	MeleeHitContext.HitLocation = HitLocation;
	if (!CurrentAttackRequest.SourceSkillUnitGroup->ReceiveMeleeWeaponHit(MeleeHitContext))
	{
		return;
	}

	++CurrentAttackHitCount;
	CurrentHitTargets.Add(OtherActor);
	FLxMeleeAttackHitResult HitResult;
	HitResult.SkillIDTag = CurrentAttackRequest.SkillIDTag;
	HitResult.SourceSkillUnitGroup = CurrentAttackRequest.SourceSkillUnitGroup;
	HitResult.HitTarget = OtherActor;
	HitResult.HitComponent = OtherComp;
	HitResult.HitLocation = HitLocation;
	OnMeleeAttackHit.Broadcast(HitResult);
}

void ULxCharacterCloseCombatComponent::HandleShieldBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CloseCombatState != ELxCloseCombatState::Blocking || !IsValid(OtherActor) || OtherActor == GetOwner() || !IsValid(OtherComp))
	{
		return;
	}

	ULxCharacterCloseCombatComponent* AttackerCloseCombat = OtherActor->FindComponentByClass<ULxCharacterCloseCombatComponent>();
	if (!AttackerCloseCombat || AttackerCloseCombat->CloseCombatState != ELxCloseCombatState::Attacking
		|| AttackerCloseCombat->CurrentAttackRequest.WeaponCollision != OtherComp)
	{
		return;
	}

	FLxBlockHitResult BlockResult;
	BlockResult.AttackSkillIDTag = AttackerCloseCombat->CurrentAttackRequest.SkillIDTag;
	BlockResult.AttackerActor = OtherActor;
	BlockResult.WeaponCollision = OtherComp;
	BlockResult.HitLocation = bFromSweep && !SweepResult.ImpactPoint.IsNearlyZero()
		? FVector(SweepResult.ImpactPoint) : CurrentBlockRequest.ShieldCollision->GetComponentLocation();
	BlockResult.bBlockSucceeded = EvaluateBlockHit(BlockResult);
	OnBlockHit.Broadcast(BlockResult);
	if (BlockResult.bBlockSucceeded)
	{
		AttackerCloseCombat->InterruptCurrentAttackByBlock();
	}
	else
	{
		FinishBlock();
	}
}

bool ULxCharacterCloseCombatComponent::MatchesCurrentAttackSource(const ULxSkillUnitGroup* InSourceSkillUnitGroup) const
{
	return InSourceSkillUnitGroup == nullptr || CurrentAttackRequest.SourceSkillUnitGroup == InSourceSkillUnitGroup;
}

void ULxCharacterCloseCombatComponent::InterruptCurrentAttackByBlock()
{
	if (CloseCombatState == ELxCloseCombatState::Attacking)
	{
		FinishAttack();
	}
}

void ULxCharacterCloseCombatComponent::FinishAttack()
{
	if (OwnerCharacter)
	{
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerCharacter->GetCharacterBehaviorControlComponent())
		{
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Attacking);
			BehaviorControlComponent->RemoveFacingControlRequest();
		}
	}
	if (CurrentAttackRequest.WeaponCollision)
	{
		CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatComponent::HandleWeaponBeginOverlap);
	}
	if (OwnerCharacter && CurrentAttackRequest.AttackMontage)
	{
		OwnerCharacter->StopAnimMontage(CurrentAttackRequest.AttackMontage);
	}

	CurrentAttackRequest = FLxMeleeAttackRequest();
	CurrentAttackHitCount = 0;
	CurrentHitTargets.Reset();
	CloseCombatState = ELxCloseCombatState::Idle;
	FLxMeleeAttackEndContext EndContext;
	OnMeleeAttackEnded.Broadcast(EndContext);
	OnDataChange.Broadcast();
}

void ULxCharacterCloseCombatComponent::FinishBlock()
{
	if (OwnerCharacter)
	{
		if (ULxCharacterBehaviorControlComponent* BehaviorControlComponent =
			OwnerCharacter->GetCharacterBehaviorControlComponent())
		{
			BehaviorControlComponent->RemoveBehaviorState(LxTag_CharacterState_Combat_Blocking);
			BehaviorControlComponent->RemoveFacingControlRequest();
		}
	}
	if (CurrentBlockRequest.ShieldCollision)
	{
		CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatComponent::HandleShieldBeginOverlap);
	}
	if (OwnerCharacter && CurrentBlockRequest.BlockMontage)
	{
		OwnerCharacter->StopAnimMontage(CurrentBlockRequest.BlockMontage);
	}

	CurrentBlockRequest = FLxBlockRequest();
	CloseCombatState = ELxCloseCombatState::Idle;
	FLxBlockEndContext EndContext;
	OnBlockEnded.Broadcast(EndContext);
	OnDataChange.Broadcast();
}
