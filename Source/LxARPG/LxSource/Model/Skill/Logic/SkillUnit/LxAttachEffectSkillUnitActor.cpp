#include "LxAttachEffectSkillUnitActor.h"

#include "Components/SceneComponent.h"
#include "LxARPG/LxSource/Model/CharacterPoint/Logic/LxCharacterAnchorPointComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillLifeComponent.h"
#include "LxARPG/LxSource/Model/Skill/Logic/SkillUnitComponent/LxSkillTriggerComponent.h"
#include "LxARPG/LxSource/Player/Characters/LxBaseCharacter.h"
#include "Net/UnrealNetwork.h"

ALxAttachEffectSkillUnitActor::ALxAttachEffectSkillUnitActor()
{
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	LifeComponent = CreateDefaultSubobject<ULxSkillLifeComponent>(TEXT("LifeComponent"));
	TriggerComponent = CreateDefaultSubobject<ULxSkillTriggerComponent>(TEXT("TriggerComponent"));
}

void ALxAttachEffectSkillUnitActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALxAttachEffectSkillUnitActor, AttachTarget);
}

bool ALxAttachEffectSkillUnitActor::InitializeFromPreviousSkillUnitResult(const FLxSkillUnitResult& PreviousResult,
	int32 HitTargetIndex, const FLxSkillAttachEffectSpec& InAttachEffectSpec)
{
	UObject* PreviousUnit = PreviousResult.SourceUnit;
	if (!PreviousResult.bSuccess || PreviousResult.ResultType != ELxSkillUnitResultType::Hit
		|| !PreviousResult.HitTargets.IsValidIndex(HitTargetIndex) || !IsValid(PreviousResult.HitTargets[HitTargetIndex]))
	{
		return false;
	}

	PreviousSkillUnit = PreviousUnit;
	AttachTarget = PreviousResult.HitTargets[HitTargetIndex];
	AttachEffectSpec = InAttachEffectSpec;
	SkillUnitSpec.LifeSpec.Duration = AttachEffectSpec.Duration;
	ApplySkillUnitSpecToComponents();

	if (const AActor* PreviousUnitActor = Cast<AActor>(PreviousUnit))
	{
		if (AActor* PreviousOwner = PreviousUnitActor->GetOwner())
		{
			SetOwner(PreviousOwner);
		}
	}
	return true;
}

void ALxAttachEffectSkillUnitActor::ActivateSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		return;
	}

	if (!HasAuthority())
	{
		AttachToResolvedTarget();
		Super::ActivateSkillUnit_Implementation();
		return;
	}

	if (!IsValid(AttachTarget) || !CanActivateAttachEffect() || !AttachToResolvedTarget())
	{
		FinishSkillUnit(MakeSkillUnitResult(ELxSkillUnitResultType::Cancelled, false));
		Destroy();
		return;
	}
	AttachTarget->OnDestroyed.AddUniqueDynamic(this, &ALxAttachEffectSkillUnitActor::HandleAttachTargetDestroyed);
	bAttachEffectApplied = false;
	bEndingAttachEffect = false;

	Super::ActivateSkillUnit_Implementation();
	if (IsSkillUnitActive())
	{
		HandleAttachEffectActivated();
	}
}

void ALxAttachEffectSkillUnitActor::StopSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAttachEffect(ELxAttachEffectEndReason::Stopped, ELxSkillUnitResultType::Completed, true);
	}
}

void ALxAttachEffectSkillUnitActor::CancelSkillUnit_Implementation()
{
	if (IsSkillUnitActive())
	{
		EndAttachEffect(ELxAttachEffectEndReason::Cancelled, ELxSkillUnitResultType::Cancelled, false);
	}
}

void ALxAttachEffectSkillUnitActor::ApplySkillUnitSpecToComponents()
{
	SkillUnitSpec.LifeSpec.Duration = AttachEffectSpec.Duration;
	Super::ApplySkillUnitSpecToComponents();
}


void ALxAttachEffectSkillUnitActor::HandleLifeStateChanged(ELxSkillAbilityComponentState OldState,
	ELxSkillAbilityComponentState NewState)
{
	if (NewState == ELxSkillAbilityComponentState::Finished)
	{
		EndAttachEffect(ELxAttachEffectEndReason::DurationExpired, ELxSkillUnitResultType::Expired, true);
	}
}

bool ALxAttachEffectSkillUnitActor::CanActivateAttachEffect() const
{
	return true;
}

void ALxAttachEffectSkillUnitActor::HandleAttachEffectActivated()
{
}

void ALxAttachEffectSkillUnitActor::TriggerAttachTargetHit()
{
	if (!IsSkillUnitActive() || !IsValid(AttachTarget) || !TriggerComponent)
	{
		return;
	}

	FLxSkillDetectionResult DetectionResult;
	DetectionResult.EventType = ELxSkillDetectionEventType::HitTarget;
	DetectionResult.SourceComponent = TriggerComponent;
	DetectionResult.SourceUnit = this;
	DetectionResult.HitActor = AttachTarget;
	DetectionResult.CandidateTargets.Add(AttachTarget);
	DetectionResult.HitLocation = AttachTarget->GetActorLocation();
	bAttachEffectApplied = true;
	TriggerComponent->HandleDetectionResult(DetectionResult);
}

void ALxAttachEffectSkillUnitActor::EndAttachEffect(ELxAttachEffectEndReason EndReason,
	ELxSkillUnitResultType ResultType, bool bSuccess)
{
	if (bEndingAttachEffect)
	{
		return;
	}
	bEndingAttachEffect = true;

	StopSkillUnitComponents();
	if (AttachTarget)
	{
		AttachTarget->OnDestroyed.RemoveDynamic(this, &ALxAttachEffectSkillUnitActor::HandleAttachTargetDestroyed);
	}
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	FLxAttachEffectEndResult EndResult;
	EndResult.AttachTarget = AttachTarget;
	EndResult.PreviousSkillUnit = PreviousSkillUnit;
	EndResult.EndReason = EndReason;
	EndResult.bEffectApplied = bAttachEffectApplied;
	OnAttachEffectEnded.Broadcast(this, EndResult);

	FLxSkillUnitResult UnitResult = MakeSkillUnitResult(ResultType, bSuccess);
	if (AttachTarget)
	{
		UnitResult.HitTargets.Add(AttachTarget);
	}
	FinishSkillUnit(UnitResult);

	if (SkillUnitSpec.LifeSpec.DelayBeforeDestroy > 0.0f)
	{
		SetLifeSpan(SkillUnitSpec.LifeSpec.DelayBeforeDestroy);
	}
	else
	{
		Destroy();
	}
}

USceneComponent* ALxAttachEffectSkillUnitActor::ResolveAttachComponent() const
{
	if (!IsValid(AttachTarget))
	{
		return nullptr;
	}

	// 角色身上的依附效果统一挂载到光环效果锚点，确保视觉表现与光环效果使用同一位置。
	if (const ALxBaseCharacter* TargetCharacter = Cast<ALxBaseCharacter>(AttachTarget))
	{
		return TargetCharacter->GetAuraEffectAnchorPoint();
	}

	return AttachTarget->GetRootComponent();
}

bool ALxAttachEffectSkillUnitActor::AttachToResolvedTarget()
{
	USceneComponent* TargetComponent = ResolveAttachComponent();
	if (!TargetComponent)
	{
		return false;
	}

	if (!AttachToComponent(TargetComponent, FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		return false;
	}

	SetActorRelativeTransform(FTransform::Identity);
	return true;
}

void ALxAttachEffectSkillUnitActor::OnRep_AttachTarget()
{
	AttachToResolvedTarget();
}

void ALxAttachEffectSkillUnitActor::HandleAttachTargetDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == AttachTarget && IsSkillUnitActive())
	{
		EndAttachEffect(ELxAttachEffectEndReason::TargetInvalid, ELxSkillUnitResultType::Cancelled, false);
	}
}
