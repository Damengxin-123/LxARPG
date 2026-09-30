#include "LxCharacterCombatComponent.h"
#include "LxARPG/LxSource/Model/DataTransfer/LxCharacterDataTransferComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"

ULxCharacterCombatComponent::ULxCharacterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
	SkillCastModule = CreateDefaultSubobject<ULxSkillCastModule>(TEXT("技能释放模块"));
	CloseCombatModule = CreateDefaultSubobject<ULxCharacterCloseCombatModule>(TEXT("近身战斗模块"));
}

void ULxCharacterCombatComponent::BaseComponentInitialize()
{
	if (bCombatInitialized) return;
	bCombatInitialized = true;
	if (const ALxBaseCharacter* Character = Cast<ALxBaseCharacter>(GetOwner()))
	{
		DataTransferComponent = Character->GetCharacterDataTransferComponent();
		if (DataTransferComponent)
			DataTransferComponent->OnAnimationEvent.AddUObject(this, &ULxCharacterCombatComponent::HandleAnimationEvent);
	}

	if (SkillCastModule) SkillCastModule->InitializeModule(this);
	if (CloseCombatModule)
	{
		CloseCombatModule->InitializeModule(this);
		if (!CloseCombatModule->OnMeleeAttackHit.IsBoundToObject(this))
		{
			CloseCombatModule->OnMeleeAttackHit.AddUObject(this, &ULxCharacterCombatComponent::HandleMeleeAttackHit);
		}
		if (!CloseCombatModule->OnMeleeAttackEnded.IsBoundToObject(this))
		{
			CloseCombatModule->OnMeleeAttackEnded.AddUObject(this, &ULxCharacterCombatComponent::HandleMeleeAttackEnded);
		}
		if (!CloseCombatModule->OnBlockHit.IsBoundToObject(this))
		{
			CloseCombatModule->OnBlockHit.AddUObject(this, &ULxCharacterCombatComponent::HandleBlockHit);
		}
		if (!CloseCombatModule->OnBlockEnded.IsBoundToObject(this))
		{
			CloseCombatModule->OnBlockEnded.AddUObject(this, &ULxCharacterCombatComponent::HandleBlockEnded);
		}
	}
	RegisterReplicatedModules();
}

void ULxCharacterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (DataTransferComponent) DataTransferComponent->OnAnimationEvent.RemoveAll(this);
	if (CloseCombatModule)
	{
		CloseCombatModule->OnMeleeAttackHit.RemoveAll(this);
		CloseCombatModule->OnMeleeAttackEnded.RemoveAll(this);
		CloseCombatModule->OnBlockHit.RemoveAll(this);
		CloseCombatModule->OnBlockEnded.RemoveAll(this);
		CloseCombatModule->ShutdownModule();
	}
	if (SkillCastModule) SkillCastModule->ShutdownModule();
	Super::EndPlay(EndPlayReason);
}

bool ULxCharacterCombatComponent::CanStartSkillCast() const
{
	return CloseCombatModule == nullptr || CloseCombatModule->IsCloseCombatIdle();
}

void ULxCharacterCombatComponent::HandleAnimationEvent(const FLxCharacterAnimationEvent& Event)
{
	if (GetOwner() && GetOwner()->HasAuthority() && SkillCastModule)
		SkillCastModule->HandleAnimationEvent(Event);
}

bool ULxCharacterCombatComponent::CanStartCloseCombat() const
{
	return SkillCastModule == nullptr || SkillCastModule->IsSkillCastIdle();
}

void ULxCharacterCombatComponent::NotifyCombatModuleDataChanged()
{
	OnDataChange.Broadcast();
}

void ULxCharacterCombatComponent::RequestPlaySkillActionAnimation(FGuid InCastId, FGameplayTag InSkillId, ELxCharacterMotionType InMotionType)
{
	MulticastPlaySkillActionAnimation(InCastId, InSkillId, InMotionType);
}

void ULxCharacterCombatComponent::RequestStopSkillActionAnimation()
{
	MulticastStopSkillActionAnimation();
}

void ULxCharacterCombatComponent::ServerHandleSkillItemReleaseInput_Implementation(const FGameplayTag InSkillItemIDTag,
	const ELxSkillReleaseInputState InInputState, AActor* InTargetActor, const FVector_NetQuantize InAimLocation,
	const bool bInHasAimLocation, const FVector_NetQuantizeNormal InAimDirection, const bool bInHasAimDirection)
{
	if (SkillCastModule)
	{
		SkillCastModule->HandleSkillItemReleaseInputFromServer(InSkillItemIDTag, InInputState, InTargetActor,
			InAimLocation, bInHasAimLocation, InAimDirection, bInHasAimDirection);
	}
}

void ULxCharacterCombatComponent::MulticastPlaySkillActionAnimation_Implementation(FGuid InCastId, FGameplayTag InSkillId, ELxCharacterMotionType InMotionType)
{
	if (SkillCastModule) SkillCastModule->PlaySkillActionAnimation(InCastId, InSkillId, InMotionType);
}

void ULxCharacterCombatComponent::MulticastStopSkillActionAnimation_Implementation()
{
	if (SkillCastModule) SkillCastModule->StopSkillActionAnimation();
}

void ULxCharacterCombatComponent::HandleMeleeAttackHit(const FLxMeleeAttackHitResult& HitResult)
{
	OnMeleeAttackHit.Broadcast(HitResult);
}

void ULxCharacterCombatComponent::HandleMeleeAttackEnded(const FLxMeleeAttackEndContext& EndContext)
{
	OnMeleeAttackEnded.Broadcast(EndContext);
}

void ULxCharacterCombatComponent::HandleBlockHit(const FLxBlockHitResult& BlockResult)
{
	OnBlockHit.Broadcast(BlockResult);
}

void ULxCharacterCombatComponent::HandleBlockEnded(const FLxBlockEndContext& EndContext)
{
	OnBlockEnded.Broadcast(EndContext);
}

void ULxCharacterCombatComponent::RegisterReplicatedModules()
{
	if (GetOwner() == nullptr || !GetOwner()->HasAuthority()) return;
	AddReplicatedSubObject(SkillCastModule);
	AddReplicatedSubObject(CloseCombatModule);
}
