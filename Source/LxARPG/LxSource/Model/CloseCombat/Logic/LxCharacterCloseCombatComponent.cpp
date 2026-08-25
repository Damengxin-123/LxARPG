#include "LxCharacterCloseCombatComponent.h"

#include "Components/PrimitiveComponent.h"
#include "LxARPG/LxSource/Model/BehaviorControl/LxCharacterBehaviorControlComponent.h"
#include "LxARPG/LxSource/Model/Combat/Logic/LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnit/LxSkillUnitGroup.h"
#include "LxARPG/LxSource/Model/Tags/LxGameplayTags.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterCloseCombatModule::ULxCharacterCloseCombatModule()
{
}

void ULxCharacterCloseCombatModule::InitializeModule(ULxCharacterCombatComponent* InOwnerComponent)
{
	Super::InitializeModule(InOwnerComponent);
	OwnerCharacter = Cast<ALxBaseCharacter>(GetOwner());
}

void ULxCharacterCloseCombatModule::ShutdownModule()
{
	if (CloseCombatState == ELxCloseCombatState::Attacking)
	{
		FinishAttack();
	}
	else if (CloseCombatState == ELxCloseCombatState::Blocking)
	{
		FinishBlock();
	}

	OwnerCharacter = nullptr;
	Super::ShutdownModule();
}

bool ULxCharacterCloseCombatModule::StartAttack(const FLxMeleeAttackRequest& InAttackRequest)
{
	if (!IsCloseCombatIdle() || OwnerComponent == nullptr || !OwnerComponent->CanStartCloseCombat()
		|| !InAttackRequest.IsValid())
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

	CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatModule::HandleWeaponBeginOverlap);
	CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ULxCharacterCloseCombatModule::HandleWeaponBeginOverlap);
	if (OwnerCharacter && CurrentAttackRequest.AttackMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentAttackRequest.AttackMontage);
	}

	BroadcastModuleDataChanged();
	return true;
}

bool ULxCharacterCloseCombatModule::EndAttack(ULxSkillUnitGroup* InSourceSkillUnitGroup)
{
	if (CloseCombatState != ELxCloseCombatState::Attacking || !MatchesCurrentAttackSource(InSourceSkillUnitGroup))
	{
		return false;
	}

	FinishAttack();
	return true;
}

bool ULxCharacterCloseCombatModule::StartBlock(const FLxBlockRequest& InBlockRequest)
{
	if (!IsCloseCombatIdle() || OwnerComponent == nullptr || !OwnerComponent->CanStartCloseCombat()
		|| !InBlockRequest.IsValid())
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
	CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatModule::HandleShieldBeginOverlap);
	CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.AddDynamic(this, &ULxCharacterCloseCombatModule::HandleShieldBeginOverlap);
	if (OwnerCharacter && CurrentBlockRequest.BlockMontage)
	{
		OwnerCharacter->PlayAnimMontage(CurrentBlockRequest.BlockMontage);
	}

	BroadcastModuleDataChanged();
	return true;
}

bool ULxCharacterCloseCombatModule::EndBlock()
{
	if (CloseCombatState != ELxCloseCombatState::Blocking)
	{
		return false;
	}

	FinishBlock();
	return true;
}

bool ULxCharacterCloseCombatModule::EvaluateBlockHit_Implementation(const FLxBlockHitResult&) const
{
	return true;
}

void ULxCharacterCloseCombatModule::HandleWeaponBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CloseCombatState != ELxCloseCombatState::Attacking || !IsValid(OtherActor) || OtherActor == GetOwner() || !IsValid(OtherComp))
	{
		return;
	}

	ULxCharacterCombatComponent* TargetCombatComponent = OtherActor->FindComponentByClass<ULxCharacterCombatComponent>();
	if (ULxCharacterCloseCombatModule* TargetCloseCombat = TargetCombatComponent != nullptr
		? TargetCombatComponent->GetCloseCombatModule() : nullptr)
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

void ULxCharacterCloseCombatModule::HandleShieldBeginOverlap(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32, bool bFromSweep, const FHitResult& SweepResult)
{
	if (CloseCombatState != ELxCloseCombatState::Blocking || !IsValid(OtherActor) || OtherActor == GetOwner() || !IsValid(OtherComp))
	{
		return;
	}

	ULxCharacterCombatComponent* AttackerCombatComponent = OtherActor->FindComponentByClass<ULxCharacterCombatComponent>();
	ULxCharacterCloseCombatModule* AttackerCloseCombat = AttackerCombatComponent != nullptr
		? AttackerCombatComponent->GetCloseCombatModule() : nullptr;
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

bool ULxCharacterCloseCombatModule::MatchesCurrentAttackSource(const ULxSkillUnitGroup* InSourceSkillUnitGroup) const
{
	return InSourceSkillUnitGroup == nullptr || CurrentAttackRequest.SourceSkillUnitGroup == InSourceSkillUnitGroup;
}

void ULxCharacterCloseCombatModule::InterruptCurrentAttackByBlock()
{
	if (CloseCombatState == ELxCloseCombatState::Attacking)
	{
		FinishAttack();
	}
}

void ULxCharacterCloseCombatModule::FinishAttack()
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
		CurrentAttackRequest.WeaponCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatModule::HandleWeaponBeginOverlap);
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
	BroadcastModuleDataChanged();
}

void ULxCharacterCloseCombatModule::FinishBlock()
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
		CurrentBlockRequest.ShieldCollision->OnComponentBeginOverlap.RemoveDynamic(this, &ULxCharacterCloseCombatModule::HandleShieldBeginOverlap);
	}
	if (OwnerCharacter && CurrentBlockRequest.BlockMontage)
	{
		OwnerCharacter->StopAnimMontage(CurrentBlockRequest.BlockMontage);
	}

	CurrentBlockRequest = FLxBlockRequest();
	CloseCombatState = ELxCloseCombatState::Idle;
	FLxBlockEndContext EndContext;
	OnBlockEnded.Broadcast(EndContext);
	BroadcastModuleDataChanged();
}
