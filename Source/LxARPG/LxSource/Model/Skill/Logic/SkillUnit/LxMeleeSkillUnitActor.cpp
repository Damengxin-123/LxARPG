#include "LxMeleeSkillUnitActor.h"

#include "Components/SceneComponent.h"

ALxMeleeSkillUnitActor::ALxMeleeSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ALxMeleeSkillUnitActor::InitializeMeleeParameters(const FLxSkillMeleeSpec& InMeleeSpec)
{
	MeleeSpec = InMeleeSpec;
	MeleeSpec.MaxHitCount = FMath::Max(MeleeSpec.MaxHitCount, 0);
}

bool ALxMeleeSkillUnitActor::ReceiveWeaponHit(const FLxMeleeHitContext& InHitContext)
{
	if (!bSkillUnitActive || !IsValid(InHitContext.HitTarget))
	{
		return false;
	}


	++CurrentHitCount;
	OnMeleeHitTarget.Broadcast(this, InHitContext);

	FLxSkillUnitResult HitResult = MakeSkillUnitResult(ELxSkillUnitResultType::Hit, true);
	HitResult.HitTargets.Add(InHitContext.HitTarget);
	HitResult.HitLocations.Add(InHitContext.HitLocation);
	HitResult.SourceToTargetDirections.Add(
		(InHitContext.HitTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal());
	HitResult.TriggeredCount = CurrentHitCount;
	PublishSkillUnitHitResult(HitResult);
	return true;
}

void ALxMeleeSkillUnitActor::ActivateSkillUnit_Implementation()
{
	CurrentHitCount = 0;
	Super::ActivateSkillUnit_Implementation();
}
