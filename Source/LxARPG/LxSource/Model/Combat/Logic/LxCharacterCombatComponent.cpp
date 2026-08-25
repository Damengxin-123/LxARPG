#include "LxCharacterCombatComponent.h"

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

	if (SkillCastModule) SkillCastModule->InitializeModule(this);
	if (CloseCombatModule)
	{
		CloseCombatModule->InitializeModule(this);
		CloseCombatModule->OnMeleeAttackHit.AddUniqueDynamic(this, &ULxCharacterCombatComponent::HandleMeleeAttackHit);
		CloseCombatModule->OnMeleeAttackEnded.AddUniqueDynamic(this, &ULxCharacterCombatComponent::HandleMeleeAttackEnded);
		CloseCombatModule->OnBlockHit.AddUniqueDynamic(this, &ULxCharacterCombatComponent::HandleBlockHit);
		CloseCombatModule->OnBlockEnded.AddUniqueDynamic(this, &ULxCharacterCombatComponent::HandleBlockEnded);
	}
	RegisterReplicatedModules();
}

void ULxCharacterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CloseCombatModule)
	{
		CloseCombatModule->OnMeleeAttackHit.RemoveDynamic(this, &ULxCharacterCombatComponent::HandleMeleeAttackHit);
		CloseCombatModule->OnMeleeAttackEnded.RemoveDynamic(this, &ULxCharacterCombatComponent::HandleMeleeAttackEnded);
		CloseCombatModule->OnBlockHit.RemoveDynamic(this, &ULxCharacterCombatComponent::HandleBlockHit);
		CloseCombatModule->OnBlockEnded.RemoveDynamic(this, &ULxCharacterCombatComponent::HandleBlockEnded);
		CloseCombatModule->ShutdownModule();
	}
	if (SkillCastModule) SkillCastModule->ShutdownModule();
	Super::EndPlay(EndPlayReason);
}

bool ULxCharacterCombatComponent::CanStartSkillCast() const
{
	return CloseCombatModule == nullptr || CloseCombatModule->IsCloseCombatIdle();
}

bool ULxCharacterCombatComponent::CanStartCloseCombat() const
{
	return SkillCastModule == nullptr || SkillCastModule->IsSkillCastIdle();
}

void ULxCharacterCombatComponent::NotifyCombatModuleDataChanged()
{
	OnDataChange.Broadcast();
}

void ULxCharacterCombatComponent::RequestPlaySkillActionAnimation(const float InSkillReleaseDuration)
{
	MulticastPlaySkillActionAnimation(InSkillReleaseDuration);
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

void ULxCharacterCombatComponent::MulticastPlaySkillActionAnimation_Implementation(const float InSkillReleaseDuration)
{
	if (SkillCastModule) SkillCastModule->PlaySkillActionAnimation(InSkillReleaseDuration);
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
